/*
* File: EngineContentGen.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/EngineContentGen.hpp"

#include <filesystem>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/MaterialAsset.hpp"
#include "Lutum/Assets/MeshAsset.hpp"
#include "Lutum/Assets/TextureAsset.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"

namespace fs = std::filesystem;

namespace Lutum {
namespace {

    // Reuse the UUID of an existing file so regeneration is identity-stable
    AssetID PreserveOrGenerateId(const fs::path& absolute) {
        std::ifstream file(absolute, std::ios::binary);
        if (file) {
            uint8_t header[AssetHeader::kSize] = {};
            file.read(reinterpret_cast<char*>(header), sizeof(header));
            if (file.gcount() == sizeof(header)) {
                if (const auto decoded = AssetHeader::Decode(header))
                    return decoded->id;
            }
        }
        return GenerateAssetID();
    }

    bool WriteFile(const fs::path& absolute, std::span<const uint8_t> bytes) {
        std::error_code ec;
        fs::create_directories(absolute.parent_path(), ec);
        std::ofstream file(absolute, std::ios::binary);
        if (!file) {
            LUTUM_ERROR("EngineContentGen: failed to open '{}'", absolute.string());
            return false;
        }
        file.write(reinterpret_cast<const char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
        return file.good();
    }

    MeshData BuildCube() {
        // 24 verts, per-face normals
        MeshData mesh;
        const Vec3 normals[6] = {
            { 0, 0, 1}, { 0, 0,-1}, { 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}
        };
        const Vec3 positions[6][4] = {
            {{-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}}, // +Z
            {{ 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}}, // -Z
            {{ 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}}, // +X
            {{-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}}, // -X
            {{-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}}, // +Y
            {{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}}, // -Y
        };
        const Vec2 uvs[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

        for (int face = 0; face < 6; ++face) {
            const auto base = static_cast<uint32_t>(mesh.vertices.size());
            for (int v = 0; v < 4; ++v)
                mesh.vertices.push_back({positions[face][v], normals[face], uvs[v]});
            for (const uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u})
                mesh.indices.push_back(base + i);
        }
        return mesh;
    }

    MeshData BuildPlane() {
        // 1x1 on XZ, +Y normal; matches the cube's top-face winding
        MeshData mesh;
        mesh.vertices = {
            {{-0.5f, 0, 0.5f}, {0, 1, 0}, {0, 1}}, {{ 0.5f, 0, 0.5f}, {0, 1, 0}, {1, 1}},
            {{ 0.5f, 0,-0.5f}, {0, 1, 0}, {1, 0}}, {{-0.5f, 0,-0.5f}, {0, 1, 0}, {0, 0}},
        };
        mesh.indices = {0, 1, 2, 0, 2, 3};
        return mesh;
    }

    std::vector<uint8_t> BuildCheckerPng() {
        constexpr int kSize = 64, kSquare = 8;
        std::vector<uint8_t> pixels(kSize * kSize * 4);
        for (int y = 0; y < kSize; ++y) {
            for (int x = 0; x < kSize; ++x) {
                const bool light = ((x / kSquare) + (y / kSquare)) % 2 == 0;
                const uint8_t v = light ? 180 : 90;
                uint8_t* p = &pixels[(y * kSize + x) * 4];
                p[0] = p[1] = p[2] = v;
                p[3] = 255;
            }
        }

        std::vector<uint8_t> png;
        stbi_write_png_to_func(
            [](void* ctx, void* data, int size) {
                auto* out = static_cast<std::vector<uint8_t>*>(ctx);
                const auto* bytes = static_cast<uint8_t*>(data);
                out->insert(out->end(), bytes, bytes + size);
            },
            &png, kSize, kSize, 4, pixels.data(), kSize * 4);
        return png;
    }

} // anonymous namespace

bool GenerateEngineContent() {
    const std::string engineRoot = FileSystem::Resolve("/Engine/");
    if (engineRoot.empty()) {
        LUTUM_ERROR("EngineContentGen: /Engine/ not mounted");
        return false;
    }
    const fs::path root(engineRoot);

    const fs::path cubePath = root / "Primitives" / "Cube.lasset";
    const fs::path planePath = root / "Primitives" / "Plane.lasset";
    const fs::path texturePath = root / "Textures" / "DefaultTexture.lasset";
    const fs::path materialPath = root / "Materials" / "DefaultMaterial.lasset";

    const AssetID textureId = PreserveOrGenerateId(texturePath); // material references it
    const std::vector<uint8_t> png = BuildCheckerPng();
    if (png.empty()) {
        LUTUM_ERROR("EngineContentGen: PNG encode failed");
        return false;
    }

    bool ok = WriteFile(cubePath, MeshAsset::Encode(BuildCube(), PreserveOrGenerateId(cubePath)));
    ok &= WriteFile(planePath, MeshAsset::Encode(BuildPlane(), PreserveOrGenerateId(planePath)));
    ok &= WriteFile(texturePath, TextureAsset::Encode(png, textureId));
    ok &= WriteFile(materialPath,
        MaterialAsset::Encode(MaterialData{textureId}, PreserveOrGenerateId(materialPath)));

    if (ok)
        LUTUM_INFO("EngineContentGen: engine content written to '{}'", engineRoot);
    return ok;
}

} // Lutum