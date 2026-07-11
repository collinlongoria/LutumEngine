/*
* File: MeshAsset.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_MESHASSET_HPP
#define LUTUM_MESHASSET_HPP
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"
#include "Lutum/Core/Math.hpp"

namespace Lutum {

struct MeshVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};
static_assert(sizeof(MeshVertex) == 32 && std::is_trivially_copyable_v<MeshVertex>, "MeshVertex layout is serialized raw");

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
};

namespace MeshAsset {

    // payload = u32 vertexCount + u32 indexCount + raw vertices + raw u32 indices
    inline constexpr StableKey kTypeKey = HashName("StaticMesh");
    inline constexpr uint32_t kPayloadVersion = 1;

    [[nodiscard]]
    std::vector<uint8_t> Encode(const MeshData& data, AssetID id);

    [[nodiscard]]
    std::optional<MeshData> Decode(std::span<const uint8_t> fileBytes);

    [[nodiscard]]
    std::optional<MeshData> Load(std::string_view virtualPath);

} // MeshAsset
} // Lutum
#endif //LUTUM_MESHASSET_HPP
