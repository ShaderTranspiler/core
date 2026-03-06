#pragma once

#include "common/bump_arena.h"

namespace stc {

// NOTE: although this was designed to be usable both on its own, and for STL containers, it has not
// NOTE: been properly tested with them yet
template <IsUnsigned SizeT, typename T>
class BumpArenaAllocator final {
public:
    using value_type = T;

    explicit BumpArenaAllocator(BumpArena<SizeT>* arena) noexcept
        : arena{arena} {}

    template <typename U>
    BumpArenaAllocator(const BumpArenaAllocator<SizeT, U>& other) noexcept
        : arena(other.arena) {}

    template <typename U>
    friend class ArenaAllocator;

    [[nodiscard]] T* allocate(size_t n) {
        return static_cast<T*>((arena->template allocate_for<T>(n)).second);
    }

    // no-op because of arena design, memory is deallocated together
    void deallocate(T*, size_t) noexcept {}

    // NOTE: this is an extra helper, potentially useful for non-STL DSA stuff
    template <typename... Args>
    [[nodiscard]] std::pair<SizeT, T*> emplace(Args&&... args) {
        return arena->template emplace<T>(std::forward<Args>(args)...);
    }

    bool can_allocate(SizeT n = 1) { return arena->template can_allocate<T>(n); }

    bool operator==(const BumpArenaAllocator& other) { return arena == other.arena; }
    bool operator!=(const BumpArenaAllocator& other) { return !(*this == other); }

private:
    BumpArena<SizeT>* arena;
};

template <typename T>
using BumpArenaAllocator16 = BumpArenaAllocator<uint16_t, T>;

template <typename T>
using BumpArenaAllocator32 = BumpArenaAllocator<uint32_t, T>;

template <typename T>
using BumpArenaAllocator64 = BumpArenaAllocator<uint64_t, T>;

} // namespace stc
