/*
* File: AssetImport.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/AssetImport.hpp"

#include <filesystem>
#include <fstream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/MeshAsset.hpp"
#include "Lutum/Assets/TextureAsset.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/Graphics/ImageIO.hpp"

namespace fs = std::filesystem;

namespace Lutum::AssetImport {
namespace {

    std::string TargetPath(const std::string& absolutePath) {
        return "/Game/" + fs::path(absolutePath).stem().string() + ".lasset";
    }

    AssetID PreserveOrGenerateId(const std::string& virtualPath) {
        if (FileSystem::Exists(virtualPath)) {
            if (const auto bytes = FileSystem::ReadBytesPrefix(virtualPath, AssetHeader::kSize)) {
                if (const auto header = AssetHeader::Decode(*bytes)) {
                    LUTUM_INFO("Import: overwriting '{}' — UUID preserved", virtualPath);
                    return header->id;
                }
            }
        }
        return GenerateAssetID();
    }

    std::optional<std::vector<uint8_t>> ReadSourceFile(const std::string& absolutePath) {
        std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
        if (!file) {
            LUTUM_ERROR("Import: cannot open '{}'", absolutePath);
            return std::nullopt;
        }
        std::vector<uint8_t> bytes(static_cast<size_t>(file.tellg()));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return bytes;
    }

} // anonymous namespace

bool ImportMeshFile(const std::string& absolutePath) {
    Assimp::Importer importer;
    // FlipUVs: Assimp UVs are bottom-left origin, ours are top-left (stb rows)
    // PreTransformVertices: bake node transforms — one section, no hierarchy (#21)
    const aiScene* scene = importer.ReadFile(absolutePath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices |
        aiProcess_FlipUVs);
    if (!scene || scene->mNumMeshes == 0) {
        LUTUM_ERROR("Import: Assimp failed on '{}': {}", absolutePath, importer.GetErrorString());
        return false;
    }
    if (scene->mNumMaterials > 1) // Assimp always emits one default material
        LUTUM_INFO("Import: '{}' has materials — ignored (one-section StaticMesh)", absolutePath);

    MeshData mesh;
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* src = scene->mMeshes[m];
        const auto base = static_cast<uint32_t>(mesh.vertices.size());

        for (unsigned v = 0; v < src->mNumVertices; ++v) {
            MeshVertex vert;
            vert.position = {src->mVertices[v].x, src->mVertices[v].y, src->mVertices[v].z};
            vert.normal = src->HasNormals()
                ? Vec3{src->mNormals[v].x, src->mNormals[v].y, src->mNormals[v].z}
                : Vec3{0, 1, 0};
            vert.uv = src->HasTextureCoords(0)
                ? Vec2{src->mTextureCoords[0][v].x, src->mTextureCoords[0][v].y}
                : Vec2{0, 0};
            mesh.vertices.push_back(vert);
        }
        for (unsigned f = 0; f < src->mNumFaces; ++f) {
            const aiFace& face = src->mFaces[f];
            if (face.mNumIndices != 3)
                continue; // points/lines survive triangulation; skip
            for (unsigned i = 0; i < 3; ++i)
                mesh.indices.push_back(base + face.mIndices[i]);
        }
    }
    if (mesh.indices.empty()) {
        LUTUM_ERROR("Import: '{}' produced no triangles", absolutePath);
        return false;
    }

    const std::string target = TargetPath(absolutePath);
    if (!FileSystem::WriteBytes(target, MeshAsset::Encode(mesh, PreserveOrGenerateId(target))))
        return false;

    LUTUM_INFO("Import: '{}' -> '{}' ({} vertices, {} triangles)",
               absolutePath, target, mesh.vertices.size(), mesh.indices.size() / 3);
    return true;
}

bool ImportTextureFile(const std::string& absolutePath) {
    const auto bytes = ReadSourceFile(absolutePath);
    if (!bytes)
        return false;

    if (!ImageIO::LoadFromMemory(*bytes)) { // must be decodable before we wrap it
        LUTUM_ERROR("Import: '{}' is not a decodable image", absolutePath);
        return false;
    }

    const std::string target = TargetPath(absolutePath);
    if (!FileSystem::WriteBytes(target, TextureAsset::Encode(*bytes, PreserveOrGenerateId(target))))
        return false;

    LUTUM_INFO("Import: '{}' -> '{}'", absolutePath, target);
    return true;
}

} // Lutum::AssetImport