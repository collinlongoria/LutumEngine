/*
* File: Resource.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/ECS/Resource.hpp"

#include <deque>
#include <mutex>
#include <unordered_map>

#include "Lutum/Core/Log.hpp"

namespace Lutum::Curia {
namespace {
    struct ResourceEntry {
        StableKey key = 0;
        std::string name;
    };

    struct RegistryState {
        std::deque<ResourceEntry> entries;
        std::unordered_map<StableKey, ResourceID> byKey;
        std::mutex mutex;
    };

    RegistryState& State() {
        static RegistryState state;
        return state;
    }
} // anonymous namespace

namespace ResourceRegistry {
    ResourceID Register(StableKey key, const char* name) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        if (auto it = state.byKey.find(key); it != state.byKey.end()) {
            LUTUM_ASSERT(state.entries[it->second].name == name, "stable-key hash collision: '{}' vs '{}' (key {:#018x})", state.entries[it->second].name, name, key);
            return it->second;
        }

        const ResourceID id = static_cast<ResourceID>(state.entries.size());
        state.entries.push_back(ResourceEntry{key, std::string(name)});
        state.byKey.emplace(key, id);
        return id;
    }

    const std::string& Name(ResourceID id) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        LUTUM_ASSERT(id < state.entries.size(), "unknown ResourceID {}", id);
        return state.entries[id].name;
    }

    uint32_t Count() {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);
        return static_cast<uint32_t>(state.entries.size());
    }
} // ResourceRegistry

} // Lutum::Curia
