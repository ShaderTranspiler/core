#pragma once

#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "config.h"

namespace stc {

inline std::string indent(size_t level) {
    return STC_USE_TABS ? std::string(level, '\t') : std::string(level * STC_DUMP_INDENT, ' ');
}

std::nullptr_t report(std::string_view msg, bool is_warning, std::ostream& out = std::cerr);
std::nullptr_t error(std::string_view msg, std::ostream& out = std::cerr);
std::nullptr_t warning(std::string_view msg, std::ostream& out = std::cerr);

template <typename T, typename Projection = std::identity>
requires std::regular_invocable<Projection&, const T&> && requires {
    // unordered set is instantiable with the return type of Projection
    std::unordered_set<std::remove_cvref_t<std::invoke_result_t<Projection&, const T&>>>{};
}
bool has_duplicates(const std::vector<T>& vec, Projection proj = {}) {
    using ProjType = std::remove_cvref_t<std::invoke_result_t<Projection&, const T&>>;

    std::unordered_set<ProjType> buffer;
    buffer.reserve(vec.size());

    for (const T& el : vec) {
        if (!buffer.insert(std::invoke(proj, el)).second)
            return true;
    }

    return false;
}

template <typename T>
requires std::is_integral_v<T>
inline bool is_power_of_two(T x) {
    return x != 0 && (x & (x - 1)) == 0;
}

} // namespace stc