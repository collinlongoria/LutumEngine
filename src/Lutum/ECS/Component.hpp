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
#include <optional>
#include <string>
#include <type_traits>

namespace Lutum::Curia {

using ComponentID = uint32_t; // dense runtime ID
using StableKey = uint64_t; // name hash

constexpr StableKey HashName(const char* name) {
    StableKey hash = 14695981039346656037ULL;
    for (const char* c = name; *c != '\0'; ++c) {
        hash ^= static_cast<uint8_t>(*c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

struct ComponentInfo {
    StableKey key = 0;
    size_t size = 0;
    size_t alignment = 1;
    std::string name; // for debugging / tooling
};

// Global component metadata, indexed by ComponentID
namespace ComponentRegistry {
    // NOTE: re-registering an existing key returns its ID
    // NOTE: will assert on layout mismatch and on hash collision
    ComponentID Register(StableKey key, const char* name, size_t size, size_t alignment);
    [[nodiscard]]
    const ComponentInfo& Get(ComponentID id);
    // unknown keys resolve to nullopt
    [[nodiscard]]
    std::optional<ComponentID> FindByKey(StableKey key);
    [[nodiscard]]
    uint32_t Count();
} // ComponentRegistry

// Every component must declare its serialized identity
// NOTE: The NAME STRING, not the type, is the stable identity (renaming it will break saves)
template<typename T>
concept HasStableName = requires
{
    { T::kCuriaName } -> std::convertible_to<const char*>;
};

template<typename T>
concept IsValidComponent =
    std::is_trivially_copyable_v<T> &&
    std::is_standard_layout_v<T> &&
    HasStableName<T>;

template<IsValidComponent T>
struct ComponentType {
    static ComponentID Id() {
        static const ComponentID id = ComponentRegistry::Register(
            HashName(T::kCuriaName),
            T::kCuriaName,
            std::is_empty_v<T> ? 0 : sizeof(T),
            std::is_empty_v<T> ? 1 : alignof(T)
        );
        return id;
    }

    [[nodiscard]]
    static constexpr StableKey Key() { return HashName(T::kCuriaName); }
};

} // Lutum::Curia

#endif //LUTUM_CURIA_COMPONENT_HPP