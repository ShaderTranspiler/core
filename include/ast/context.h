#pragma once

#include "common/bump_arena.h"
#include "common/dyn_cast.h"
#include "common/src_info.h"
#include "types/type_pool.h"

namespace stc {

// base AST context class

// encapsulates data needed for any sort of handling of an AST, regardless of its type or
// implementation

// NodeIdTy is the size type used to store arena indices/offsets when referring to nodes
// NodeBaseTy is the base class from which all AST types are derived
// NodeKindTy is the kind (enum) type used to uniquely identify different node types
// Derived should be provided by classes deriving from this, so that partial movability can be
// ensured automatically at compile-time through concepts

template <CNullableStrongId NodeIdTy, typename NodeBaseTy, typename NodeKindTy>
class ASTCtx {
public:
    using node_id_type   = NodeIdTy;
    using node_base_type = NodeBaseTy;
    using node_kind_type = NodeKindTy;

protected:
    BumpArena<typename NodeIdTy::id_type> node_arena;

public:
    types::TypePool type_pool;
    SrcInfoPool src_info_pool;

    explicit ASTCtx(std::vector<types::BuiltinTD> type_builtins = {},
                    NodeIdTy::id_type node_arena_kb             = 128U,
                    SrcLocationId::id_type src_info_arena_kb    = 128U,
                    types::TypeId::id_type type_arena_kb        = 32U)
        : node_arena{node_arena_kb * 1024U},
          type_pool{static_cast<types::TypeId::id_type>(type_arena_kb * 1024U),
                    std::move(type_builtins)},
          src_info_pool{src_info_arena_kb * 1024U} {}

    ASTCtx(const ASTCtx&)                  = delete;
    ASTCtx& operator=(const ASTCtx&) const = delete;
    ASTCtx(ASTCtx&&) noexcept              = default;
    ASTCtx& operator=(ASTCtx&&) noexcept   = default;

protected:
    // "partial" move ctor, moving the type and src info pools, and empty initializing node arena
    // protected to allow derived classes to use as a starting point, but publicly only accessible
    // from the factory function move_pools_from
    template <typename T, typename U, typename V>
    explicit ASTCtx(ASTCtx<T, U, V>&& other, NodeIdTy::id_type node_arena_kb)
        : node_arena{node_arena_kb * 1024U},
          type_pool{std::move(other.type_pool)},
          src_info_pool{std::move(other.src_info_pool)} {}

public:
    template <typename T, typename U, typename V>
    [[nodiscard]] static ASTCtx move_pools_from(ASTCtx<T, U, V>&& other,
                                                NodeIdTy::id_type node_arena_kb = 128U) {
        return ASTCtx{std::move(other), node_arena_kb};
    }

    template <typename T, typename... Args>
    requires std::derived_from<T, NodeBaseTy>
    [[nodiscard]] std::pair<NodeIdTy, T*> emplace_node(Args&&... args) {
        return node_arena.template emplace<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] inline NodeBaseTy* get_node(NodeIdTy id) const {
        return static_cast<NodeBaseTy*>(node_arena.get_ptr(id));
    }

    template <typename T>
    requires CDynCastable<T, NodeBaseTy>
    [[nodiscard]] inline T* get_dyn(NodeIdTy id) const {
        return dyn_cast<T>(get_node(id));
    }

    // helper, mainly meant for debug assertions
    template <typename T>
    requires CDynCastable<T, NodeBaseTy>
    bool isa(NodeIdTy id) const {
        if (id == NodeIdTy::null_id())
            return false;

        T* node = get_dyn<T>(id);

        return node != nullptr;
    }

    operator const types::TypePool&() const { return type_pool; }
    operator const SrcInfoPool&() const { return src_info_pool; }
};

}; // namespace stc
