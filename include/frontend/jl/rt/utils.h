#pragma once

#include "frontend/jl/rt/env.h"

namespace stc::jl::rt {

// lookup chain's expected format is: [ModuleA.ModuleB.(...).]fn_name
// the chain is expected to start from root_mod
[[nodiscard]]
inline jl_function_t* find_jl_function(std::string_view lookup_chain, rt::JuliaRTEnv& env,
                                       JuliaModule& root_mod) {

    size_t last_dot_idx = lookup_chain.find_last_of('.');

    // TODO: check for dot end/begin

    JuliaModule& mod = root_mod;

    if (last_dot_idx != lookup_chain.npos)
        mod = env.module_cache.get_mod(lookup_chain.substr(0, last_dot_idx), root_mod);

    return mod.get_fn(lookup_chain.substr(last_dot_idx + 1));
}

[[nodiscard]]
inline jl_function_t* find_jl_function(std::string_view lookup_chain, rt::JuliaRTEnv& env) {
    return find_jl_function(lookup_chain, env, env.module_cache.main_mod);
}

} // namespace stc::jl::rt