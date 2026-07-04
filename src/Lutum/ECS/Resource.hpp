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

namespace Lutum::Curia {

using ResourceID = uint32_t;

namespace detail {
    inline std::atomic<ResourceID> s_resourceCounter{0};
}

// Resources are registry-singletons
// Storage lives on the Registry
// this header only provides stable runtime IDs so the Scheduler can include resource access in conflict analysis.
//
// TODO: same first-touch runtime ID caveat as ComponentType. See Component.hpp.
template <typename T>
    requires std::is_class_v<T>
struct ResourceType {
    static ResourceID Id() {
        static const ResourceID id = detail::s_resourceCounter.fetch_add(1, std::memory_order_relaxed);
        return id;
    }
};
} // Lutum::Curia

#endif //LUTUM_CURIA_RESOURCE_HPP
