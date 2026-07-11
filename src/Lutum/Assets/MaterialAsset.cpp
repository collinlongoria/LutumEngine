/*
* File: MaterialAsset.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Assets/MaterialAsset.hpp"

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/PayloadIO.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"

namespace Lutum::MaterialAsset {

std::vector<uint8_t> Encode(const MaterialData& data, AssetID id) {
    AssetHeader header;
    header.typeKey = kTypeKey;
    header.id = id;
    const auto headerBytes = header.Encode();

    AssetIO::ByteWriter w;
    w.Bytes(headerBytes.data(), headerBytes.size());
    w.U32(kPayloadVersion);
    w.U64(data.albedo.value);
    return std::move(w.buf);
}

std::optional<MaterialData> Decode(std::span<const uint8_t> fileBytes) {
    const auto header = AssetHeader::Decode(fileBytes);
    if (!header || header->typeKey != kTypeKey)
        return std::nullopt;

    AssetIO::ByteReader r{fileBytes.data() + AssetHeader::kSize, fileBytes.size() - AssetHeader::kSize};
    uint32_t version = 0;
    MaterialData out;
    if (!r.Read(version) || version != kPayloadVersion)
        return std::nullopt;
    if (!r.Read(out.albedo.value) || r.pos != r.size)
        return std::nullopt;
    return out;
}

std::optional<MaterialData> Load(std::string_view virtualPath) {
    const auto bytes = FileSystem::ReadBytes(virtualPath);
    if (!bytes)
        return std::nullopt;
    auto decoded = Decode(*bytes);
    if (!decoded)
        LUTUM_WARN("MaterialAsset: '{}' is not a valid Material asset", virtualPath);
    return decoded;
}

} // Lutum::MaterialAsset