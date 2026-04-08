#pragma once

#include "frontend/jl/rt/module_cache.h"

namespace stc::jl::rt {

struct JuliaTypeCache {
    jl_datatype_t* bool_ = jl_bool_type;

    jl_datatype_t* int8   = jl_int8_type;
    jl_datatype_t* uint8  = jl_uint8_type;
    jl_datatype_t* int16  = jl_int16_type;
    jl_datatype_t* uint16 = jl_uint16_type;
    jl_datatype_t* int32  = jl_int32_type;
    jl_datatype_t* uint32 = jl_uint32_type;
    jl_datatype_t* int64  = jl_int64_type;
    jl_datatype_t* uint64 = jl_uint64_type;
    jl_datatype_t* int128;
    jl_datatype_t* uint128;

    jl_datatype_t* float16 = jl_float16_type;
    jl_datatype_t* float32 = jl_float32_type;
    jl_datatype_t* float64 = jl_float64_type;

    jl_unionall_t* array_ua = jl_array_type;

    explicit JuliaTypeCache(JuliaModuleCache& mod_cache) {
        jl_module_t* jl_core = mod_cache.core_mod.mod_ptr();

        uint128 = safe_cast<jl_datatype_t>(jl_get_global(jl_core, jl_symbol("UInt128")));
        assert(uint128 != nullptr &&
               "failed to load uint128 datatype from julia through the core module");

        int128 = safe_cast<jl_datatype_t>(jl_get_global(jl_core, jl_symbol("UInt128")));
        assert(int128 != nullptr &&
               "failed to load int128 datatype from julia through the core module");
    }

    // FEATURE: internal caching for applied types (would require smart GC rooting)
    jl_datatype_t* vector_of(jl_datatype_t* el_type) const {
        jl_value_t* vec_val = jl_apply_array_type(reinterpret_cast<jl_value_t*>(el_type), 1);
        return safe_cast<jl_datatype_t>(vec_val);
    }
};

} // namespace stc::jl::rt
