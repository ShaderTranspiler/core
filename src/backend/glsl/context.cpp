#include "backend/glsl/context.h"

namespace stc::glsl {

void GLSLCtx::init_gl_types() {
    _gl_void_t   = TypeId::void_id();
    _gl_bool_t   = type_pool.bool_td();
    _gl_int_t    = type_pool.int_td(32, true);
    _gl_uint_t   = type_pool.int_td(32, false);
    _gl_float_t  = type_pool.float_td(32);
    _gl_double_t = type_pool.float_td(64);
}

} // namespace stc::glsl
