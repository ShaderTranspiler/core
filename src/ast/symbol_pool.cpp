#include "ast/symbol_pool.h"

namespace stc {

SymbolId SymbolPool::get_id(std::string_view symbol_value) {
    auto it = pool.find(symbol_value);

    if (it != pool.end())
        return it->second;

    auto [id, str_ptr] = arena_allocator.emplace(std::string{symbol_value});
    pool.emplace(*str_ptr, id);

    return SymbolId{id};
}

std::string_view SymbolPool::get_symbol(SymbolId id) const {
    assert(!id.is_null() && "lookup of invalid null symbol id");
    assert(arena.get_ptr<std::string>(id) != nullptr && "lookup of symbol id not in pool");

    return *arena.get_ptr<std::string>(id);
}

bool SymbolPool::has_id(SymbolId id) const {
    return arena.get_ptr(id) != nullptr;
}

std::optional<std::string_view> SymbolPool::get_symbol_maybe(SymbolId id) const {
    if (id.is_null())
        return std::nullopt;

    auto* result = arena.get_ptr<std::string>(id);

    if (result == nullptr)
        return std::nullopt;

    return *result;
}

} // namespace stc
