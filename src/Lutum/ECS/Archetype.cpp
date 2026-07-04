/*
* File: Archetype.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/ECS/Archetype.hpp"

#include <algorithm>
#include <cstring>

#include "Lutum/Core/Log.hpp"

namespace Lutum::Curia {

uint64_t HashSignature(const std::vector<ComponentID>& sortedSignature) {
    uint64_t hash = 14695981039346656037ULL;
    for (ComponentID cid : sortedSignature) {
        for (int i = 0; i < 4; ++i) {
            hash ^= (cid >> (i * 8)) & 0xFF;
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

Archetype::Archetype(std::vector<ComponentID> signature)
    : m_signature(std::move(signature))
{
    LUTUM_ASSERT(std::is_sorted(m_signature.begin(), m_signature.end()), "archetype signature must be sorted");

    ComponentID maxId = 0;
    for (ComponentID cid : m_signature)
        maxId = std::max(maxId, cid);

    m_offsets.resize(m_signature.empty() ? 0 : maxId + 1, INVALID_OFFSET);
    m_sizes.resize(m_signature.empty() ? 0 : maxId + 1, 0);

    for (ComponentID cid : m_signature) {
        const ComponentInfo& info = ComponentRegistry::Get(cid);
        m_sizes[cid] = info.size;
    }

    ComputeLayout();
}

void Archetype::ComputeLayout() {
    /*
     * Gather non-tag components and lay them out in descending alignment order
     * Since every cpp type's size is a multiple of its alignment, this ordering keeps each array's offset aligned with zero padding in the common case; the AlignUp below covers it regardless
     */
    struct Item {
        ComponentID cid;
        size_t size;
        size_t alignment;
    };

    std::vector<Item> items;
    size_t stride = 0;

    for (ComponentID cid : m_signature) {
        const ComponentInfo& info = ComponentRegistry::Get(cid);
        LUTUM_ASSERT(info.alignment <= 64, "component alignment {} exceeds slab alignment (64)", info.alignment);

        if (info.size == 0)
            continue; // tags occupy no storage

        items.push_back(Item{cid, info.size, info.alignment});
        stride += info.size;
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.alignment != b.alignment)
            return a.alignment > b.alignment;
        return a.cid < b.cid; // deterministic tie breaker
    });

    if (stride == 0) {
        // tag-only or empty archetype
        m_capacity = static_cast<uint32_t>(Slab::SIZE);
        for (ComponentID cid : m_signature)
            m_offsets[cid] = 0;
        return;
    }

    const auto alignUp = [](size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    };

    // Start from the padding-free estimate and shrink until the padded layout fits
    // Should converge in at most a couple of iterations?
    uint32_t capacity = static_cast<uint32_t>(Slab::SIZE / stride);
    LUTUM_ASSERT(capacity > 0, "archetype row stride {} exceeds slab size", stride);

    while (capacity > 0) {
        size_t offset = 0;
        for (const Item& item : items) {
            offset = alignUp(offset, item.alignment);
            offset += item.size * capacity;
        }

        if (offset <= Slab::SIZE)
            break;
        --capacity;
    }

    LUTUM_ASSERT(capacity > 0, "no capacity for achetype layout");
    m_capacity = capacity;

    size_t offset = 0;
    for (const Item& item : items) {
        offset = alignUp(offset, item.alignment);
        m_offsets[item.cid] = offset;
        offset += item.size * capacity;
    }

    // Tags: valid offset, no storage.
    for (ComponentID cid : m_signature) {
        if (m_sizes[cid] == 0)
            m_offsets[cid] = 0;
    }
}

std::pair<uint32_t, uint32_t> Archetype::AllocateRow(Entity entity) {
    for (uint32_t ci = 0; ci < m_slabs.size(); ++ci) {
        Slab* slab = m_slabs[ci].get();
        if (slab->entityCount < m_capacity) {
            const uint32_t row = slab->entityCount++;
            m_entities[ci].push_back(entity);
            return {ci, row};
        }
    }

    auto slab = std::make_unique<Slab>();
    slab->entityCount = 1;
    m_slabs.push_back(std::move(slab));
    m_entities.push_back({entity});
    return {static_cast<uint32_t>(m_slabs.size() - 1), 0};
}

Entity Archetype::RemoveRow(uint32_t slabIndex, uint32_t row) {
    Slab* slab = m_slabs[slabIndex].get();
    const uint32_t last = slab->entityCount - 1;
    Entity moved = INVALID_ENTITY;

    if (row != last) {
        for (ComponentID cid : m_signature) {
            const size_t size = m_sizes[cid];
            if (size == 0)
                continue;

            const size_t base = m_offsets[cid];
            std::memcpy(slab->data + base + row * size,
                        slab->data + base + last * size,
                        size);
        }
        moved = m_entities[slabIndex][last];
        m_entities[slabIndex][row] = moved;
    }

    m_entities[slabIndex].pop_back();
    slab->entityCount--;
    return moved;
}

void Archetype::WriteComponent(ComponentID cid, Slab* slab, uint32_t row, const void* data) {
    const size_t size = m_sizes[cid];
    if (size == 0)
        return;
    std::memcpy(slab->data + m_offsets[cid] + row * size, data, size);
}

void Archetype::ReadComponent(ComponentID cid, const Slab* slab, uint32_t row, void* out) const {
    const size_t size = m_sizes[cid];
    if (size == 0)
        return;
    std::memcpy(out, slab->data + m_offsets[cid] + row * size, size);
}

void Archetype::CopyComponent(ComponentID cid,
                              const Archetype& dstArch, Slab* dstSlab, uint32_t dstRow,
                              const Archetype& srcArch, const Slab* srcSlab, uint32_t srcRow) {
    const size_t size = dstArch.m_sizes[cid];
    if (size == 0)
        return;

    std::memcpy(dstSlab->data + dstArch.m_offsets[cid] + dstRow * size,
                srcSlab->data + srcArch.m_offsets[cid] + srcRow * size,
                size);
}
} // Lutum::Curia