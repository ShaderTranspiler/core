#pragma once

#include "ast/context.h"
#include "sir/ast.h"

namespace stc::sir {

class SIRCtx : public ASTCtx<NodeId, NodeBase, NodeKind> {
public:
    using ASTCtx<NodeId, NodeBase, NodeKind>::ASTCtx;

    SIRCtx(const SIRCtx&)                = delete;
    SIRCtx& operator=(const SIRCtx&)     = delete;
    SIRCtx(SIRCtx&&) noexcept            = default;
    SIRCtx& operator=(SIRCtx&&) noexcept = default;

    template <typename T, typename U, typename V>
    [[nodiscard]] static SIRCtx move_pools_from(ASTCtx<T, U, V>&& other,
                                                NodeId::id_type node_arena_kb = 128U) {
        return SIRCtx{std::move(other), node_arena_kb};
    }

    inline Decl* get_decl(NodeId id) const { return get_dyn<Decl>(id); }
    inline Stmt* get_stmt(NodeId id) const { return get_dyn<Stmt>(id); }
    inline Expr* get_expr(NodeId id) const { return get_dyn<Expr>(id); }
};

}; // namespace stc::sir
