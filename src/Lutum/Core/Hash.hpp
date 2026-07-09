/*
* File: Hash.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_HASH_HPP
#define LUTUM_HASH_HPP
#include <cstdint>

namespace Lutum {

using StableKey = uint64_t;

// FNV-1a
constexpr StableKey HashName(const char* name) {
    StableKey hash = 14695981039346656037ULL;
    for (const char* c = name; *c != '\0'; ++c) {
        hash ^= static_cast<uint8_t>(*c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

}

#endif //LUTUM_HASH_HPP
