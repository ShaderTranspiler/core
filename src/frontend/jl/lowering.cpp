#include "frontend/jl/lowering.h"
#include "sir/ast.h"

namespace {

using SIRNodeId = stc::sir::NodeId;

} // namespace

namespace stc::jl {

SIRNodeId JLLoweringVisitor::visit_default_case() {
    internal_error("nullptr found in the Julia AST during lowering to SIR");
    this->success = false;

    return SIRNodeId::null_id();
}

SIRNodeId JLLoweringVisitor::fail(std::string_view msg) {
    internal_error(msg);
    success = false;
    return SIRNodeId::null_id();
}

SIRNodeId JLLoweringVisitor::visit_and_check(NodeId id) {
    SIRNodeId result = visit(id);

    if (result.is_null()) {
        return fail("null_id returned by a node in the Julia -> SIR lowering visitor.");
    }

    return result;
}

SIRNodeId JLLoweringVisitor::visit_ptr(Expr* node) {
    return this->dispatch_wrapper(node);
}

SIRNodeId JLLoweringVisitor::visit_VarDecl(VarDecl& var) {
    SIRNodeId init =
        !var.initializer.is_null() ? visit_and_check(var.initializer) : SIRNodeId::null_id();

    return emplace_node<sir::VarDecl>(var.location, var.identifier, var.type, init);
}

SIRNodeId JLLoweringVisitor::visit_FunctionDecl(FunctionDecl& fn) {
    std::vector<SIRNodeId> params;
    params.reserve(fn.param_decls.size());

    for (NodeId param : fn.param_decls)
        params.push_back(visit_and_check(param));

    SIRNodeId body = visit_and_check(fn.body);

    return emplace_node<sir::FunctionDecl>(fn.location, fn.identifier, fn.ret_type,
                                           std::move(params), body);
}

SIRNodeId JLLoweringVisitor::visit_ParamDecl(ParamDecl& param) {
    assert(param.default_initializer.is_null() && "param with default value not caught by sema");

    return emplace_node<sir::ParamDecl>(param.location, param.identifier, param.type);
}

SIRNodeId JLLoweringVisitor::visit_StructDecl(StructDecl& struct_) {
    std::vector<SIRNodeId> fields;
    fields.reserve(struct_.field_decls.size());

    for (NodeId field : struct_.field_decls)
        fields.push_back(visit_and_check(field));

    return emplace_node<sir::StructDecl>(struct_.location, struct_.identifier, std::move(fields));
}

SIRNodeId JLLoweringVisitor::visit_FieldDecl(FieldDecl& field) {
    return emplace_node<sir::FieldDecl>(field.location, field.identifier, field.type);
}

SIRNodeId JLLoweringVisitor::visit_CompoundExpr(CompoundExpr& cmpd) {
    std::vector<SIRNodeId> sir_nodes;
    sir_nodes.reserve(cmpd.body.size());

    for (NodeId expr : cmpd.body)
        sir_nodes.push_back(visit_and_check(expr));

    return emplace_node<sir::CompoundStmt>(cmpd.location, std::move(sir_nodes));
}

SIRNodeId JLLoweringVisitor::visit_BoolLiteral(BoolLiteral& bool_lit) {
    SIRNodeId id = emplace_node<sir::BoolLiteral>(bool_lit.location, bool_lit.value());
    auto* bl     = sir_ctx.get_and_dyn_cast<sir::BoolLiteral>(id);
    bl->set_type(sir_ctx.type_pool.bool_td());

    return id;
}

#define GEN_INT_LITERAL_VISITOR(type, width, is_signed)                                            \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                               \
    SIRNodeId JLLoweringVisitor::visit_##type(type& lit) {                                         \
        return emplace_node<sir::IntLiteral>(lit.location,                                         \
                                             sir_ctx.type_pool.int_td((width), (is_signed)),       \
                                             std::to_string(lit.value));                           \
    }

GEN_INT_LITERAL_VISITOR(Int32Literal, 32, true)
GEN_INT_LITERAL_VISITOR(Int64Literal, 64, true)
GEN_INT_LITERAL_VISITOR(UInt8Literal, 8, false)
GEN_INT_LITERAL_VISITOR(UInt16Literal, 16, false)
GEN_INT_LITERAL_VISITOR(UInt32Literal, 32, false)
GEN_INT_LITERAL_VISITOR(UInt64Literal, 64, false)

SIRNodeId JLLoweringVisitor::visit_UInt128Literal([[maybe_unused]] UInt128Literal& lit) {
    return fail("unsupported UInt128 literal node not caught by sema");
}

