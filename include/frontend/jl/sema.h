#pragma once

#include "frontend/jl/scope.h"
#include "frontend/jl/sym_res.h"
#include "frontend/jl/visitor.h"

#include <span>

namespace stc::jl {

class JLSema : public JLVisitor<JLSema, JLCtx, TypeId> {
    TypePool& tpool;
    std::vector<JLScope> scopes;
    TypeId expected_type       = TypeId::null_id();
    TypeId current_fn_ret      = TypeId::null_id();
    MethodDecl* current_method = nullptr;
    bool _success              = true;
    bool visiting_method_body  = false;
    bool allow_pretyped_nodes  = false;
    bool in_interactive_ctx;

public:
    explicit JLSema(JLCtx& ctx, CompoundExpr& global_scope_body, bool in_interactive_ctx = false)
        : JLVisitor{ctx}, tpool{ctx.type_pool}, scopes{}, in_interactive_ctx{in_interactive_ctx} {

        push_scope(ScopeKind::Global, global_scope_body);
    }

    bool success() const { return _success; }
    bool is_interactive() const { return in_interactive_ctx; }

    TypeId visit_default_case();
    void finalize();

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(TypeId, type)
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
    // clang-format on

    // deferred method body visitor
    void visit_method_body(MethodDecl& method);

    bool check(Expr& expr, TypeId expected, bool allow_pretyped = false);
    bool check(NodeId node_id, TypeId expected, bool allow_pretyped = false) {
        if (node_id.is_null()) {
            _success = false;
            stc::internal_error("trying to type check node with null id");
            return false;
        }

        Expr* expr = ctx.get_node(node_id);

        if (expr == nullptr) {
            _success = false;
            stc::internal_error("arena returned nullptr for node id during type checking");
            return false;
        }

        return check(*expr, expected, allow_pretyped);
    }

    TypeId infer(Expr& expr, bool allow_pretyped = false);
    TypeId infer(NodeId node_id, bool allow_pretyped = false) {
        if (node_id.is_null()) {
            _success = false;
            stc::internal_error("trying to infer type for node with null id");
            return TypeId::null_id();
        }

        Expr* expr = ctx.get_node(node_id);

        if (expr == nullptr) {
            _success = false;
            stc::internal_error("arena returned nullptr for node id during type inference");
            return TypeId::null_id();
        }

        return infer(*expr, allow_pretyped);
    }

private:
    bool is_checking() const { return !expected_type.is_null(); }
    bool is_inferring() const { return !is_checking(); }

    void assert_scopes_notempty() const {
        assert(!scopes.empty() && "Empty scopes list in Julia Sema class");
    }

    JLScope& push_scope(ScopeKind scope_kind, CompoundExpr& body) {
        return scopes.emplace_back(scope_kind, body);
    }

    void pop_scope(bool is_global = false) {
        assert_scopes_notempty();

        // is_global is mostly just for catching accidental global scope pops (for now)
        if (is_global && scopes.size() != 1) {
            stc::internal_error("trying to pop non-global scope, but is_global argument is set to "
                                "true in pop_scope function");
            return;
        }

        if (!is_global && scopes.size() == 1) {
            stc::internal_error("trying to pop global scope, without setting is_global argument to "
                                "true in pop_scope function");
            return;
        }

        // uses index lookup for scopes and dmq, because visit_method_body could force scopes or
        // dmq to grow, potentially creating dangling references/invalidated iterators

        size_t scope_idx = scopes.size() - 1;
        for (size_t i = 0; i < scopes[scope_idx].deferred_method_queue.size(); i++) {
            NodeId m_id = scopes[scope_idx].deferred_method_queue[i];
            auto* mdecl = ctx.get_and_dyn_cast<MethodDecl>(m_id);

            if (mdecl == nullptr) {
                Expr* expr = ctx.get_node(m_id);

                if (expr == nullptr)
                    stc::internal_error("Invalid node id in deferred method body visitor queue");
                else
                    internal_error("non-method-declaration node found in deferred method body "
                                   "visitor queue",
                                   *expr);

                continue;
            }

            visit_method_body(*mdecl);
        }

        scopes.pop_back();
    }

    const JLScope& current_scope() const {
        assert_scopes_notempty();
        return scopes.back();
    }

    JLScope& current_scope() {
        assert_scopes_notempty();
        return scopes.back();
    }

