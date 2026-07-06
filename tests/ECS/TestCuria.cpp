/*
* File: TestCuria.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <doctest/doctest.h>

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <array>
#include <cstddef>

#include "Lutum/Core/Jobs.hpp"
#include "Lutum/ECS/CommandBuffer.hpp"
#include "Lutum/ECS/Query.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Scheduler.hpp"

namespace {

using namespace Lutum::Curia;

struct Position {
    static constexpr const char* kCuriaName = "Test.Position";
    float x, y, z;
};

struct Velocity {
    static constexpr const char* kCuriaName = "Test.Velocity";
    float x, y, z;
};

struct Lifetime {
    static constexpr const char* kCuriaName = "Test.Lifetime";
    float seconds;
};

struct AlignedThing {
    static constexpr const char* kCuriaName = "Test.AlignedThing";
    alignas(16) float v[4];
};

struct Small {
    static constexpr const char* kCuriaName = "Test.Small";
    uint8_t b;
};

struct DeadTag {
    static constexpr const char* kCuriaName = "Test.DeadTag";
};

struct Frozen {
    static constexpr const char* kCuriaName = "Test.Frozen";
};

struct MeshHandle {
    static constexpr const char* kCuriaName = "Test.MeshHandle";
    uint32_t id;
};

struct Reflected {
    static constexpr const char* kCuriaName = "Test.Reflected";

    float x;
    uint16_t id;
    bool active;
    float arr[3];
    Entity target;

    static constexpr auto CuriaFields() {
        return std::array{
            FieldInfo{"x",      offsetof(Reflected, x),      FieldType::F32,       1},
            FieldInfo{"id",     offsetof(Reflected, id),     FieldType::U16,       1},
            FieldInfo{"active", offsetof(Reflected, active), FieldType::Bool,      1},
            FieldInfo{"arr",    offsetof(Reflected, arr),    FieldType::F32,       3},
            FieldInfo{"target", offsetof(Reflected, target), FieldType::EntityRef, 1},
        };
    }
};

struct CuriaJobsFixture {
    CuriaJobsFixture() {
        Lutum::Jobs::Initialize();
    }

    ~CuriaJobsFixture() {
        Lutum::Jobs::Shutdown();
    }
};

void PopulateMovementRegistry(Registry& registry) {
    for (int i = 0; i < 10'000; ++i) {
        Entity e = registry.Create();

        registry.Add(e, Position{static_cast<float>(i), 0, 0});
        registry.Add(e, Velocity{1, 0, 0});

        if (i % 2 == 0) {
            registry.Add<Frozen>(e);
        }

        if (i % 3 == 0) {
            registry.Add(e, MeshHandle{static_cast<uint32_t>(i)});
        }
    }
}

uint32_t CountFrozenPositions(Registry& registry) {
    Query<Position, With<Frozen>> frozenQuery;
    frozenQuery.Refresh(registry);

    uint32_t frozenCount = 0;
    frozenQuery.Each([&](Position&) {
        ++frozenCount;
    });

    return frozenCount;
}

uint64_t ExpectedMeshHandleSum() {
    uint64_t expectedSum = 0;

    for (int i = 0; i < 10'000; ++i) {
        if (i % 3 == 0) {
            expectedSum += static_cast<uint64_t>(i);
        }
    }

    return expectedSum;
}

} // anonymous namespace

TEST_CASE("Curia Registry supports CRUD and generation recycling") {
    Registry registry;

    Entity a = registry.Create();

    registry.Add(a, Position{1, 2, 3});
    registry.Add(a, Velocity{0, 1, 0});

    REQUIRE(registry.Get<Position>(a) != nullptr);
    CHECK(registry.Get<Position>(a)->y == 2.0f);

    registry.Remove<Velocity>(a);

    CHECK_FALSE(registry.Has<Velocity>(a));
    CHECK(registry.Has<Position>(a));

    registry.Destroy(a);

    Entity b = registry.Create();

    CHECK(EntityTraits::Index(b) == EntityTraits::Index(a));
    CHECK_FALSE(registry.Alive(a));
    CHECK(registry.Alive(b));
}

TEST_CASE("Curia Registry preserves alignment for aligned components") {
    Registry registry;

    for (int i = 0; i < 3000; ++i) {
        Entity e = registry.Create();

        registry.Add(e, Small{static_cast<uint8_t>(i)});
        registry.Add(e, AlignedThing{{static_cast<float>(i), 0, 0, 0}});

        auto* aligned = registry.Get<AlignedThing>(e);

        REQUIRE(aligned != nullptr);
        CHECK(reinterpret_cast<uintptr_t>(aligned) % alignof(AlignedThing) == 0);
    }
}

TEST_CASE("Curia Registry remove observer fires during destroy while component is readable") {
    Registry registry;

    bool observerFired = false;
    float observedX = 0.0f;

    registry.ObserveRemove<Position>([&](Entity e) {
        observerFired = true;

        auto* position = registry.Get<Position>(e);
        REQUIRE(position != nullptr);

        observedX = position->x;
    });

    Entity c = registry.Create();

    registry.Add(c, Position{9, 9, 9});
    registry.Add(c, DeadTag{});

    registry.Destroy(c);

    CHECK(observerFired);
    CHECK(observedX == 9.0f);
    CHECK_FALSE(registry.Alive(c));
}

TEST_CASE("Curia Registry supports filtered queries") {
    Registry registry;
    PopulateMovementRegistry(registry);

    Query<Position, Velocity, Without<Frozen>> moveQuery;
    moveQuery.Refresh(registry);

    uint32_t movedCount = 0;
    moveQuery.Each([&](Position& p, Velocity& v) {
        p.x += v.x;
        ++movedCount;
    });

    CHECK(movedCount == 5000);
    CHECK(CountFrozenPositions(registry) == 5000);
}

TEST_CASE_FIXTURE(CuriaJobsFixture, "Curia Registry supports parallel query iteration") {
    Registry registry;
    PopulateMovementRegistry(registry);

    std::atomic<uint64_t> parSum{0};

    Query<MeshHandle> meshQuery;
    meshQuery.Refresh(registry);

    meshQuery.ParEach([&](MeshHandle& m) {
        parSum.fetch_add(m.id, std::memory_order_relaxed);
    });

    CHECK(parSum.load(std::memory_order_relaxed) == ExpectedMeshHandleSum());
}

TEST_CASE("Curia Registry supports deferred command buffer changes") {
    Registry registry;
    PopulateMovementRegistry(registry);

    Query<Position, Velocity, Without<Frozen>> moveQuery;
    moveQuery.Refresh(registry);

    CommandBuffer cmd;

    Entity newbie = cmd.CreateDeferred();

    cmd.Add(newbie, Position{-1, -1, -1});
    cmd.Add<Frozen>(newbie);

    uint32_t destroyRequests = 0;
    moveQuery.EachWithEntity([&](Entity e, Position& p, Velocity&) {
        if (p.x > 9990.0f) {
            cmd.Destroy(e);
            ++destroyRequests;
        }
    });

    CHECK(destroyRequests == 5);

    const size_t beforeExecute = registry.EntityCount();
    cmd.Execute(registry);

    CHECK(cmd.Empty());
    CHECK(registry.EntityCount() == beforeExecute + 1 - destroyRequests);
    CHECK(CountFrozenPositions(registry) == 5001);
}

TEST_CASE_FIXTURE(CuriaJobsFixture, "Curia Scheduler phases systems by dependencies and executes deferred destroys") {
    Registry registry;
    Scheduler scheduler(registry);

    for (int i = 0; i < 10'000; ++i) {
        Entity e = registry.Create();

        registry.Add(e, Position{0, 0, 0});
        registry.Add(e, Velocity{1, 0, 0});
        registry.Add(e, Lifetime{static_cast<float>(i % 100) * 0.01f});
    }

    Query<Position, Velocity> moveQuery;
    Query<Lifetime> lifeQuery;
    Query<Velocity> dampQuery;

    uint32_t movementRuns = 0;
    uint32_t lifetimeRuns = 0;
    uint32_t dampingRuns = 0;
    uint32_t postDampingRuns = 0;

    scheduler.AddSystem("Movement")
        .Reads<Velocity>()
        .Writes<Position>()
        .Execute([&](Registry& r, CommandBuffer&) {
            ++movementRuns;

            moveQuery.Refresh(r);
            moveQuery.Each([](Position& p, Velocity& v) {
                p.x += v.x * 0.016f;
            });
        });

    scheduler.AddSystem("LifetimeTick")
        .Writes<Lifetime>()
        .Execute([&](Registry& r, CommandBuffer& cmd) {
            ++lifetimeRuns;

            lifeQuery.Refresh(r);
            lifeQuery.EachWithEntity([&](Entity e, Lifetime& l) {
                l.seconds -= 0.016f;

                if (l.seconds <= 0.0f) {
                    cmd.Destroy(e);
                }
            });
        });

    scheduler.AddSystem("Damping")
        .Writes<Velocity>()
        .Execute([&](Registry& r, CommandBuffer&) {
            ++dampingRuns;

            dampQuery.Refresh(r);
            dampQuery.Each([](Velocity& v) {
                v.x *= 0.99f;
            });
        });

    scheduler.AddSystem("PostDamping")
        .After("Damping")
        .Execute([&](Registry&, CommandBuffer&) {
            ++postDampingRuns;
        });

    scheduler.Compile();

    CHECK(scheduler.PhaseCount() == 3);

    CHECK(scheduler.PhaseOf("Movement") == 0);
    CHECK(scheduler.PhaseOf("LifetimeTick") == 0);
    CHECK(scheduler.PhaseOf("Damping") == 1);
    CHECK(scheduler.PhaseOf("PostDamping") == 2);

    CHECK(scheduler.PhaseSize(0) == 2);
    CHECK(scheduler.PhaseSize(1) == 1);
    CHECK(scheduler.PhaseSize(2) == 1);

    CHECK(scheduler.PhaseContains(0, "Movement"));
    CHECK(scheduler.PhaseContains(0, "LifetimeTick"));
    CHECK(scheduler.PhaseContains(1, "Damping"));
    CHECK(scheduler.PhaseContains(2, "PostDamping"));

    scheduler.LogGraph();

    const size_t before = registry.EntityCount();

    for (int frame = 0; frame < 120; ++frame) {
        scheduler.Run();
    }

    const size_t after = registry.EntityCount();

    CHECK(before == 10'000);
    CHECK(after == 0);

    CHECK(movementRuns == 120);
    CHECK(lifetimeRuns == 120);
    CHECK(dampingRuns == 120);
    CHECK(postDampingRuns == 120);
}

TEST_CASE("stable keys: idempotent registration and lookup") {
    const ComponentID a = ComponentType<Position>::Id();
    CHECK(ComponentType<Position>::Id() == a);

    // Key is compile-time and matches the declared name
    static_assert(ComponentType<Position>::Key() == HashName("Test.Position"));

    // Direct re-register by key is idempotent
    CHECK(ComponentRegistry::Register(HashName("Test.Position"), "Test.Position",
                                      sizeof(Position), alignof(Position)) == a);

    // Lookup by key
    const auto found = ComponentRegistry::FindByKey(HashName("Test.Position"));
    REQUIRE(found.has_value());
    CHECK(*found == a);
    CHECK(ComponentRegistry::Get(a).name == "Test.Position");
    CHECK(ComponentRegistry::Get(a).size == sizeof(Position));
    CHECK(ComponentRegistry::Get(a).key == HashName("Test.Position"));

    // Unknown keys are tolerated, not fatal — the loader depends on this
    CHECK_FALSE(ComponentRegistry::FindByKey(HashName("Test.DoesNotExist")).has_value());

    // Tags register with zero size
    const ComponentID tag = ComponentType<DeadTag>::Id();
    CHECK(ComponentRegistry::Get(tag).size == 0);

    // Resources: same mechanism
    const ResourceID r = ResourceType<Position>::Id(); // any named class works
    CHECK(ResourceType<Position>::Id() == r);
    CHECK(ResourceRegistry::Name(r) == "Test.Position");
}

namespace {

struct ByteReader {
    const std::vector<uint8_t>& buf;
    size_t pos = 0;

    template <typename T>
    T Read() {
        T v{};
        REQUIRE(pos + sizeof(T) <= buf.size());
        std::memcpy(&v, buf.data() + pos, sizeof(T));
        pos += sizeof(T);
        return v;
    }
    void Skip(size_t n) {
        REQUIRE(pos + n <= buf.size());
        pos += n;
    }
    std::string ReadString(size_t n) {
        REQUIRE(pos + n <= buf.size());
        std::string s(reinterpret_cast<const char*>(buf.data() + pos), n);
        pos += n;
        return s;
    }
};

} // anonymous namespace

TEST_CASE("snapshot writer: header, determinism, full-buffer walk") {
    Registry r;

    Entity a = r.Create();
    r.Add(a, Position{1, 2, 3});
    r.Add(a, Velocity{4, 5, 6});

    Entity b = r.Create();
    r.Add(b, Position{7, 8, 9});
    r.Add<DeadTag>(b);

    Entity bare = r.Create();            // stays in the empty archetype
    Entity doomed = r.Create();
    r.Destroy(doomed);                   // becomes a free-list entry

    const std::vector<uint8_t> bytes = r.SaveSnapshot();
    CHECK(r.SaveSnapshot() == bytes);    // canonical: same state, same bytes

    ByteReader rd{bytes};

    // Header
    CHECK(rd.ReadString(4) == "LREG");
    CHECK(rd.Read<uint32_t>() == 1);

    // Type table: Position, Velocity, DeadTag (key-sorted; order not asserted)
    const uint32_t typeCount = rd.Read<uint32_t>();
    CHECK(typeCount == 3);

    struct FileType { uint64_t key; uint32_t size; std::string name; };
    std::vector<FileType> types;
    uint64_t prevKey = 0;
    for (uint32_t i = 0; i < typeCount; ++i) {
        FileType t;
        t.key = rd.Read<uint64_t>();
        t.size = rd.Read<uint32_t>();
        t.name = rd.ReadString(rd.Read<uint16_t>());
        CHECK(t.key > prevKey);          // strictly ascending
        prevKey = t.key;
        CHECK(t.key == HashName(t.name.c_str()));
        types.push_back(std::move(t));
    }
    const auto findType = [&](const char* name) -> const FileType* {
        for (const FileType& t : types)
            if (t.name == name) return &t;
        return nullptr;
    };
    REQUIRE(findType("Test.Position"));
    CHECK(findType("Test.Position")->size == sizeof(Position));
    REQUIRE(findType("Test.Velocity"));
    CHECK(findType("Test.Velocity")->size == sizeof(Velocity));
    REQUIRE(findType("Test.DeadTag"));
    CHECK(findType("Test.DeadTag")->size == 0);

    // Directory
    CHECK(rd.Read<uint32_t>() == 4);     // directory size: a, b, bare, doomed
    const uint32_t freeCount = rd.Read<uint32_t>();
    REQUIRE(freeCount == 1);
    CHECK(rd.Read<uint32_t>() == EntityTraits::Index(doomed));
    CHECK(rd.Read<uint32_t>() == EntityTraits::Generation(doomed) + 1);

    // Archetypes: [] (bare), [Pos,Vel] (a), [Pos,DeadTag] (b) — 3 populated
    const uint32_t archCount = rd.Read<uint32_t>();
    CHECK(archCount == 3);

    size_t totalRows = 0;
    bool sawBare = false, sawA = false, sawB = false;
    for (uint32_t i = 0; i < archCount; ++i) {
        const uint32_t sigCount = rd.Read<uint32_t>();
        std::vector<uint32_t> sig(sigCount);
        for (auto& s : sig) {
            s = rd.Read<uint32_t>();
            REQUIRE(s < typeCount);
        }

        const uint32_t rows = rd.Read<uint32_t>();
        totalRows += rows;

        std::vector<Entity> ents(rows);
        for (auto& e : ents)
            e = rd.Read<uint64_t>();

        if (sigCount == 0) {
            sawBare = (rows == 1 && ents[0] == bare);
        }
        if (rows == 1 && ents[0] == a) sawA = true;
        if (rows == 1 && ents[0] == b) sawB = true;

        // Skip packed columns using file-declared sizes
        for (uint32_t s : sig)
            rd.Skip(static_cast<size_t>(rows) * types[s].size);
    }
    CHECK(totalRows == 3);
    CHECK(sawBare);
    CHECK(sawA);
    CHECK(sawB);

    // The walk must consume the buffer exactly
    CHECK(rd.pos == bytes.size());
}

namespace {

// Byte offsets of a type-table entry's key and size fields
std::pair<size_t, size_t> FindTypeEntryOffsets(const std::vector<uint8_t>& buf,
                                               std::string_view name) {
    ByteReader rd{buf};
    rd.Skip(8); // magic + version
    const uint32_t typeCount = rd.Read<uint32_t>();
    for (uint32_t i = 0; i < typeCount; ++i) {
        const size_t keyOff = rd.pos;
        rd.Read<uint64_t>();
        const size_t sizeOff = rd.pos;
        rd.Read<uint32_t>();
        const uint16_t nameLen = rd.Read<uint16_t>();
        if (rd.ReadString(nameLen) == name)
            return {keyOff, sizeOff};
    }
    REQUIRE_MESSAGE(false, "type not found in snapshot");
    return {0, 0};
}

} // anonymous namespace

TEST_CASE("snapshot round-trip: multi-slab, tags, free list, byte identity") {
    Registry src;

    // AlignedThing is 16B -> 1024 rows/slab; 1500 entities forces multiple slabs
    std::vector<Entity> ents;
    ents.reserve(1500);
    for (int i = 0; i < 1500; ++i) {
        Entity e = src.Create();
        src.Add(e, AlignedThing{{static_cast<float>(i), 0.5f, 0.0f, 0.0f}});
        if (i % 3 == 0) src.Add(e, Position{static_cast<float>(i), 0, 0});
        if (i % 5 == 0) src.Add<Frozen>(e);
        ents.push_back(e);
    }
    Entity bare = src.Create();          // component-less, must survive
    const Entity dead1 = ents[10];
    const Entity dead2 = ents[20];
    src.Destroy(dead1);
    src.Destroy(dead2);

    const std::vector<uint8_t> bytes = src.SaveSnapshot();

    Registry dst;
    REQUIRE(dst.LoadSnapshot(bytes));

    // Byte-identical resave BEFORE any mutation
    CHECK(dst.SaveSnapshot() == bytes);

    CHECK(dst.EntityCount() == src.EntityCount());
    CHECK(dst.Alive(bare));
    CHECK_FALSE(dst.Alive(dead1));
    CHECK_FALSE(dst.Alive(dead2));

    for (int i = 0; i < 1500; ++i) {
        if (i == 10 || i == 20)
            continue;
        const Entity e = ents[i];
        REQUIRE(dst.Alive(e));
        const AlignedThing* at = dst.Get<AlignedThing>(e);
        REQUIRE(at != nullptr);
        CHECK(at->v[0] == static_cast<float>(i));
        CHECK(dst.Has<Position>(e) == (i % 3 == 0));
        CHECK(dst.Has<Frozen>(e) == (i % 5 == 0));
        if (i % 3 == 0)
            CHECK(dst.Get<Position>(e)->x == static_cast<float>(i));
    }

    // Free list restored in order: Create() pops dead2's index first (LIFO),
    // with the post-destroy generation
    const Entity n1 = dst.Create();
    CHECK(EntityTraits::Index(n1) == EntityTraits::Index(dead2));
    CHECK(EntityTraits::Generation(n1) == EntityTraits::Generation(dead2) + 1);
}

TEST_CASE("snapshot load: unknown component dropped, archetypes merge") {
    Registry src;
    Entity e1 = src.Create();
    src.Add(e1, Position{1, 2, 3});
    src.Add(e1, Velocity{4, 5, 6});
    Entity e2 = src.Create();
    src.Add(e2, Position{7, 8, 9});    // already [Position]-only

    std::vector<uint8_t> bytes = src.SaveSnapshot();

    // Make Velocity's key unknown to this binary
    const auto [keyOff, sizeOff] = FindTypeEntryOffsets(bytes, "Test.Velocity");
    const uint64_t bogus = 0xDEADBEEFCAFEF00DULL;
    std::memcpy(bytes.data() + keyOff, &bogus, sizeof(bogus));

    Registry dst;
    REQUIRE(dst.LoadSnapshot(bytes)); // warns, does not fail

    // e1's [Pos,Vel] collapsed to [Pos] and merged with e2's archetype
    REQUIRE(dst.Alive(e1));
    REQUIRE(dst.Alive(e2));
    CHECK(dst.Has<Position>(e1));
    CHECK_FALSE(dst.Has<Velocity>(e1));
    CHECK(dst.Get<Position>(e1)->y == 2);
    CHECK(dst.Get<Position>(e2)->z == 9);
    CHECK(dst.ArchetypeCount() == 2); // empty + [Position]
}

TEST_CASE("snapshot load: failures leave the registry untouched") {
    Registry src;
    Entity e = src.Create();
    src.Add(e, Position{1, 2, 3});
    const std::vector<uint8_t> good = src.SaveSnapshot();

    SUBCASE("layout size mismatch") {
        std::vector<uint8_t> bad = good;
        const auto [keyOff, sizeOff] = FindTypeEntryOffsets(bad, "Test.Position");
        const uint32_t wrongSize = sizeof(Position) + 4;
        std::memcpy(bad.data() + sizeOff, &wrongSize, sizeof(wrongSize));

        Registry dst;
        CHECK_FALSE(dst.LoadSnapshot(bad));
        CHECK(dst.EntityCount() == 0);
        CHECK(dst.Alive(dst.Create())); // still usable
    }
    SUBCASE("bad magic") {
        std::vector<uint8_t> bad = good;
        bad[0] = 'X';
        Registry dst;
        CHECK_FALSE(dst.LoadSnapshot(bad));
        CHECK(dst.EntityCount() == 0);
    }
    SUBCASE("bad version") {
        std::vector<uint8_t> bad = good;
        const uint32_t v = 999;
        std::memcpy(bad.data() + 4, &v, sizeof(v));
        Registry dst;
        CHECK_FALSE(dst.LoadSnapshot(bad));
    }
    SUBCASE("truncated") {
        std::vector<uint8_t> bad = good;
        bad.pop_back();
        Registry dst;
        CHECK_FALSE(dst.LoadSnapshot(bad));
        CHECK(dst.EntityCount() == 0);
    }
    SUBCASE("trailing bytes") {
        std::vector<uint8_t> bad = good;
        bad.push_back(0);
        Registry dst;
        CHECK_FALSE(dst.LoadSnapshot(bad));
    }
}

TEST_CASE("Clear: observers fire, archetypes survive, queries stay valid") {
    Registry r;

    int removals = 0;
    r.ObserveRemove<Position>([&](Entity) { ++removals; });

    Query<Position> q;
    for (int i = 0; i < 10; ++i) {
        Entity e = r.Create();
        r.Add(e, Position{static_cast<float>(i), 0, 0});
        if (i % 2 == 0) r.Add(e, Velocity{1, 1, 1});
    }
    q.Refresh(r);
    const size_t archCountBefore = r.ArchetypeCount();

    r.Clear();

    CHECK(removals == 10);
    CHECK(r.EntityCount() == 0);
    CHECK(r.ArchetypeCount() == archCountBefore); // emptied, not destroyed

    int visited = 0;
    q.Each([&](Position&) { ++visited; });
    CHECK(visited == 0);

    // Repopulate: the SAME cached archetypes refill; no Refresh needed
    Entity e = r.Create();
    CHECK(EntityTraits::Index(e) == 0);           // directory restarted
    r.Add(e, Position{42, 0, 0});
    q.Each([&](Position& p) { ++visited; CHECK(p.x == 42); });
    CHECK(visited == 1);
}

TEST_CASE("Clear + LoadSnapshot: load-over-live round trip") {
    Registry r;

    Entity a = r.Create();
    r.Add(a, Position{1, 2, 3});
    r.Add<Frozen>(a);
    Entity b = r.Create();
    r.Add(b, Velocity{4, 5, 6});
    const std::vector<uint8_t> bytes = r.SaveSnapshot();

    // Mutate past the save point, then load back over the live registry
    r.Destroy(b);
    Entity c = r.Create();
    r.Add(c, Position{9, 9, 9});

    r.Clear();
    REQUIRE(r.LoadSnapshot(bytes));

    CHECK(r.SaveSnapshot() == bytes);             // byte-identical through Clear
    REQUIRE(r.Alive(a));
    REQUIRE(r.Alive(b));
    CHECK(r.Get<Position>(a)->z == 3);
    CHECK(r.Has<Frozen>(a));
    CHECK(r.Get<Velocity>(b)->x == 4);
}

TEST_CASE("reflection: field tables registered and validated") {
    const ComponentID cid = ComponentType<Reflected>::Id();
    const ComponentInfo& info = ComponentRegistry::Get(cid);

    REQUIRE(info.fields.size() == 5);
    CHECK(info.fields[0].name == std::string("x"));
    CHECK(info.fields[0].offset == offsetof(Reflected, x));
    CHECK(info.fields[3].type == FieldType::F32);
    CHECK(info.fields[3].count == 3);
    CHECK(info.fields[4].type == FieldType::EntityRef);
    CHECK(FieldTypeSize(info.fields[4].type) == sizeof(Entity));

    // Unreflected components have empty tables
    CHECK(ComponentRegistry::Get(ComponentType<Position>::Id()).fields.empty());
    CHECK(ComponentRegistry::Get(ComponentType<DeadTag>::Id()).fields.empty());
}

TEST_CASE("reflection: untyped access via ArchetypeOf + GetRaw") {
    Registry r;
    Entity target = r.Create();
    Entity e = r.Create();
    r.Add(e, Reflected{1.5f, 42, true, {7, 8, 9}, target});
    r.Add(e, Position{1, 2, 3});
    r.Add<DeadTag>(e);

    const Archetype* arch = r.ArchetypeOf(e);
    REQUIRE(arch != nullptr);
    CHECK(arch->Signature().size() == 3);

    const ComponentID cid = ComponentType<Reflected>::Id();
    void* raw = r.GetRaw(e, cid);
    REQUIRE(raw != nullptr);

    // Walk fields generically, the way the inspector will
    const ComponentInfo& info = ComponentRegistry::Get(cid);
    for (const FieldInfo& f : info.fields) {
        std::byte* fieldPtr = static_cast<std::byte*>(raw) + f.offset;
        if (f.name == std::string("x")) {
            float v;
            std::memcpy(&v, fieldPtr, sizeof(v));
            CHECK(v == 1.5f);
            const float newV = 99.0f; // write-through, like an edit widget
            std::memcpy(fieldPtr, &newV, sizeof(newV));
        }
        if (f.name == std::string("target")) {
            Entity t;
            std::memcpy(&t, fieldPtr, sizeof(t));
            CHECK(t == target);
        }
    }
    CHECK(r.Get<Reflected>(e)->x == 99.0f); // raw write visible through typed access

    CHECK(r.GetRaw(e, ComponentType<DeadTag>::Id()) == nullptr);   // tag
    CHECK(r.GetRaw(e, ComponentType<Velocity>::Id()) == nullptr);  // absent
    r.Destroy(e);
    CHECK(r.GetRaw(e, cid) == nullptr);                            // dead
    CHECK(r.ArchetypeOf(e) == nullptr);
}