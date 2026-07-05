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