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

#include <cstdint>
#include <cstddef>
#include <atomic>

#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Query.hpp"
#include "Lutum/ECS/CommandBuffer.hpp"

namespace {

using namespace Lutum::Curia;

struct Position {
    float x, y, z;
};

struct Velocity {
    float x, y, z;
};

struct AlignedThing {
    alignas(16) float v[4];
};

struct Small {
    uint8_t b;
};

struct DeadTag {};

struct Frozen {};

struct MeshHandle {
    uint32_t id;
};

}

TEST_CASE("Curia World supports CRUD and generation recycling") {
    Registry world;

    Entity a = world.Create();

    world.Add(a, Position{1, 2, 3});
    world.Add(a, Velocity{0, 1, 0});

    REQUIRE(world.Get<Position>(a) != nullptr);
    CHECK(world.Get<Position>(a)->y == 2.0f);

    world.Remove<Velocity>(a);

    CHECK_FALSE(world.Has<Velocity>(a));
    CHECK(world.Has<Position>(a));

    world.Destroy(a);

    Entity b = world.Create();

    CHECK(EntityTraits::Index(b) == EntityTraits::Index(a));
    CHECK_FALSE(world.Alive(a));
    CHECK(world.Alive(b));
}

TEST_CASE("Curia World preserves alignment for aligned components") {
    Registry world;

    for (int i = 0; i < 3000; ++i) {
        Entity e = world.Create();

        world.Add(e, Small{static_cast<uint8_t>(i)});
        world.Add(e, AlignedThing{{static_cast<float>(i), 0, 0, 0}});

        auto* aligned = world.Get<AlignedThing>(e);

        REQUIRE(aligned != nullptr);
        CHECK(reinterpret_cast<uintptr_t>(aligned) % alignof(AlignedThing) == 0);
    }
}

TEST_CASE("Curia World remove observer fires during destroy while component is readable") {
    Registry world;

    bool observerFired = false;
    bool sawExpectedValue = false;

    world.ObserveRemove<Position>([&](Entity e) {
        observerFired = true;

        auto* position = world.Get<Position>(e);
        REQUIRE(position != nullptr);

        sawExpectedValue = position->x == 9.0f;
    });

    Entity c = world.Create();

    world.Add(c, Position{9, 9, 9});
    world.Add(c, DeadTag{});

    world.Destroy(c);

    CHECK(observerFired);
    CHECK(sawExpectedValue);
    CHECK_FALSE(world.Alive(c));
}

TEST_CASE("Curia Registry supports filtered queries") {
    Registry registry;

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

    Query<Position, Velocity, Without<Frozen>> moveQuery;
    moveQuery.Refresh(registry);

    moveQuery.Each([](Position& p, Velocity& v) {
        p.x += v.x;
    });

    Query<Position, With<Frozen>> frozenQuery;
    frozenQuery.Refresh(registry);

    uint32_t frozenCount = 0;

    frozenQuery.Each([&](Position&) {
        ++frozenCount;
    });

    CHECK(frozenCount == 5000);
}

TEST_CASE("Curia Registry supports parallel query iteration") {
    Registry registry;

    uint64_t expectedSum = 0;

    for (int i = 0; i < 10'000; ++i) {
        Entity e = registry.Create();

        registry.Add(e, Position{static_cast<float>(i), 0, 0});
        registry.Add(e, Velocity{1, 0, 0});

        if (i % 2 == 0) {
            registry.Add<Frozen>(e);
        }

        if (i % 3 == 0) {
            registry.Add(e, MeshHandle{static_cast<uint32_t>(i)});
            expectedSum += static_cast<uint64_t>(i);
        }
    }

    std::atomic<uint64_t> parSum{0};

    Query<MeshHandle> meshQuery;
    meshQuery.Refresh(registry);

    meshQuery.ParEach([&](MeshHandle& m) {
        parSum.fetch_add(m.id, std::memory_order_relaxed);
    });

    CHECK(parSum.load(std::memory_order_relaxed) == expectedSum);
}

TEST_CASE("Curia Registry supports deferred command buffer changes") {
    Registry registry;

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

    Query<Position, Velocity, Without<Frozen>> moveQuery;
    moveQuery.Refresh(registry);

    CommandBuffer cmd;

    Entity newbie = cmd.CreateDeferred();

    cmd.Add(newbie, Position{-1, -1, -1});
    cmd.Add<Frozen>(newbie);

    moveQuery.EachWithEntity([&](Entity e, Position& p, Velocity&) {
        if (p.x > 9990.0f) {
            cmd.Destroy(e);
        }
    });

    cmd.Execute(registry);

    CHECK(cmd.Empty());

    Query<Position, With<Frozen>> frozenQuery;
    frozenQuery.Refresh(registry);

    uint32_t frozenCount = 0;

    frozenQuery.Each([&](Position&) {
        ++frozenCount;
    });

    CHECK(frozenCount == 5001);
}