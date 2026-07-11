/*
* File: MeshAsset.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Assets/MeshAsset.hpp"

#include <cstring>

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/PayloadIO.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"

namespace Lutum::MeshAsset {

std::vector<uint8_t> Encode(const MeshData& data, AssetID id) {
    AssetHeader header;
    header.typeKey = kTypeKey;
    header.id = id;
    const auto headerBytes = header.Encode();

    AssetIO::ByteWriter w;
    w.Bytes(headerBytes.data(), headerBytes.size());
    w.U32(kPayloadVersion);
    w.U32(static_cast<uint32_t>(data.vertices.size()));
    w.U32(static_cast<uint32_t>(data.indices.size()));
    w.Bytes(data.vertices.data(), data.vertices.size() * sizeof(MeshVertex));
    w.Bytes(data.indices.data(), data.indices.size() * sizeof(uint32_t));
    return std::move(w.buf);
}

std::optional<MeshData> Decode(std::span<const uint8_t> fileBytes) {
    const auto header = AssetHeader::Decode(fileBytes);
    if (!header || header->typeKey != kTypeKey)
        return std::nullopt;

    AssetIO::ByteReader r{fileBytes.data() + AssetHeader::kSize, fileBytes.size() - AssetHeader::kSize};

    uint32_t version = 0, vertexCount = 0, indexCount = 0;
    if (!r.Read(version) || version != kPayloadVersion)
        return std::nullopt;
    if (!r.Read(vertexCount) || !r.Read(indexCount))
        return std::nullopt;

    const uint8_t* verts = r.View(static_cast<size_t>(vertexCount) * sizeof(MeshVertex));
    const uint8_t* inds = r.View(static_cast<size_t>(indexCount) * sizeof(uint32_t));
    if (!verts || !inds || r.pos != r.size) // truncated or trailing bytes
        return std::nullopt;

    MeshData out;
    out.vertices.resize(vertexCount);
    out.indices.resize(indexCount);
    std::memcpy(out.vertices.data(), verts, static_cast<size_t>(vertexCount) * sizeof(MeshVertex));
    std::memcpy(out.indices.data(), inds, static_cast<size_t>(indexCount) * sizeof(uint32_t));
    return out;
}

std::optional<MeshData> Load(std::string_view virtualPath) {
    const auto bytes = FileSystem::ReadBytes(virtualPath);
    if (!bytes)
        return std::nullopt;
    auto decoded = Decode(*bytes);
    if (!decoded)
        LUTUM_WARN("MeshAsset: '{}' is not a valid StaticMesh asset", virtualPath);
    return decoded;
}

} // Lutum::MeshAsset