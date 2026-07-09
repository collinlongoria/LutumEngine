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

#include <deque>
#include <mutex>
#include <unordered_map>

#include "Lutum/Core/Log.hpp"

namespace Lutum::Curia {
namespace {
    struct RegistryState {
        std::deque<ComponentInfo> infos;
        std::unordered_map<StableKey, ComponentID> byKey;
        std::mutex mutex;
    };

    RegistryState& State() {
        static RegistryState state;
        return state;
    }
} // anonymous namespace

namespace ComponentRegistry {
    ComponentID Register(StableKey key, const char* name, size_t size, size_t alignment, std::span<const FieldInfo> fields, const void* defaultData) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        if (auto it = state.byKey.find(key); it != state.byKey.end()) {
            const ComponentInfo& existing = state.infos[it->second];
            LUTUM_ASSERT(existing.name == name, "stable-key hash collision: '{}' vs '{}' (key {:#018x})", existing.name, name, key);
            LUTUM_ASSERT(existing.size == size && existing.alignment == alignment, "layout mismatch on re-register of '{}' ({}B/{} vs {}B/{})", name, existing.size, existing.alignment, size, alignment);
            LUTUM_ASSERT(existing.fields.size() == fields.size(), "field-table mismatch on re-register of '{}'", name);
            return it->second;
        }

        for (const FieldInfo& f : fields) {
            LUTUM_ASSERT(f.name != nullptr && f.count >= 1, "bad field descriptor in '{}'", name);
            LUTUM_ASSERT(f.offset + FieldTypeSize(f.type) * f.count <= size, "field '{}::{}' overruns the component ({} + {}*{} > {})", name, f.name, f.offset, FieldTypeSize(f.type), f.count, size);
        }

        std::vector<uint8_t> defaults(size, 0);
        if (size > 0 && defaultData != nullptr)
            std::memcpy(defaults.data(), defaultData, size);

        const ComponentID id = static_cast<ComponentID>(state.infos.size());
        state.infos.push_back(ComponentInfo{key, size, alignment, std::string(name), std::vector<FieldInfo>(fields.begin(), fields.end()), std::move(defaults)});
        state.byKey.emplace(key, id);
        return id;
    }

    const ComponentInfo& Get(ComponentID id) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        LUTUM_ASSERT(id < state.infos.size(), "unknown ComponentID {}", id);
        return state.infos[id];
    }

    std::optional<ComponentID> FindByKey(StableKey key) {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);

        if (auto it = state.byKey.find(key); it != state.byKey.end())
            return it->second;
        return std::nullopt;
    }

    uint32_t Count() {
        RegistryState& state = State();
        std::lock_guard lock(state.mutex);
        return static_cast<uint32_t>(state.infos.size());
    }
} // ComponentRegistry

} // Lutum::Curia