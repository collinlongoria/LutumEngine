/*
* File: Entity.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_ENTITY_HPP
#define LUTUM_CURIA_ENTITY_HPP
#include <cstdint>

namespace Lutum::Curia {

// Index in low 32 bits, generation in high 32
using Entity = uint64_t;

inline constexpr Entity INVALID_ENTITY = ~static_cast<Entity>(0);

struct EntityTraits {
    [[nodiscard]]
    static constexpr uint32_t Index(Entity e) { return static_cast<uint32_t>(e); }
    [[nodiscard]]
    static constexpr uint32_t Generation(Entity e) { return static_cast<uint32_t>(e >> 32); }
    [[nodiscard]]
    static constexpr Entity Make(uint32_t index, uint32_t generation) {
        return (static_cast<Entity>(generation) << 32) | index;
    }
};

} // Lutum::Curia

#endif //LUTUM_CURIA_ENTITY_HPP
