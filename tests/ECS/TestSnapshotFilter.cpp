/*
* File: TestSnapshotFilter.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/12/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <doctest/doctest.h>

#include <array>
#include <span>
#include <vector>

#include "Lutum/ECS/Registry.hpp"

using namespace Lutum;
using namespace Lutum::Curia;

namespace {

// Test-local components. Registration is GLOBAL across the test binary
// (name-hash keyed), so these names must stay unique to this file.
struct SnapFilterData {
    static constexpr const char* kCuriaName = "SnapFilterData";
    int value = 0;
};

struct SnapFilterTag {
    static constexpr const char* kCuriaName = "SnapFilterTag";
};

} // anonymous namespace

TEST_CASE("Snapshot: filtered save frees tagged entities at generation+1") {
    Registry registry;

    const Entity plain = registry.Create();
    registry.Add(plain, SnapFilterData{41});

    const Entity tagged = registry.Create();
    registry.Add(tagged, SnapFilterData{99});
    registry.Add(tagged, SnapFilterTag{});

    const std::array<StableKey, 1> exclude = {HashName("SnapFilterTag")};
    const std::vector<uint8_t> filtered = registry.SaveSnapshot(exclude);

    // Filtering is a save-time view; the source registry is untouched
    CHECK(registry.Alive(tagged));

    Registry loaded;
    REQUIRE(loaded.LoadSnapshot(filtered));

    // Plain entity survives, data intact; tagged entity does not exist
    REQUIRE(loaded.Alive(plain));
    const SnapFilterData* data = loaded.Get<SnapFilterData>(plain);
    REQUIRE(data != nullptr);
    CHECK(data->value == 41);
    CHECK(!loaded.Alive(tagged));

    // The freed slot recycles at generation+1, so a serialized EntityRef to the
    // stripped entity can never alias the slot's next occupant. Exactly one
    // slot is free here, so Create must take it regardless of free-list order.
    const Entity recycled = loaded.Create();
    CHECK(EntityTraits::Index(recycled) == EntityTraits::Index(tagged));
    CHECK(EntityTraits::Generation(recycled) == EntityTraits::Generation(tagged) + 1);

    // The no-filter overload and an empty filter are byte-identical
    CHECK(registry.SaveSnapshot() == registry.SaveSnapshot(std::span<const StableKey>{}));

    // Unknown exclusion keys are ignored: nothing is stripped
    const std::array<StableKey, 1> unknown = {HashName("KeyThatWasNeverRegistered")};
    Registry fromUnknown;
    REQUIRE(fromUnknown.LoadSnapshot(registry.SaveSnapshot(unknown)));
    CHECK(fromUnknown.Alive(tagged));
    CHECK(fromUnknown.Alive(plain));
}