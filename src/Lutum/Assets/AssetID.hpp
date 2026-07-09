/*
* File: AssetID.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ASSETID_HPP
#define LUTUM_ASSETID_HPP
#include <cstdint>
#include <functional>
#include <random>

namespace Lutum {
// immutable random identity for an asset
// 0 is reserved as null ID
struct AssetID {
    uint64_t value = 0;

    [[nodiscard]]
    constexpr bool IsNull() const { return value == 0; }

    constexpr bool operator==(const AssetID&) const = default;
    constexpr auto operator<=>(const AssetID&) const = default;
};

inline constexpr AssetID kNullAssetID{};

// TODO: main-thread only; not thread safe
[[nodiscard]]
inline AssetID GenerateAssetID() {
    static std::mt19937_64 s_rng{[] {
        std::random_device rd;
        return (static_cast<uint64_t>(rd()) << 32) ^ rd();
    }()};

    uint64_t v = 0;
    while (v == 0) // 0 is reserved for null
        v = s_rng();
    return AssetID{v};
}

} // Lutum

template <>
struct std::hash<Lutum::AssetID> {
    size_t operator()(const Lutum::AssetID& id) const noexcept {
        return std::hash<uint64_t>{}(id.value);
    }
};

#endif //LUTUM_ASSETID_HPP
