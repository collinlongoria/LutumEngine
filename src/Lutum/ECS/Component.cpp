/*
* File: Component.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/ECS/Component.hpp"

#include <mutex>
#include <vector>

#include "Lutum/Core/Log.hpp"

namespace Lutum::Curia {
namespace {
    struct RegistryState {
        std::vector<ComponentInfo> infos;
        std::mutex mutex;
    };

    RegistryState& State() {
        static RegistryState state;
        return state;
    }
} // anonymous namespace

namespace ComponentRegistry {
    ComponentID Register(size_t size, size_t alignment) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        const ComponentID id = static_cast<ComponentID>(state.infos.size());
        state.infos.push_back(ComponentInfo{size, alignment});
        return id;
    }

    const ComponentInfo& Get(ComponentID id) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        LUTUM_ASSERT(id < state.infos.size(), "unknown ComponentID {}", id);
        return state.infos[id];
    }

    uint32_t Count() {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);
        return static_cast<uint32_t>(state.infos.size());
    }
} // ComponentRegistry

} // Lutum::Curia