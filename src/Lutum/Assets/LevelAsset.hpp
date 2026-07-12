/*
* File: LevelAsset.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/12/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_LEVELASSET_HPP
#define LUTUM_LEVELASSET_HPP
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"

namespace Lutum::LevelAsset {

// Payload = LREG snapshot blob VERBATIM
inline constexpr StableKey kTypeKey = HashName("Level");
inline constexpr uint32_t kPayloadVersion = 1;

[[nodiscard]]
std::vector<uint8_t> Encode(std::span<const uint8_t> snapshotBytes, AssetID id);

[[nodiscard]]
std::optional<std::vector<uint8_t>> Decode(std::span<const uint8_t> fileBytes);

[[nodiscard]]
std::optional<std::vector<uint8_t>> Load(std::string_view virtualPath);

} // Lutum::LevelAsset
#endif //LUTUM_LEVELASSET_HPP