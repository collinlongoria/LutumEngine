/*
* File: Resource.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_RESOURCE_HPP
#define LUTUM_CURIA_RESOURCE_HPP
#include <atomic>
#include <cstdint>
#include <type_traits>

#include "Lutum/ECS/Component.hpp"

namespace Lutum::Curia {

using ResourceID = uint32_t;

namespace ResourceRegistry {
    // NOTE: re-registering an existing key returns its ID
    ResourceID Register(StableKey key, const char* name);
    [[nodiscard]]
    const std::string& Name(ResourceID id);
    [[nodiscard]]
    uint32_t Count();
} // ResourceRegistry

// NOTE: resources declare identity the same way components do
template<typename T>
    requires std::is_class_v<T> && HasStableName<T>
struct ResourceType {
    static ResourceID Id() {
        static const ResourceID id = ResourceRegistry::Register(HashName(T::kCuriaName), T::kCuriaName);
        return id;
    }

    [[nodiscard]]
    static constexpr StableKey Key() { return HashName(T::kCuriaName); }
};

} // Lutum::Curia

#endif //LUTUM_CURIA_RESOURCE_HPP
