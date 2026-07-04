/*
* File: Archetype.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_ARCHETYPE_HPP
#define LUTUM_CURIA_ARCHETYPE_HPP
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Lutum/ECS/Slab.hpp"
#include "Lutum/ECS/Component.hpp"
#include "Lutum/ECS/Entity.hpp"

namespace Lutum::Curia {

inline constexpr size_t INVALID_OFFSET = std::numeric_limits<size_t>::max();

// FNV-1a over a sorted signature
// Used by Registry for archetype lookup
[[nodiscard]]
uint64_t HashSignature(const std::vector<ComponentID>& sortedSignature);

class Archetype {
public:
    // Signature must be sorted
    // Registry is consulted for size/alignments
    explicit Archetype(std::vector<ComponentID> signature);

    Archetype(const Archetype&) = delete;
    Archetype& operator=(const Archetype&) = delete;

    [[nodiscard]]
    const std::vector<ComponentID>& Signature() const { return m_signature; }
    [[nodiscard]]
    uint32_t Capacity() const { return m_capacity; }
    [[nodiscard]]
    bool HasComponent(ComponentID cid) const {
        return cid < m_offsets.size() && m_offsets[cid] != INVALID_OFFSET;
    }
    [[nodiscard]]
    size_t ComponentOffset(ComponentID cid) const { return m_offsets[cid]; }
    [[nodiscard]]
    size_t ComponentSize(ComponentID cid) const { return m_sizes[cid]; }

    [[nodiscard]]
    uint32_t SlabCount() const { return static_cast<uint32_t>(m_slabs.size()); }
    [[nodiscard]]
    Slab* GetSlab(uint32_t index) { return m_slabs[index].get(); }
    [[nodiscard]]
    const Slab* GetSlab(uint32_t index) const { return m_slabs[index].get(); }
    [[nodiscard]]
    Entity EntityAt(uint32_t slabIndex, uint32_t row) const {
        return m_entities[slabIndex][row];
    }

    // Reserves a row for entity; returns {slabIndex, row}
    // NOTE: grows as needed
    std::pair<uint32_t, uint32_t> AllocateRow(Entity entity);

    // Returns the entity that was moved into 'row' or INVALID_ENTITY if none
    Entity RemoveRow(uint32_t slabIndex, uint32_t row);

    void WriteComponent(ComponentID cid, Slab* slab, uint32_t row, const void* data);
    void ReadComponent(ComponentID cid, const Slab* slab, uint32_t row, void* out) const;
    static void CopyComponent(ComponentID cid,
        const Archetype& dstArch, Slab* dstSlab, uint32_t dstRow,
        const Archetype& srcArch, const Slab* srcSlab, uint32_t srcRow);

    // Transition graph: archetype reached by adding/removing one component
    std::unordered_map<ComponentID, Archetype*> addEdges;
    std::unordered_map<ComponentID, Archetype*> removeEdges;

private:
    void ComputeLayout();

    std::vector<ComponentID> m_signature;
    std::vector<std::unique_ptr<Slab>> m_slabs;
    std::vector<std::vector<Entity>> m_entities; // per-slab, parallel to rows

    // Flat, indexed by ComponentID
    // INVALID_OFFSET = not in this archetype
    std::vector<size_t> m_offsets;
    std::vector<size_t> m_sizes;

    uint32_t m_capacity = 0; // entities per slab
};

} // Lutum::Curia

#endif //LUTUM_CURIA_ARCHETYPE_HPP
