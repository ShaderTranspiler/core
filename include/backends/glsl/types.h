#pragma once

#include "ir/type_pool.h"

/*
#define GL_T_DECL inline constexpr

#define GL_DEFINE_VEC_TS(ElemT, prefix)                                                            \
    GL_T_DECL ir::VectorTD gl_##prefix##vec2{ElemT, 2};                                            \
    GL_T_DECL ir::VectorTD gl_##prefix##vec3{ElemT, 3};                                            \
    GL_T_DECL ir::VectorTD gl_##prefix##vec4{ElemT, 4};

#define GL_DEFINE_MAT_TS(prefix)                                                                   \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat2x2{gl_##prefix##vec2, 2};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat2x3{gl_##prefix##vec3, 2};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat2x4{gl_##prefix##vec4, 2};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat3x2{gl_##prefix##vec2, 3};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat4x2{gl_##prefix##vec2, 4};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat3x3{gl_##prefix##vec3, 3};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat3x4{gl_##prefix##vec4, 3};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat4x3{gl_##prefix##vec3, 4};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat4x4{gl_##prefix##vec4, 4};                              \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat2 = gl_##prefix##mat2x2;                                \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat3 = gl_##prefix##mat3x3;                                \
    GL_T_DECL ir::MatrixTD gl_##prefix##mat4 = gl_##prefix##mat4x4;
*/

namespace stc::glsl {

// GL_T_DECL ir::VoidTD gl_void{};
// GL_T_DECL ir::BoolTD gl_bool{};
// GL_T_DECL ir::IntTD gl_int{32, true};
// GL_T_DECL ir::IntTD gl_uint{32, false};
// GL_T_DECL ir::FloatTD gl_float{32};
// GL_T_DECL ir::FloatTD gl_double{64};
//
// GL_DEFINE_VEC_TS(gl_float, )
// GL_DEFINE_VEC_TS(gl_double, d)
// GL_DEFINE_VEC_TS(gl_int, i)
// GL_DEFINE_VEC_TS(gl_uint, u)
// GL_DEFINE_VEC_TS(gl_bool, b)
//
// GL_DEFINE_MAT_TS()
// GL_DEFINE_MAT_TS(d)

} // namespace stc::glsl

#undef GL_T_DECL
#undef GL_DEFINE_VEC_TS
#undef GL_DEFINE_MAT_TS
