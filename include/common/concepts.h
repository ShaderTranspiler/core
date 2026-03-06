#pragma once

#include <iostream>
#include <unordered_map>

namespace stc {

template <typename T>
concept Printable = requires (std::ostream& os, T t) {
    { os << t } -> std::same_as<std::ostream&>;
};

template <typename T>
concept NotVoid = !std::is_void_v<T>;

template <typename T>
concept IsUnsigned = std::is_unsigned_v<T>;

template <typename T>
concept Hashable = requires (T a) {
    { std::hash<T>{}(a) } noexcept -> std::convertible_to<size_t>;
};

template <typename Key>
concept UnorderedMapKey = requires { typename std::unordered_map<Key, int>; };
}; // namespace stc
