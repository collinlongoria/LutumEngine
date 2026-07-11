/*
* File: RenderPools.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/11/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/RenderPools.hpp"

#include "Lutum/Assets/AssetRegistry.hpp"
#include "Lutum/Assets/MeshAsset.hpp"
#include "Lutum/Assets/TextureAsset.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/Graphics/GpuUploader.hpp"
#include "Lutum/Graphics/ImageIO.hpp"

namespace Lutum {
namespace {

    const Assets::AssetInfo* FindChecked(AssetID id, StableKey expectedType, const char* poolName) {
        const Assets::AssetInfo* info = Assets::Find(id);
        if (!info) {
            LUTUM_WARN("{}: unknown asset {:#018x}", poolName, id.value);
            return nullptr;
        }
        if (info->typeKey != expectedType) {
            LUTUM_WARN("{}: '{}' is not the expected asset type", poolName, info->virtualPath);
            return nullptr;
        }
        return info;
    }

} // anonymous namespace

void MeshPool::Initialize(GraphicsDevice& device) { m_device = &device; }
void MeshPool::Clear() { m_meshes.clear(); }

const GpuMesh* MeshPool::Get(AssetID id) {
    if (id.IsNull() || !m_device)
        return nullptr;

    if (const auto it = m_meshes.find(id); it != m_meshes.end())
        return it->second ? &*it->second : nullptr;

    const auto [it, inserted] = m_meshes.emplace(id, LoadMesh(id));
    return it->second ? &*it->second : nullptr;
}

std::optional<GpuMesh> MeshPool::LoadMesh(AssetID id) const {
    const auto* info = FindChecked(id, MeshAsset::kTypeKey, "MeshPool");
    if (!info)
        return std::nullopt;

    const auto data = MeshAsset::Load(info->virtualPath);
    if (!data || data->indices.empty())
        return std::nullopt;

    const auto vertexBytes = static_cast<uint32_t>(data->vertices.size() * sizeof(MeshVertex));
    const auto indexBytes = static_cast<uint32_t>(data->indices.size() * sizeof(uint32_t));

    auto vbo = Buffer::Create(*m_device, BufferUsage::VERTEX, vertexBytes, info->name.c_str());
    auto ibo = Buffer::Create(*m_device, BufferUsage::INDEX, indexBytes, info->name.c_str());
    if (!vbo || !ibo)
        return std::nullopt;

    GpuUploader uploader(*m_device);
    if (!uploader.Begin() ||
        !uploader.Upload(*vbo, data->vertices.data(), vertexBytes) ||
        !uploader.Upload(*ibo, data->indices.data(), indexBytes) ||
        !uploader.End()) {
        LUTUM_ERROR("MeshPool: upload failed for '{}'", info->virtualPath);
        return std::nullopt;
    }

    GpuMesh mesh;
    mesh.vbo = std::move(*vbo);
    mesh.ibo = std::move(*ibo);
    mesh.indexCount = static_cast<uint32_t>(data->indices.size());
    LUTUM_INFO("MeshPool: loaded '{}' ({} indices)", info->virtualPath, mesh.indexCount);
    return mesh;
}

void TexturePool::Initialize(GraphicsDevice& device) { m_device = &device; }
void TexturePool::Clear() { m_textures.clear(); }

const Texture* TexturePool::Get(AssetID id) {
    if (id.IsNull() || !m_device)
        return nullptr;

    if (const auto it = m_textures.find(id); it != m_textures.end())
        return it->second ? &*it->second : nullptr;

    const auto [it, inserted] = m_textures.emplace(id, LoadTexture(id));
    return it->second ? &*it->second : nullptr;
}

std::optional<Texture> TexturePool::LoadTexture(AssetID id) const {
    const auto* info = FindChecked(id, TextureAsset::kTypeKey, "TexturePool");
    if (!info)
        return std::nullopt;

    const auto sourceBytes = TextureAsset::Load(info->virtualPath);
    if (!sourceBytes)
        return std::nullopt;

    const auto image = ImageIO::LoadFromMemory(*sourceBytes);
    if (!image)
        return std::nullopt;

    Texture::CreateInfo createInfo = {};
    createInfo.width = image->width;
    createInfo.height = image->height;
    auto texture = Texture::Create(*m_device, createInfo, info->name.c_str());
    if (!texture)
        return std::nullopt;

    GpuUploader uploader(*m_device);
    if (!uploader.Begin() ||
        !uploader.Upload(*texture, image->pixels.data(), static_cast<uint32_t>(image->pixels.size())) ||
        !uploader.End()) {
        LUTUM_ERROR("TexturePool: upload failed for '{}'", info->virtualPath);
        return std::nullopt;
    }

    LUTUM_INFO("TexturePool: loaded '{}' ({}x{})", info->virtualPath, image->width, image->height);
    return texture;
}

void MaterialPool::Clear() { m_materials.clear(); }

const MaterialData* MaterialPool::Get(AssetID id) {
    if (id.IsNull())
        return nullptr;

    if (const auto it = m_materials.find(id); it != m_materials.end())
        return it->second ? &*it->second : nullptr;

    std::optional<MaterialData> loaded;
    if (const auto* info = FindChecked(id, MaterialAsset::kTypeKey, "MaterialPool"))
        loaded = MaterialAsset::Load(info->virtualPath);

    const auto [it, inserted] = m_materials.emplace(id, std::move(loaded));
    return it->second ? &*it->second : nullptr;
}

} // Lutum