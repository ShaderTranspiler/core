#pragma once

#include <cstdint>
#include <stdexcept>

namespace stc {

constexpr uint8_t operator""_u8(unsigned long long x) {
    if (x < 0 || x > 255)
        throw std::logic_error{"Using _u8 literal on value not fit for uint8_t storage"};

    return static_cast<uint8_t>(x);
}

} // namespace stc