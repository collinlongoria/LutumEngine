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
#include <optional>
#include <span>

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

    struct ByteReader {
        const uint8_t* data = nullptr;
        size_t size = 0;
        size_t pos = 0;

        bool Bytes(void* out, size_t n) {
            if (n > size - pos)
                return false;
            std::memcpy(out, data + pos, n);
            pos += n;
            return true;
        }

        // zero-copy view into the buffer
        // nullptr on overrun
        [[nodiscard]]
        const uint8_t* View(size_t n) {
            if (n > size - pos)
                return nullptr;
            const uint8_t* p = data + pos;
            pos += n;
            return p;
        }

        template <typename T>
        bool Read(T& out) { return Bytes(&out, sizeof(T)); }
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

bool Registry::LoadSnapshot(std::span<const uint8_t> bytes) {
    LUTUM_ASSERT(m_directory.empty() && m_archetypes.size() == 1, "LoadSnapshot: requires a freshly constructed registry");

    ByteReader r{bytes.data(), bytes.size()};

    // Header

    uint8_t magic[4] = {};
    if (!r.Bytes(magic, sizeof(magic)) || std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
        LUTUM_ERROR("snapshot: bad magic");
        return false;
    }
    uint32_t version = 0;
    if (!r.Read(version) || version != kSnapshotVersion) {
        LUTUM_ERROR("snapshot: unsupported version {} (expected {})", version, kSnapshotVersion);
        return false;
    }

    // parse + validate

    struct FileType {
        StableKey key = 0;
        uint32_t size = 0;
        std::string name;
        std::optional<ComponentID> cid;
    };

    uint32_t typeCount = 0;
    if (!r.Read(typeCount)) {
        LUTUM_ERROR("snapshot: truncated type table");
        return false;
    }

    std::vector<FileType> types(typeCount);
    for (FileType& t : types) {
        uint16_t nameLen = 0;
        if (!r.Read(t.key) || !r.Read(t.size) || !r.Read(nameLen)) {
            LUTUM_ERROR("snapshot: truncated type table");
            return false;
        }
        const uint8_t* namePtr = r.View(nameLen);
        if (!namePtr) {
            LUTUM_ERROR("snapshot: truncated type name");
            return false;
        }
        t.name.assign(reinterpret_cast<const char*>(namePtr), nameLen);

        t.cid = ComponentRegistry::FindByKey(t.key);
        if (t.cid) {
            const ComponentInfo& info = ComponentRegistry::Get(*t.cid);
            if (info.size != t.size) {
                LUTUM_ERROR("snapshot: layout mismatch for '{}': file {}B, runtime {}B — "
                            "component changed since save (bump snapshot version)",
                            t.name, t.size, info.size);
                return false;
            }
        }
        else {
            LUTUM_WARN("snapshot: dropping unknown component '{}' (key {:#018x})", t.name, t.key);
        }
    }

    // Directory

    uint32_t directoryCount = 0;
    uint32_t freeCount = 0;
    if (!r.Read(directoryCount) || !r.Read(freeCount) || freeCount > directoryCount) {
        LUTUM_ERROR("snapshot: bad directory header");
        return false;
    }

    std::vector<std::pair<uint32_t, uint32_t>> freeList(freeCount); // (index, generation)
    std::vector<bool> claimed(directoryCount, false);
    for (auto& [index, generation] : freeList) {
        if (!r.Read(index) || !r.Read(generation) || index >= directoryCount || claimed[index]) {
            LUTUM_ERROR("snapshot: bad free-list entry");
            return false;
        }
        claimed[index] = true;
    }

    // Archetypes

    struct FileArch {
        std::vector<uint32_t> sig; // type-table indices, strictly ascending
        uint32_t rows = 0;
        const uint8_t* entities = nullptr; // rows * sizeof(Entity)
        std::vector<const uint8_t*> columns; // parallel to sig; nullptr for tags
    };

    uint32_t archCount = 0;
    if (!r.Read(archCount)) {
        LUTUM_ERROR("snapshot: truncated archetype list");
        return false;
    }

    std::vector<FileArch> arches(archCount);
    uint64_t aliveRows = 0;
    for (FileArch& fa : arches) {
        uint32_t sigCount = 0;
        if (!r.Read(sigCount)) {
            LUTUM_ERROR("snapshot: truncated signature");
            return false;
        }
        fa.sig.resize(sigCount);
        for (size_t i = 0; i < sigCount; ++i) {
            if (!r.Read(fa.sig[i]) || fa.sig[i] >= typeCount ||
                (i > 0 && fa.sig[i] <= fa.sig[i - 1])) {
                LUTUM_ERROR("snapshot: bad signature (index out of range or not strictly ascending)");
                return false;
            }
        }

        if (!r.Read(fa.rows)) {
            LUTUM_ERROR("snapshot: truncated archetype");
            return false;
        }

        fa.entities = r.View(static_cast<size_t>(fa.rows) * sizeof(Entity));
        if (!fa.entities) {
            LUTUM_ERROR("snapshot: truncated entity list");
            return false;
        }
        for (uint32_t row = 0; row < fa.rows; ++row) {
            Entity e = INVALID_ENTITY;
            std::memcpy(&e, fa.entities + static_cast<size_t>(row) * sizeof(Entity), sizeof(Entity));
            const uint32_t index = EntityTraits::Index(e);
            if (index >= directoryCount || claimed[index]) {
                LUTUM_ERROR("snapshot: entity index {} out of range or duplicated", index);
                return false;
            }
            claimed[index] = true;
        }
        aliveRows += fa.rows;

        fa.columns.resize(sigCount, nullptr);
        for (size_t i = 0; i < sigCount; ++i) {
            const uint32_t colSize = types[fa.sig[i]].size;
            if (colSize == 0)
                continue; // tags: no bytes
            fa.columns[i] = r.View(static_cast<size_t>(fa.rows) * colSize);
            if (!fa.columns[i]) {
                LUTUM_ERROR("snapshot: truncated component column '{}'", types[fa.sig[i]].name);
                return false;
            }
        }
    }

    if (r.pos != r.size) {
        LUTUM_ERROR("snapshot: {} trailing bytes", r.size - r.pos);
        return false;
    }
    if (aliveRows + freeCount != directoryCount) {
        LUTUM_ERROR("snapshot: directory mismatch ({} alive + {} free != {})",
                    aliveRows, freeCount, directoryCount);
        return false;
    }

    // commit

    m_directory.resize(directoryCount);
    m_freeIndices.reserve(freeCount);
    for (const auto& [index, generation] : freeList) {
        m_directory[index].generation = generation;
        m_freeIndices.push_back(index);
    }

    for (const FileArch& fa : arches) {
        std::vector<ComponentID> runtimeSig;
        runtimeSig.reserve(fa.sig.size());
        for (uint32_t idx : fa.sig) {
            if (types[idx].cid)
                runtimeSig.push_back(*types[idx].cid);
        }
        std::sort(runtimeSig.begin(), runtimeSig.end());

        // NOTE: dropping unknown components can merge two file archetypes into one runtime archetype
        Archetype* arch = FindOrCreateArchetype(std::move(runtimeSig));

        for (uint32_t row = 0; row < fa.rows; ++row) {
            Entity e = INVALID_ENTITY;
            std::memcpy(&e, fa.entities + static_cast<size_t>(row) * sizeof(Entity), sizeof(Entity));

            auto [slabIndex, rowIndex] = arch->AllocateRow(e);
            EntityRecord& record = m_directory[EntityTraits::Index(e)];
            record.archetype = arch;
            record.slabIndex = slabIndex;
            record.rowIndex = rowIndex;
            record.generation = EntityTraits::Generation(e);

            for (size_t i = 0; i < fa.sig.size(); ++i) {
                const FileType& t = types[fa.sig[i]];
                if (!t.cid || t.size == 0)
                    continue;
                arch->WriteComponent(*t.cid, arch->GetSlab(slabIndex), rowIndex,
                                     fa.columns[i] + static_cast<size_t>(row) * t.size);
            }
        }
    }

    return true;
}

} // Lutum::Curia