    const JLScope& global_scope() const {
        assert_scopes_notempty();
        return scopes[0];
    }

    JLScope& global_scope() {
        assert_scopes_notempty();
        return scopes[0];
    }

    // false result means redeclaration attempt
    [[nodiscard]] bool st_register(SymbolId sym, NodeId decl) {
        assert_scopes_notempty();
        assert(!decl.is_null() && "Trying to declare a symbol table entry with null id as decl");

        return current_scope().st_add_sym(sym, decl);
    }

    std::optional<BindingType> binding_of(SymbolId sym) const {
        const JLScope& cur = current_scope();

        if (cur.is_global()) {
            assert(scopes.size() == 1);
            return BindingType::Global;
        }

        if (!cur.bt_contains(sym)) {
            return std::nullopt;
        }

        return cur.bt_find_sym(sym);
    }

    // attempts to find symbol in any visible scope
    NodeId find_sym(SymbolId sym) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
            auto result = it->st_find_sym(sym);

            if (!result.is_null())
                return result;
        }

        return NodeId::null_id();
    }

    // attempts to find symbol in visible non-global scopes
    NodeId find_local_sym(SymbolId sym) const {
        for (auto it = scopes.rbegin(); (it + 1) != scopes.rend(); it++) {
            auto result = it->st_find_sym(sym);

            if (!result.is_null())
                return result;
        }

        return NodeId::null_id();
    }

    NodeId find_sym_in_current_scope(SymbolId sym) const {
        assert_scopes_notempty();

        return current_scope().st_find_sym(sym);
    }

    void dump_scopes() const {
        for (const JLScope& scope : scopes)
            scope.dump(ctx);
    }

    bool check(Expr& expr, bool allow_pretyped = false) {
        return check(expr, expected_type, allow_pretyped);
    }

    bool check(NodeId node_id, bool allow_pretyped = false) {
        return check(node_id, expected_type, allow_pretyped);
    }

    void dump(const Expr& expr) const;
    std::string type_str(TypeId id) const;

    TypeId fail(std::string_view msg, const Expr& expr);
    TypeId warn(std::string_view msg, const Expr& expr);
    TypeId internal_error(std::string_view msg, const Expr& expr);

    bool is_method_sig_redecl(const MethodDecl& method_decl, const FunctionDecl& fn_decl);

    class ScopeRAII {
        using ParamDecls = std::span<std::reference_wrapper<ParamDecl>>;

        JLSema& sema;
        bool _success = true;

    public:
        explicit ScopeRAII(JLSema& sema, ScopeKind scope_kind, CompoundExpr& scope_body,
                           ParamDecls param_decls = {})
            : sema{sema} {
            sema.push_scope(scope_kind, scope_body);

            SymbolRes res{sema.ctx, sema.scopes, sema.is_interactive()};

            for (ParamDecl& pdecl : param_decls)
                res.visit(&pdecl);

            res.visit(&scope_body);
            _success = res.finalize();
        }

        explicit ScopeRAII(JLSema& sema, ScopeKind scope_kind, NodeId scope_body_id,
                           ParamDecls param_decls = {})
            : ScopeRAII{sema, scope_kind, unwrap_or_throw(sema, scope_body_id), param_decls} {}

        ~ScopeRAII() {
            try {
                sema.pop_scope();
            } catch (std::exception& e) {
                stc::internal_error("exception thrown while popping scope, see message below:");
                std::cerr << e.what() << '\n';
            } catch (...) {
                stc::internal_error("exception thrown while popping scope");
            }
        }

        ScopeRAII(const ScopeRAII&)            = delete;
        ScopeRAII(ScopeRAII&&)                 = delete;
        ScopeRAII& operator=(const ScopeRAII&) = delete;
        ScopeRAII& operator=(ScopeRAII&&)      = delete;

        bool sym_res_successful() const { return _success; }

    private:
        [[nodiscard]] CompoundExpr& unwrap_or_throw(JLSema& sema, NodeId id) {
            CompoundExpr* cmpd = sema.ctx.get_and_dyn_cast<CompoundExpr>(id);

            if (cmpd == nullptr)
                throw std::invalid_argument{
                    "Invalid node kind passed to function expecting a compound expression"};

            return *cmpd;
        }
    };
};
static_assert(CJLVisitorImpl<JLSema, TypeId>);

} // namespace stc::jl
