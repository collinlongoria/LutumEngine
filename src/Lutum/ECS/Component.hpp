/*
* File: Component.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_COMPONENT_HPP
#define LUTUM_CURIA_COMPONENT_HPP
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Lutum::Curia {

using ComponentID = uint32_t;

struct ComponentInfo {
    size_t size = 0;
    size_t alignment = 1;
};

// Global component metadata, indexed by ComponentID
namespace ComponentRegistry {
    ComponentID Register(size_t size, size_t alignment);
    [[nodiscard]]
    const ComponentInfo& Get(ComponentID id);
    [[nodiscard]]
    uint32_t Count();
}

template<typename T>
concept IsValidComponent = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

/*
 * TODO: Component IDs are assigned by first-touch order at runtime. This is stable only within a single process
 * and NOT across runs, binaries, or DLL boundaries. Before serialization-by-ID or plugin support, IDs must be replaced
 * with (or mapped from) stable keys, e.g. type-name hashes.
 */
template<IsValidComponent T>
struct ComponentType {
    static ComponentID Id() {
        static const ComponentID id = ComponentRegistry::Register(
            std::is_empty_v<T> ? 0 : sizeof(T),
            std::is_empty_v<T> ? 1 : alignof(T)
        );
        return id;
    }
};

} // Lutum::Curia

#endif //LUTUM_CURIA_COMPONENT_HPP