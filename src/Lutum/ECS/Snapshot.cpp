/*
* File: Snapshot.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/ECS/Registry.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <unordered_map>

namespace Lutum::Curia {

static_assert(std::endian::native == std::endian::little,
    "snapshot format is little-endian raw; add byte swapping before porting to a BE target");

namespace {

    constexpr uint8_t kMagic[4] = {'L', 'R', 'E', 'G'};
    constexpr uint32_t kSnapshotVersion = 1;

    struct ByteWriter {
        std::vector<uint8_t> buf;

        void Bytes(const void* p, size_t n) {
            const auto* b = static_cast<const uint8_t*>(p);
            buf.insert(buf.end(), b, b + n);
        }
        void U16(uint16_t v) { Bytes(&v, sizeof(v)); }
        void U32(uint32_t v) { Bytes(&v, sizeof(v)); }
        void U64(uint64_t v) { Bytes(&v, sizeof(v)); }
    };

    struct TypeRef {
        StableKey key = 0;
        ComponentID cid = 0;
    };

} // anonymous namespace

std::vector<uint8_t> Registry::SaveSnapshot() const {
    // gather populated archetypes, signatures re-sorted by stable key
    struct ArchEntry {
        const Archetype* arch = nullptr;
        std::vector<TypeRef> types; // key-sorted
        uint32_t rows = 0;
    };

    std::vector<ArchEntry> archEntries;
    for (const auto& archPtr : m_archetypes) {
        const Archetype* arch = archPtr.get();

        uint32_t rows = 0;
        for (uint32_t s = 0; s < arch->SlabCount(); ++s)
            rows += arch->GetSlab(s)->entityCount;
        if (rows == 0)
            continue; // zero-row archetypes carry no state

        ArchEntry entry;
        entry.arch = arch;
        entry.rows = rows;
        entry.types.reserve(arch->Signature().size());
        for (ComponentID cid : arch->Signature())
            entry.types.push_back(TypeRef{ComponentRegistry::Get(cid).key, cid});
        std::sort(entry.types.begin(), entry.types.end(),
            [](const TypeRef& a, const TypeRef& b) { return a.key < b.key; });
        archEntries.push_back(std::move(entry));
    }

    // archetype order: lexicographic by key signature
    // NOTE: output bytes are independent of runtime ID assignment / creation order
    std::sort(archEntries.begin(), archEntries.end(),
        [](const ArchEntry& a, const ArchEntry& b) {
            return std::lexicographical_compare(
                a.types.begin(), a.types.end(),
                b.types.begin(), b.types.end(),
                [](const TypeRef& x, const TypeRef& y) { return x.key < y.key; });
        });

    // type table: every referenced type, key-sorted, deduplicated
    std::vector<TypeRef> typeTable;
    for (const ArchEntry& entry : archEntries)
        typeTable.insert(typeTable.end(), entry.types.begin(), entry.types.end());
    std::sort(typeTable.begin(), typeTable.end(),
              [](const TypeRef& a, const TypeRef& b) { return a.key < b.key; });
    typeTable.erase(std::unique(typeTable.begin(), typeTable.end(),
                    [](const TypeRef& a, const TypeRef& b) { return a.key == b.key; }),
                    typeTable.end());

    std::unordered_map<StableKey, uint32_t> typeIndex;
    typeIndex.reserve(typeTable.size());
    for (uint32_t i = 0; i < typeTable.size(); ++i)
        typeIndex.emplace(typeTable[i].key, i);

    // --- Write ---

    ByteWriter w;

    // Header
    w.Bytes(kMagic, sizeof(kMagic));
    w.U32(kSnapshotVersion);

    // Type Table
    w.U32(static_cast<uint32_t>(typeTable.size()));
    for (const TypeRef& t : typeTable) {
        const ComponentInfo& info = ComponentRegistry::Get(t.cid);
        w.U64(t.key);
        w.U32(static_cast<uint32_t>(info.size));
        LUTUM_ASSERT(info.name.size() <= UINT16_MAX, "component name too long");
        w.U16(static_cast<uint16_t>(info.name.size()));
        w.Bytes(info.name.data(), info.name.size());
    }

    // Directory
    w.U32(static_cast<uint32_t>(m_directory.size()));
    w.U32(static_cast<uint32_t>(m_freeIndices.size()));
    for (uint32_t index : m_freeIndices) {
        w.U32(index);
        w.U32(m_directory[index].generation);
    }

    // Archetypes
    w.U32(static_cast<uint32_t>(archEntries.size()));
    for (const ArchEntry& entry : archEntries) {
        const Archetype* arch = entry.arch;

        w.U32(static_cast<uint32_t>(entry.types.size()));
        for (const TypeRef& t : entry.types)
            w.U32(typeIndex.at(t.key));

        w.U32(entry.rows);

        // Entities, slab-major
        for (uint32_t s = 0; s < arch->SlabCount(); ++s) {
            const Slab* slab = arch->GetSlab(s);
            for (uint32_t row = 0; row < slab->entityCount; ++row)
                w.U64(arch->EntityAt(s, row));
        }

        // Packed columns, key order, tags skipped
        for (const TypeRef& t : entry.types) {
            const size_t size = arch->ComponentSize(t.cid);
            if (size == 0)
                continue;

            const size_t offset = arch->ComponentOffset(t.cid);
            for (uint32_t s = 0; s < arch->SlabCount(); ++s) {
                const Slab* slab = arch->GetSlab(s);
                if (slab->entityCount > 0)
                    w.Bytes(slab->data + offset, slab->entityCount * size);
            }
        }
    }

    return w.buf;
}
} // Lutum::Curia