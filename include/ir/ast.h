#pragma once

#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <tuple>
#include <vector>

#include "common/base.h"
#include "common/concepts.h"
#include "common/src_info.h"
#include "common/utils.h"
#include "ir/types.h"

// CLEANUP: move dumps into visitor class, makes whole AST non-virtual
// #define __DUMP_DECL_BASE void dump(size_t level = 0, std::ostream& out = std::cerr) const
// #define DUMP_DECL_PURE virtual __DUMP_DECL_BASE = 0;
// #define DUMP_DECL_NO_OV __DUMP_DECL_BASE;
// #define DUMP_DECL __DUMP_DECL_BASE override;

#define __DUMP_DECL_BASE
#define DUMP_DECL_PURE ;
#define DUMP_DECL_NO_OV ;
#define DUMP_DECL ;

#define SAME_NODE_T_DEF(Kind)                                                                      \
    static bool same_node_t(const Stmt* stmt) {                                                    \
        return stmt->get_kind() == (Kind);                                                         \
    }

namespace stc::ir {

// CLEANUP: trailing objects pattern where applicable

struct StmtId : public StrongId<uint32_t> {
    using StrongId::StrongId;
};

struct DeclId : public StrongId<uint16_t> {
    using StrongId::StrongId;
};

struct Decl {
    SrcLocationId location;
    std::string identifier;

    explicit Decl(SrcLocationId location, std::string identifier)
        : location{location}, identifier{std::move(identifier)} {}

    Decl(const Decl&)            = default;
    Decl(Decl&&)                 = default;
    Decl& operator=(const Decl&) = default;
    Decl& operator=(Decl&&)      = default;
    virtual ~Decl()              = default;

    DUMP_DECL_PURE
};

struct Stmt {
    enum class NodeKind : uint8_t {
        FirstExpr,
        BoolLit = FirstExpr,
        IntLit,
        FloatLit,
        VecLit,
        MatLit,
        ArrLit,
        StructInstLit,
        BinOp,
        ExplCast,
        DeclRef,
        LastExpr = DeclRef,

        FirstStmt,
        Compound = FirstStmt,
        If,
        Return,
        LastStmt = Return,
    };

    static_assert(sizeof(SrcLocationId) == 4UL);
    static_assert(sizeof(NodeKind) == 1UL);

    SrcLocationId location;
    uint32_t kind         : 8;
    uint32_t type_storage : 16;
    uint32_t node_storage : 8;

    explicit Stmt(SrcLocationId location, NodeKind kind, TypeId type_storage = 0U,
                  uint8_t node_storage = 0U)
        : location{location},
          kind{static_cast<uint32_t>(kind)},
          type_storage{static_cast<uint32_t>(type_storage)},
          node_storage{static_cast<uint32_t>(node_storage)} {}

    Stmt(const Stmt&)            = default;
    Stmt(Stmt&&)                 = default;
    Stmt& operator=(const Stmt&) = default;
    Stmt& operator=(Stmt&&)      = default;
    virtual ~Stmt()              = default;

    [[nodiscard]] NodeKind get_kind() const { return static_cast<NodeKind>(kind); }

    DUMP_DECL_PURE
};

// CLEANUP: introduce concepts for these
template <typename To, typename From>
requires std::is_base_of_v<Stmt, To> && std::is_base_of_v<Stmt, From> && requires (From* ptr) {
    { To::same_node_t(ptr) } -> std::same_as<bool>;
}
To* dyn_cast(From* ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (To::same_node_t(ptr)) {
        return static_cast<To*>(ptr);
    }

    return nullptr;
}

template <typename To, typename From>
requires std::is_base_of_v<Stmt, To> && std::is_base_of_v<Stmt, From> && requires (From* ptr) {
    { To::same_node_t(ptr) } -> std::same_as<bool>;
}
std::unique_ptr<To> dyn_unique_cast(std::unique_ptr<From>&& ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (auto* cast_ptr = dyn_cast<To, From>(ptr.get())) {
        std::ignore = ptr.release();
        return std::unique_ptr<To>{cast_ptr};
    }

    return nullptr;
}

struct Expr : public Stmt {
    explicit Expr(SrcLocationId location, NodeKind kind, TypeId type)
        : Stmt{location, kind, type} {}

    explicit Expr(SrcLocationId location, NodeKind kind, TypeId type, uint8_t node_storage)
        : Stmt{location, kind, type, node_storage} {}

    TypeId type() const { return static_cast<TypeId>(type_storage); }

    static bool same_node_t(const Stmt* stmt) {
        return stmt->get_kind() >= NodeKind::FirstExpr && stmt->get_kind() <= NodeKind::LastExpr;
    }
};

struct CompoundStmt : public Stmt {
    std::vector<StmtId> body;

    explicit CompoundStmt(SrcLocationId location, std::vector<StmtId> body)
        : Stmt{location, NodeKind::Compound}, body{std::move(body)} {}

    SAME_NODE_T_DEF(NodeKind::Compound)
    DUMP_DECL_NO_OV
};
using BlockPtr = std::unique_ptr<CompoundStmt>;

// ================
//   Declarations
// ================

struct VarDecl : public Decl {
    TypeId type;
    std::optional<StmtId> initializer;

    explicit VarDecl(SrcLocationId location, std::string identifier, TypeId type,
                     std::optional<StmtId> initializer = std::nullopt)
        : Decl{location, std::move(identifier)}, type{type}, initializer{initializer} {}

