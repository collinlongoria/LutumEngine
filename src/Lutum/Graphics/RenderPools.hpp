/*
* File: RenderPools.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/11/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_RENDERPOOLS_HPP
#define LUTUM_RENDERPOOLS_HPP
#include <optional>
#include <unordered_map>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Assets/MaterialAsset.hpp"
#include "Lutum/Graphics/Buffer.hpp"
#include "Lutum/Graphics/Texture.hpp"

namespace Lutum {

class GraphicsDevice;

struct GpuMesh {
    Buffer vbo;
    Buffer ibo;
    uint32_t indexCount = 0;
};

class MeshPool {
public:
    void Initialize(GraphicsDevice& device);
    void Clear();
    [[nodiscard]]
    const GpuMesh* Get(AssetID id);

private:
    [[nodiscard]]
    std::optional<GpuMesh> LoadMesh(AssetID id) const;

    GraphicsDevice* m_device = nullptr;
    std::unordered_map<AssetID, std::optional<GpuMesh>> m_meshes;
};

class TexturePool {
public:
    void Initialize(GraphicsDevice& device);
    void Clear();
    [[nodiscard]]
    const Texture* Get(AssetID id);

private:
    [[nodiscard]]
    std::optional<Texture> LoadTexture(AssetID id) const;

    GraphicsDevice* m_device = nullptr;
    std::unordered_map<AssetID, std::optional<Texture>> m_textures;
};

class MaterialPool {
public:
    void Clear();
    [[nodiscard]] const MaterialData* Get(AssetID id);

private:
    std::unordered_map<AssetID, std::optional<MaterialData>> m_materials;
};

} // Lutum
#endif //LUTUM_RENDERPOOLS_HPP
