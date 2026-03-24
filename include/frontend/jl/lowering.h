#pragma once

#include "frontend/jl/visitor.h"
#include "sir/context.h"

namespace stc::jl {

class JLLoweringVisitor final : public JLVisitor<JLLoweringVisitor, JLCtx, sir::NodeId> {
    using SIRNodeId = sir::NodeId;

public:
    explicit JLLoweringVisitor(JLCtx&& ctx)
        : JLVisitor{ctx}, sir_ctx{sir::SIRCtx::move_pools_from(std::move(ctx))} {

        sym_plus     = sir_ctx.sym_pool.get_id("+");
        sym_minus    = sir_ctx.sym_pool.get_id("-");
        sym_asterisk = sir_ctx.sym_pool.get_id("*");
    }

    SIRNodeId visit_default_case();

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(SIRNodeId, type)
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
    // clang-format on

    bool successful() const { return success; }
    sir::SIRCtx sir_ctx;

private:
    bool success = true;

    template <typename T, typename... Args>
    SIRNodeId emplace_node(Args&&... args) {
        return sir_ctx.template emplace_node<T>(std::forward<Args>(args)...).first;
    }

    SIRNodeId fail(std::string_view msg);
    SIRNodeId visit_and_check(NodeId id);

    // skips id-lookup roundtrip for nodes that have already been looked up
    SIRNodeId visit_ptr(Expr* node);

    SymbolId sym_plus, sym_minus, sym_asterisk;
};

} // namespace stc::jl