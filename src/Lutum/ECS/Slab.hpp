/*
* File: Slab.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_SLAB_HPP
#define LUTUM_CURIA_SLAB_HPP
#include <cstddef>
#include <cstdint>

namespace Lutum::Curia {

// Fixed-size block of SoA component arrays
// Layout is owned by Archetype
// alignas(64): component array offsets are aligned relative to data[0], so the base must be at least as aligned as any component ever will be
struct Slab {
    static constexpr size_t SIZE = 16384;

    alignas(64) std::byte data[SIZE];
    uint32_t entityCount = 0;
};

} // Lutum::Curia

#endif //LUTUM_CURIA_SLAB_HPP
