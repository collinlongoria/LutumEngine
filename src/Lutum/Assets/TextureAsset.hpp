/*
* File: TextureAsset.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_TEXTUREASSET_HPP
#define LUTUM_TEXTUREASSET_HPP
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"

namespace Lutum::TextureAsset {

inline constexpr StableKey kTypeKey = HashName("Texture");
inline constexpr uint32_t kPayloadVersion = 1;

// full .lasset file bytes
[[nodiscard]]
std::vector<uint8_t> Encode(std::span<const uint8_t> sourceImageBytes, AssetID id);

// source image bytes
[[nodiscard]]
std::optional<std::vector<uint8_t>> Decode(std::span<const uint8_t> fileBytes);

[[nodiscard]]
std::optional<std::vector<uint8_t>> Load(std::string_view virtualPath);

} // Lutum::TextureAsset

#endif //LUTUM_TEXTUREASSET_HPP
