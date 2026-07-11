/*
* File: MaterialAsset.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_MATERIALASSET_HPP
#define LUTUM_MATERIALASSET_HPP
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"

namespace Lutum {

struct MaterialData {
    AssetID albedo{};
};

namespace MaterialAsset {

    // payload = u64 albedo AssetID
    inline constexpr StableKey kTypeKey = HashName("Material");
    inline constexpr uint32_t kPayloadVersion = 1;

    [[nodiscard]]
    std::vector<uint8_t> Encode(const MaterialData& data, AssetID id);

    [[nodiscard]]
    std::optional<MaterialData> Decode(std::span<const uint8_t> fileBytes);

    [[nodiscard]]
    std::optional<MaterialData> Load(std::string_view virtualPath);

} // MaterialAsset
} // Lutum
#endif //LUTUM_MATERIALASSET_HPP
