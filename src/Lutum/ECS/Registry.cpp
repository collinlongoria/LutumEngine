/*
* File: Registry.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/ECS/Registry.hpp"

#include <algorithm>

namespace Lutum::Curia {

Registry::Registry() {
    auto empty = std::make_unique<Archetype>(std::vector<ComponentID>{});
    m_emptyArchetype = empty.get();
    m_archetypeLookup[HashSignature({})].push_back(m_emptyArchetype);
    m_archetypes.push_back(std::move(empty));
}

Registry::~Registry() {
    // Reverse registration order, in case resources reference each other
    for (auto it = m_resources.rbegin(); it != m_resources.rend(); ++it) {
        if (it->ptr)
            it->destroy(it->ptr);
    }
}

Entity Registry::Create() {
    LUTUM_ASSERT(!m_inObserver, "structural change from inside an observer");

    uint32_t index;
    uint32_t generation;
    if (!m_freeIndices.empty()) {
        index = m_freeIndices.back();
        m_freeIndices.pop_back();
        generation = m_directory[index].generation;
    } else {
        index = static_cast<uint32_t>(m_directory.size());
        m_directory.push_back({});
        generation = 0;
    }

    const Entity e = EntityTraits::Make(index, generation);

    auto [ci, row] = m_emptyArchetype->AllocateRow(e);
    EntityRecord& record = m_directory[index];
    record.archetype = m_emptyArchetype;
    record.slabIndex = ci;
    record.rowIndex = row;
    record.generation = generation;
    return e;
}

void Registry::Destroy(Entity e) {
    LUTUM_ASSERT(!m_inObserver, "structural change from inside an observer");

    if (!Alive(e))
        return;

    EntityRecord& record = m_directory[EntityTraits::Index(e)];

    // Remove-observers fire for every component while the row still exists, so callbacks can Get<T>(e) to read handles they need to release.
    for (ComponentID cid : record.archetype->Signature()) {
        NotifyRemove(cid, e);
    }

    const Entity moved = record.archetype->RemoveRow(record.slabIndex, record.rowIndex);
    if (moved != INVALID_ENTITY) {
        m_directory[EntityTraits::Index(moved)].rowIndex = record.rowIndex;
    }

    record.archetype = nullptr;
    record.generation++;
    m_freeIndices.push_back(EntityTraits::Index(e));
}

bool Registry::Alive(Entity e) const {
    const uint32_t index = EntityTraits::Index(e);
    if (index >= m_directory.size())
        return false;

    const EntityRecord& record = m_directory[index];
    return record.generation == EntityTraits::Generation(e) && record.archetype != nullptr;
}

void Registry::Clear() {
    LUTUM_ASSERT(!m_inObserver, "structural change from inside an observer");

    for (uint32_t index = 0; index < m_directory.size(); ++index) {
        if (m_directory[index].archetype != nullptr)
            Destroy(EntityTraits::Make(index, m_directory[index].generation));
    }

    m_directory.clear();
    m_freeIndices.clear();
}

size_t Registry::EntityCount() const {
    return m_directory.size() - m_freeIndices.size();
}

Archetype *Registry::FindOrCreateArchetype(std::vector<ComponentID> sortedSignature) {
    const uint64_t hash = HashSignature(sortedSignature);

    std::vector<Archetype*>& bucket = m_archetypeLookup[hash];
    for (Archetype* arch : bucket) {
        if (arch->Signature() == sortedSignature)
            return arch;
    }

    auto arch = std::make_unique<Archetype>(std::move(sortedSignature));
    Archetype* ptr = arch.get();
    m_archetypes.push_back(std::move(arch));
    bucket.push_back(ptr);
    return ptr;
}

void Registry::MoveEntity(Entity e, EntityRecord &record, Archetype *dstArch) {
    Archetype* srcArch = record.archetype;
    const uint32_t srcCi = record.slabIndex;
    const uint32_t srcRow = record.rowIndex;
    const Slab* srcSlab = srcArch->GetSlab(srcCi);

    auto [dstCi, dstRow] = dstArch->AllocateRow(e);
    Slab* dstSlab = dstArch->GetSlab(dstCi);

    for (ComponentID cid : srcArch->Signature()) {
        if (dstArch->HasComponent(cid)) {
            Archetype::CopyComponent(cid, *dstArch, dstSlab, dstRow, *srcArch, srcSlab, srcRow);
        }
    }

    const Entity moved = srcArch->RemoveRow(srcCi, srcRow);
    if (moved != INVALID_ENTITY) {
        m_directory[EntityTraits::Index(moved)].rowIndex = srcRow;
    }

    record.archetype = dstArch;
    record.slabIndex = dstCi;
    record.rowIndex = dstRow;
}

void Registry::NotifyAdd(ComponentID cid, Entity e) {
    if (cid >= m_onAdd.size())
        return;

    m_inObserver = true;
    for (const auto& callback : m_onAdd[cid]) {
        callback(e);
    }
    m_inObserver = false;
}

void Registry::NotifyRemove(ComponentID cid, Entity e) {
    if (cid >= m_onRemove.size())
        return;

    m_inObserver = true;
    for (const auto& callback : m_onRemove[cid]) {
        callback(e);
    }
    m_inObserver = false;
}

} // Lutum::Curia