SIRNodeId JLLoweringVisitor::visit_Float32Literal(Float32Literal& lit) {
    return emplace_node<sir::FloatLiteral>(lit.location, sir_ctx.type_pool.float_td(32),
                                           std::to_string(lit.value));
}

SIRNodeId JLLoweringVisitor::visit_Float64Literal(Float64Literal& lit) {
    return emplace_node<sir::FloatLiteral>(lit.location, sir_ctx.type_pool.float_td(64),
                                           std::to_string(lit.value));
}

SIRNodeId JLLoweringVisitor::visit_StringLiteral([[maybe_unused]] StringLiteral& lit) {
    return fail("unsupported String literal node not caught by sema");
}

SIRNodeId JLLoweringVisitor::visit_SymbolLiteral([[maybe_unused]] SymbolLiteral& lit) {
    // TODO
    throw std::logic_error{"using unimplemented feature: decl lookup"};
}

SIRNodeId JLLoweringVisitor::visit_OpaqueValue([[maybe_unused]] OpaqueValue& opaq) {
    internal_error("OpaqueValue node not caught by sema");
    success = false;
    return SIRNodeId::null_id();
}

SIRNodeId JLLoweringVisitor::visit_GlobalRef([[maybe_unused]] GlobalRef& gref) {
    throw std::logic_error{"using unimplemented feature: global refs"};
}

SIRNodeId JLLoweringVisitor::visit_DeclRefExpr([[maybe_unused]] DeclRefExpr& dre) {
    // TODO
    throw std::logic_error{"using unimplemented feature: decl lookup"};
}

SIRNodeId JLLoweringVisitor::visit_Assignment(Assignment& assign) {
    return emplace_node<sir::Assignment>(assign.location, visit_and_check(assign.target),
                                         visit_and_check(assign.value));
}

SIRNodeId JLLoweringVisitor::visit_FunctionCall(FunctionCall& fn_call) {
    auto* sym_lit = ctx.get_and_dyn_cast<SymbolLiteral>(fn_call.target_fn);

    if (sym_lit == nullptr)
        return fail("non symbol literal node in FunctionCall's target_fn not caught by sema");

    auto make_binop = [&fn_call, this](sir::BinaryOp::OpKind kind) -> SIRNodeId {
        return emplace_node<sir::BinaryOp>(fn_call.location, kind, visit_and_check(fn_call.args[0]),
                                           visit_and_check(fn_call.args[1]));
    };

    if (fn_call.args.size() == 2) {
        if (sym_lit->value == sym_plus)
            return make_binop(sir::BinaryOp::OpKind::add);
        else if (sym_lit->value == sym_minus)
            return make_binop(sir::BinaryOp::OpKind::sub);
        else if (sym_lit->value == sym_asterisk)
            return make_binop(sir::BinaryOp::OpKind::mul);
    }

    std::vector<SIRNodeId> args{};
    args.reserve(fn_call.args.size());

    for (NodeId arg : fn_call.args)
        args.push_back(visit_and_check(arg));

    return emplace_node<sir::FunctionCall>(fn_call.location, sym_lit->value, std::move(args));
}

SIRNodeId JLLoweringVisitor::visit_IfExpr(IfExpr& if_expr) {
    SIRNodeId lo_cond = visit_and_check(if_expr.condition);

    SIRNodeId lo_true =
        emplace_node<sir::ScopedStmt>(if_expr.location, visit_and_check(if_expr.true_branch));

    SIRNodeId lo_false = visit_and_check(if_expr.false_branch);
    if (!lo_false.is_null())
        lo_false = emplace_node<sir::ScopedStmt>(if_expr.location, lo_false);

    return emplace_node<sir::IfStmt>(if_expr.location, lo_cond, lo_true, lo_false);
}

SIRNodeId JLLoweringVisitor::visit_ReturnStmt(ReturnStmt& return_stmt) {
    SIRNodeId lo_inner = visit_and_check(return_stmt.inner);

    return emplace_node<sir::ReturnStmt>(return_stmt.location, lo_inner);
}

SIRNodeId JLLoweringVisitor::visit_ContinueStmt(ContinueStmt& cont_stmt) {
    return emplace_node<sir::ContinueStmt>(cont_stmt.location);
}

SIRNodeId JLLoweringVisitor::visit_BreakStmt(BreakStmt& break_stmt) {
    return emplace_node<sir::BreakStmt>(break_stmt.location);
}

} // namespace stc::jl
