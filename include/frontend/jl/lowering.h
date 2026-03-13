#pragma once

#include "frontend/jl/visitor.h"
#include "sir/context.h"

namespace stc::jl {

class JLLoweringVisitor final : public JLVisitor<JLLoweringVisitor, JLCtx, sir::NodeId> {
    using SIRNodeId = sir::NodeId;

public:
    explicit JLLoweringVisitor(JLCtx&& ctx)
        : JLVisitor{ctx}, sir_ctx{sir::SIRCtx::move_pools_from(std::move(ctx))} {
        // sir_ctx.src_info_pool = std::move(ctx.src_info_pool);
        // sir_ctx.type_pool     = std::move(ctx.type_pool);
    }

    SIRNodeId visit_default_case();

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(SIRNodeId, type)
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
    // clang-format on

    // CLEANUP
    /*
    sir::SIRCtx&& steal_ctx() {
        sir_ctx.src_info_pool = std::move(ctx.src_info_pool);
        return std::move(sir_ctx);
    }
    */

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
};

} // namespace stc::jl