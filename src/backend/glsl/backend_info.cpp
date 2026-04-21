#include "backend/glsl/backend_info.h"
#include "backend/glsl/builtin_data.h"

#include <algorithm>

namespace stc::glsl {

namespace detail {
using namespace stc::glsl::builtins;

TypeId builtin_ty_to_id(BuiltinType ty, GLSLTypes& types) {
    using enum BuiltinType;

    assert(!is_generic_type(ty) && "generic type not unrolled during builtin parsing");

    if (ty == Void)
        return types.gl_void;

    if (ty == Float)
        return types.gl_float;

    if (ty == Double)
        return types.gl_double;

    if (ty == Int)
        return types.gl_int;

    if (ty == UInt)
        return types.gl_uint;

    if (ty == Bool)
        return types.gl_bool;

    if (is_vec(ty)) {
        BuiltinType el_type = el_type_or_self(ty);
        assert(el_type != ty);
        TypeId el_type_id = builtin_ty_to_id(el_type, types);
        return types.gl_TvecN(el_type_id, el_count(ty));
    }

    if (is_mat(ty)) {
        BuiltinType el_type = el_type_or_self(ty);
        assert(el_type != ty);
        TypeId el_type_id = builtin_ty_to_id(el_type, types);

        uint8_t col_count_ = col_count(ty);
        uint8_t row_count_ = row_count(ty);

        return types.gl_TmatNxM(el_type_id, col_count_, row_count_);
    }

    throw std::logic_error{"Invalid signature in GLSL's builtin function list"};
}

} // namespace detail

GLSLBackendInfo GLSLBackendInfo::create_instance(GLSLTypes& types) {
    GlobalList globals = create_globals(types);
    FnList functions   = create_fns(types);

    // globals and functions should already be stored in alphabetical order name-wise in the binary
    // so parsing them in sequential order should already yield a sorted result
    // (if not, an assert will catch it in the base class)

    return GLSLBackendInfo{std::move(globals), std::move(functions)};
}

BackendInfo::GlobalList GLSLBackendInfo::create_globals(GLSLTypes& types) {
    // CLEANUP: play around with clang format options here
    return {
        // clang-format off
        {"gl_FragCoord",   types.gl_vec4},
        {"gl_FragDepth",   types.gl_float},
        {"gl_FrontFacing", types.gl_bool},
        {"gl_InstanceID",  types.gl_int},
        {"gl_Position",    types.gl_vec4},
        // clang-format on
    };
}

BackendInfo::FnList GLSLBackendInfo::create_fns(GLSLTypes& types) {
    FnList fns{};
    fns.reserve(builtins::builtin_fns.size());

    for (const auto& fn : builtins::builtin_fns) {
        OverloadList overloads{};
        overloads.reserve(fn.overloads.size());

        for (const auto& overload : fn.overloads) {
            TypeList arg_types{};
            arg_types.reserve(overload.arg_count);

            for (size_t i = 0; i < overload.arg_count; i++)
                arg_types.emplace_back(detail::builtin_ty_to_id(overload.args[i], types));

            overloads.emplace_back(std::move(arg_types),
                                   detail::builtin_ty_to_id(overload.ret_ty, types));
        }

        fns.emplace_back(fn.name, std::move(overloads));
    }

    return fns;
}

bool GLSLBackendInfo::can_implicit_cast([[maybe_unused]] TypeId src_ty,
                                        [[maybe_unused]] TypeId dest_ty) const {
    return false;
}

} // namespace stc::glsl
