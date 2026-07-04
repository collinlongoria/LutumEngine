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

#include "Lutum/ECS/Registry.hpp"

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