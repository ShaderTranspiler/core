#include "frontend/jl/type_conversion.h"

namespace stc::jl {

jl_datatype_t* TypeToJLVisitor::visit_null_id() {
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(VoidTD) {
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(BoolTD) {
    return type_cache.bool_;
}

jl_datatype_t* TypeToJLVisitor::visit(IntTD int_td) {
    switch (int_td.width) {
        case 8:
            return int_td.is_signed ? type_cache.int8 : type_cache.uint8;
        case 16:
            return int_td.is_signed ? type_cache.int16 : type_cache.uint16;
        case 32:
            return int_td.is_signed ? type_cache.int32 : type_cache.uint32;
        case 64:
            return int_td.is_signed ? type_cache.int64 : type_cache.uint64;
        case 128:
            return int_td.is_signed ? type_cache.int128 : type_cache.uint128;
    }

    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(FloatTD float_td) {
    if (float_td.enc != FloatTD::Encoding::ieee754)
        return nullptr;

    switch (float_td.width) {
        case 16:
            return type_cache.float16;
        case 32:
            return type_cache.float32;
        case 64:
            return type_cache.float64;
    }

    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit([[maybe_unused]] VectorTD vec_td) {
    // TODO
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit([[maybe_unused]] MatrixTD mat_td) {
    // TODO
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(ArrayTD arr_td) {
    if (!type_pool.get_td(arr_td.element_type_id).is_scalar())
        return nullptr;

    jl_datatype_t* el_type = dispatch(arr_td.element_type_id);
    if (el_type == nullptr)
        return nullptr;

    return type_cache.vector_of(el_type);
}

jl_datatype_t* TypeToJLVisitor::visit([[maybe_unused]] StructTD struct_td) {
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(FunctionTD) {
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(MethodTD) {
    return nullptr;
}

jl_datatype_t* TypeToJLVisitor::visit(BuiltinTD builtin_td) {
    auto kind_of = [this](TypeId type) -> uint8_t {
        const auto& td = type_pool.get_td(type);

#ifndef NDEBUG
        if (!td.is_builtin())
            throw std::logic_error{"kind_of invoked on non-builtin type descriptor"};
#endif

        return td.as<BuiltinTD>().kind;
    };

    if (builtin_td.kind == kind_of(ctx.jl_Nothing_t()))
        return jl_nothing_type;

    if (builtin_td.kind == kind_of(ctx.jl_Symbol_t()))
        return jl_symbol_type;

    if (builtin_td.kind == kind_of(ctx.jl_String_t()))
        return jl_string_type;

    return nullptr;
}

TypeId parse_jl_type(jl_datatype_t* dt, const JLCtx& ctx) {
    if (dt == nullptr)
        return TypeId::null_id();

    assert(jl_is_concrete_type(reinterpret_cast<jl_value_t*>(dt)) &&
           "parse_jl_type called with non-concrete datatype");

    const auto& type_cache = ctx.jl_env.type_cache;

    // clang-format off
    if (dt == type_cache.bool_)   return ctx.jl_Bool_t();
    
    if (dt == type_cache.int8)    return ctx.jl_Int8_t();
    if (dt == type_cache.uint8)   return ctx.jl_UInt8_t();
    if (dt == type_cache.int16)   return ctx.jl_Int16_t();
    if (dt == type_cache.uint16)  return ctx.jl_UInt16_t();
    if (dt == type_cache.int32)   return ctx.jl_Int32_t();
    if (dt == type_cache.uint32)  return ctx.jl_UInt32_t();
    if (dt == type_cache.int64)   return ctx.jl_Int64_t();
    if (dt == type_cache.uint64)  return ctx.jl_UInt64_t();
    if (dt == type_cache.int128)  return ctx.jl_Int128_t();
    if (dt == type_cache.uint128) return ctx.jl_UInt128_t();
    
    if (dt == type_cache.float16) return ctx.jl_Float16_t();
    if (dt == type_cache.float32) return ctx.jl_Float32_t();
    if (dt == type_cache.float64) return ctx.jl_Float64_t();
    // clang-format on

    return TypeId::null_id();
}

} // namespace stc::jl
