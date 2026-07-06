/*
* File: Registry.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_REGISTRY_HPP
#define LUTUM_CURIA_REGISTRY_HPP
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <span>

#include "Lutum/Core/Log.hpp"
#include "Lutum/ECS/Archetype.hpp"
#include "Lutum/ECS/Component.hpp"
#include "Lutum/ECS/Entity.hpp"
#include "Lutum/ECS/Resource.hpp"

namespace Lutum::Curia {

class Registry {
public:
    Registry();
    ~Registry();

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    // --- Entity Lifecycle ---

    [[nodiscard]]
    Entity Create();

    // Fires remove-observers for every component on the entity, then frees the row
    void Destroy(Entity e);

    [[nodiscard]]
    bool Alive(Entity e) const;

    // Destroys every entity and resets the entity directory
    // Archetypes are EMPTIED but never destroyed: persistent queries cache Archetype* and stay valid across Clear
    // Resources and observers are preserved
    // NOTE: invalidates ALL outstanding Entity handles
    void Clear();

    // --- Components ---

    template <IsValidComponent T>
    void Add(Entity e, T componentData = {});

    template <IsValidComponent T>
    void Remove(Entity e);

    template <IsValidComponent T>
    [[nodiscard]]
    bool Has(Entity e) const;

    // Pointer is invalidated by any structural change (Add/Remove/Destroy)
    // Null if dead or component absent
    // NOTE: Tags yield a dummy
    template <IsValidComponent T>
    [[nodiscard]]
    T* Get(Entity e);
    template <IsValidComponent T>
    [[nodiscard]]
    const T* Get(Entity e) const;

    template <IsValidComponent T>
    void Set(Entity e, const T& componentData);

    // --- Observers ---
    // Fried after Add / before Remove (and per-component during Destroy)
    // NOTE: observers MUST not make any structural changes directly. Defer through a CommandBuffer

    template <IsValidComponent T>
    void ObserveAdd(std::function<void(Entity)> callback);
    template <IsValidComponent T>
    void ObserveRemove(std::function<void(Entity)> callback);

    // --- Introspection ---

    [[nodiscard]]
    size_t EntityCount() const;
    [[nodiscard]]
    size_t ArchetypeCount() const { return m_archetypes.size(); }
    [[nodiscard]]
    const std::vector<std::unique_ptr<Archetype>>& Archetypes() const { return m_archetypes; }

    // --- Resources ---

    template<typename T, typename... Args>
    T& SetResource(Args&&... args) {
        const ResourceID id = ResourceType<T>::Id();
        if (id >= m_resources.size())
            m_resources.resize(id + 1);

        ResourceSlot& slot = m_resources[id];
        if (slot.ptr)
            slot.destroy(slot.ptr);

        slot.ptr = new T(std::forward<Args>(args)...);
        slot.destroy = [](void* p) { delete static_cast<T*>(p); };
        return *static_cast<T*>(slot.ptr);
    }

    template<typename T>
    [[nodiscard]]
    T* TryGetResource() {
        const ResourceID id = ResourceType<T>::Id();
        if (id >= m_resources.size() || !m_resources[id].ptr)
            return nullptr;
        return static_cast<T*>(m_resources[id].ptr);
    }

    template<typename T>
    [[nodiscard]]
    T& GetResource() {
        T* resource = TryGetResource<T>();
        LUTUM_ASSERT(resource != nullptr, "missing resource");
        return *resource;
    }

    // --- Snapshot ---

    [[nodiscard]]
    std::vector<uint8_t> SaveSnapshot() const;
    [[nodiscard]]
    bool LoadSnapshot(std::span<const uint8_t> bytes);

private:
    struct EntityRecord {
        Archetype* archetype = nullptr;
        uint32_t slabIndex = 0;
        uint32_t rowIndex = 0;
        uint32_t generation = 0;
    };

    Archetype* FindOrCreateArchetype(std::vector<ComponentID> sortedSignature);
    void MoveEntity(Entity e, EntityRecord& record, Archetype* dstArch);
    void NotifyAdd(ComponentID cid, Entity e);
    void NotifyRemove(ComponentID cid, Entity e);

    std::vector<EntityRecord> m_directory;
    std::vector<uint32_t> m_freeIndices;

    std::vector<std::unique_ptr<Archetype>> m_archetypes;
    std::unordered_map<uint64_t, std::vector<Archetype*>> m_archetypeLookup;
    Archetype* m_emptyArchetype = nullptr;

    std::vector<std::vector<std::function<void(Entity)>>> m_onAdd;
    std::vector<std::vector<std::function<void(Entity)>>> m_onRemove;

    bool m_inObserver = false;

    struct ResourceSlot {
        void* ptr = nullptr;
        void (*destroy)(void*) = nullptr;
    };
    std::vector<ResourceSlot> m_resources;
};

// --- Template impls ---

template<IsValidComponent T>
void Registry::Add(Entity e, T componentData) {
    LUTUM_ASSERT(!m_inObserver, "structural change from inside an observer");
    LUTUM_ASSERT(Alive(e), "Add on dead entity");

    EntityRecord& record = m_directory[EntityTraits::Index(e)];
    const ComponentID cid = ComponentType<T>::Id();

    if (record.archetype->HasComponent(cid))
        return;

    Archetype* dst = nullptr;
    if (auto it = record.archetype->addEdges.find(cid); it != record.archetype->addEdges.end()) {
        dst = it->second;
    }
    else {
        std::vector<ComponentID> newSig = record.archetype->Signature();
        newSig.insert(std::lower_bound(newSig.begin(), newSig.end(), cid), cid);

        dst = FindOrCreateArchetype(std::move(newSig));

        record.archetype->addEdges[cid] = dst;
        dst->removeEdges[cid] = record.archetype;
    }

    MoveEntity(e, record, dst);

    if constexpr (!std::is_empty_v<T>) {
        dst->WriteComponent(cid, dst->GetSlab(record.slabIndex), record.rowIndex, &componentData);
    }

    NotifyAdd(cid, e);
}

template<IsValidComponent T>
void Registry::Remove(Entity e) {
    LUTUM_ASSERT(!m_inObserver, "structural change from inside an observer");
    LUTUM_ASSERT(Alive(e), "Remove on dead entity");

    EntityRecord& record = m_directory[EntityTraits::Index(e)];
    const ComponentID cid = ComponentType<T>::Id();

    if (!record.archetype->HasComponent(cid))
        return;

    // Before the data disappears
    NotifyRemove(cid, e);

    Archetype* dst = nullptr;
    if (auto it = record.archetype->removeEdges.find(cid); it != record.archetype->removeEdges.end()) {
        dst = it->second;
    }
    else {
        std::vector<ComponentID> newSig;
        newSig.reserve(record.archetype->Signature().size() - 1);
        for (ComponentID c : record.archetype->Signature()) {
            if (c != cid)
                newSig.push_back(c);
        }

        dst = newSig.empty() ? m_emptyArchetype : FindOrCreateArchetype(std::move(newSig));

        record.archetype->removeEdges[cid] = dst;
        dst->addEdges[cid] = record.archetype;
    }

    MoveEntity(e, record, dst);
}

template <IsValidComponent T>
bool Registry::Has(Entity e) const {
    if (!Alive(e))
        return false;
    return m_directory[EntityTraits::Index(e)].archetype->HasComponent(ComponentType<T>::Id());
}

template <IsValidComponent T>
T* Registry::Get(Entity e) {
    if (!Alive(e))
        return nullptr;

    EntityRecord& record = m_directory[EntityTraits::Index(e)];
    const ComponentID cid = ComponentType<T>::Id();
    if (!record.archetype->HasComponent(cid))
        return nullptr;

    if constexpr (std::is_empty_v<T>) {
        static thread_local T dummy{};
        return &dummy;
    }
    else {
        Slab* slab = record.archetype->GetSlab(record.slabIndex);
        return reinterpret_cast<T*>(
            slab->data + record.archetype->ComponentOffset(cid) + record.rowIndex * sizeof(T));
    }
}

template <IsValidComponent T>
const T* Registry::Get(Entity e) const {
    return const_cast<Registry*>(this)->Get<T>(e);
}

template <IsValidComponent T>
void Registry::Set(Entity e, const T& componentData) {
    if constexpr (std::is_empty_v<T>)
        return;

    if (T* ptr = Get<T>(e))
        *ptr = componentData;
}

template <IsValidComponent T>
void Registry::ObserveAdd(std::function<void(Entity)> callback) {
    const ComponentID cid = ComponentType<T>::Id();
    if (cid >= m_onAdd.size())
        m_onAdd.resize(cid + 1);
    m_onAdd[cid].push_back(std::move(callback));
}

template <IsValidComponent T>
void Registry::ObserveRemove(std::function<void(Entity)> callback) {
    const ComponentID cid = ComponentType<T>::Id();
    if (cid >= m_onRemove.size())
        m_onRemove.resize(cid + 1);
    m_onRemove[cid].push_back(std::move(callback));
}

}

#endif //LUTUM_CURIA_REGISTRY_HPP