    DUMP_DECL
};

// ===============
//   Expressions
// ===============

struct BoolLiteral : public Expr {
    explicit BoolLiteral(SrcLocationId location, bool value)
        : Expr{location, NodeKind::BoolLit, TypeId::bool_id(), static_cast<uint8_t>(value)} {}

    bool value() const { return static_cast<bool>(node_storage); }

    SAME_NODE_T_DEF(NodeKind::BoolLit)
    DUMP_DECL
};

struct IntLiteral : public Expr {
    std::string data;

    explicit IntLiteral(SrcLocationId location, TypeId int_type, std::string data)
        : Expr{location, NodeKind::IntLit, int_type}, data{std::move(data)} {}

    SAME_NODE_T_DEF(NodeKind::IntLit)
    DUMP_DECL
};

struct FloatLiteral : public Expr {
    std::string data;

    explicit FloatLiteral(SrcLocationId location, TypeId float_type, std::string data)
        : Expr{location, NodeKind::FloatLit, float_type}, data{std::move(data)} {}

    SAME_NODE_T_DEF(NodeKind::FloatLit)
    DUMP_DECL
};

struct VectorLiteral : public Expr {
    std::vector<StmtId> components;

    explicit VectorLiteral(SrcLocationId location, TypeId vec_type, std::vector<StmtId> components)
        : Expr{location, NodeKind::VecLit, vec_type}, components{std::move(components)} {}

    SAME_NODE_T_DEF(NodeKind::VecLit)
    DUMP_DECL
};

// column-major storage
struct MatrixLiteral : public Expr {
    std::vector<std::vector<StmtId>> data; // CLEANUP: flatten to 1D

    explicit MatrixLiteral(SrcLocationId location, TypeId mat_type,
                           std::vector<std::vector<StmtId>> data)
        : Expr{location, NodeKind::MatLit, mat_type}, data{std::move(data)} {}

    SAME_NODE_T_DEF(NodeKind::MatLit)
    DUMP_DECL
};

struct ArrayLiteral : public Expr {
    std::vector<StmtId> elements;

    explicit ArrayLiteral(SrcLocationId location, TypeId arr_type, std::vector<StmtId> elements)
        : Expr{location, NodeKind::ArrLit, arr_type}, elements{std::move(elements)} {}

    SAME_NODE_T_DEF(NodeKind::ArrLit)
    DUMP_DECL
};

struct StructInstantiationLiteral : public Expr {
    std::vector<std::pair<std::string, StmtId>> field_values;

    explicit StructInstantiationLiteral(SrcLocationId location, TypeId struct_type,
                                        std::vector<std::pair<std::string, StmtId>> field_values)
        : Expr{location, NodeKind::StructInstLit, struct_type},
          field_values{std::move(field_values)} {}

    SAME_NODE_T_DEF(NodeKind::StructInstLit)
    DUMP_DECL
};

struct BinaryOp : public Expr {
    enum class OpKind : uint8_t { add, sub, mul, div, pow, mod };

    StmtId lhs, rhs;

    // CLEANUP: remove need for explicit type
    explicit BinaryOp(SrcLocationId location, TypeId type, OpKind op, StmtId lhs, StmtId rhs)
        : Expr{location, NodeKind::BinOp, type, static_cast<uint8_t>(op)}, lhs{lhs}, rhs{rhs} {}

    OpKind op() const { return static_cast<OpKind>(node_storage); }

    SAME_NODE_T_DEF(NodeKind::BinOp)
    DUMP_DECL
};

struct ExplicitCast : public Expr {
    StmtId base;

    explicit ExplicitCast(SrcLocationId location, StmtId base, TypeId target_type)
        : Expr{location, NodeKind::ExplCast, target_type}, base{base} {}

    SAME_NODE_T_DEF(NodeKind::ExplCast)
    DUMP_DECL
};

struct DeclRefExpr : public Expr {
    DeclId decl;

    // TODO: remove need for explicit type
    explicit DeclRefExpr(SrcLocationId location, DeclId decl, TypeId decl_type)
        : Expr{location, NodeKind::DeclRef, decl_type}, decl{decl} {}

    SAME_NODE_T_DEF(NodeKind::DeclRef)
    DUMP_DECL
};

// ===============
//   Statements
// ===============

struct IfStmt : public Stmt {
    StmtId condition_expr;
    StmtId true_block;
    StmtId false_block;

    explicit IfStmt(SrcLocationId location, StmtId condition_expr, StmtId true_block,
                    StmtId false_block)
        : Stmt{location, NodeKind::If},
          condition_expr{condition_expr},
          true_block{true_block},
          false_block{false_block} {}

    SAME_NODE_T_DEF(NodeKind::If)
    DUMP_DECL
};

struct ReturnStmt : public Stmt {
    StmtId ret_value_expr;

    explicit ReturnStmt(SrcLocationId location, StmtId ret_value_expr)
        : Stmt{location, NodeKind::Return}, ret_value_expr{ret_value_expr} {}

    SAME_NODE_T_DEF(NodeKind::Return)
    DUMP_DECL
};

} // namespace stc::ir

#undef DUMP_DECL
#undef DUMP_DECL_NO_OV
#undef DUMP_DECL_PURE
#undef __DUMP_DECL_BASE

#undef SAME_NODE_T_DEF
