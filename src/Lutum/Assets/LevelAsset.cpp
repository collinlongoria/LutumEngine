/*
* File: LevelAsset.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/12/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Assets/LevelAsset.hpp"

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/PayloadIO.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"

namespace Lutum::LevelAsset {

std::vector<uint8_t> Encode(std::span<const uint8_t> sourceImageBytes, AssetID id) {
    AssetHeader header;
    header.typeKey = kTypeKey;
    header.id = id;
    const auto headerBytes = header.Encode();

    AssetIO::ByteWriter w;
    w.Bytes(headerBytes.data(), headerBytes.size());
    w.U32(kPayloadVersion);
    w.Bytes(sourceImageBytes.data(), sourceImageBytes.size());
    return std::move(w.buf);
}

std::optional<std::vector<uint8_t>> Decode(std::span<const uint8_t> fileBytes) {
    const auto header = AssetHeader::Decode(fileBytes);
    if (!header || header->typeKey != kTypeKey)
        return std::nullopt;

    AssetIO::ByteReader r{fileBytes.data() + AssetHeader::kSize, fileBytes.size() - AssetHeader::kSize};
    uint32_t version = 0;
    if (!r.Read(version) || version != kPayloadVersion)
        return std::nullopt;

    return std::vector<uint8_t>(r.data + r.pos, r.data + r.size);
}

std::optional<std::vector<uint8_t>> Load(std::string_view virtualPath) {
    const auto bytes = FileSystem::ReadBytes(virtualPath);
    if (!bytes)
        return std::nullopt;
    auto decoded = Decode(*bytes);
    if (!decoded)
        LUTUM_WARN("LevelAsset: '{}' is not a valid Level asset", virtualPath);
    return decoded;
}

} // Lutum::LevelAsset