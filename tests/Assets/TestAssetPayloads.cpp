/*
* File: TestAssetPayloads.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <doctest/doctest.h>

#include "Lutum/Assets/MaterialAsset.hpp"
#include "Lutum/Assets/MeshAsset.hpp"
#include "Lutum/Assets/TextureAsset.hpp"

using namespace Lutum;

TEST_CASE("MeshAsset: encode/decode round trip") {
    MeshData mesh;
    mesh.vertices = {
        {{1, 2, 3}, {0, 1, 0}, {0.25f, 0.75f}},
        {{-4, 5, -6}, {0, 0, -1}, {1, 0}},
    };
    mesh.indices = {0, 1, 0};

    const AssetID id = GenerateAssetID();
    const auto bytes = MeshAsset::Encode(mesh, id);
    const auto back = MeshAsset::Decode(bytes);
    REQUIRE(back.has_value());
    REQUIRE(back->vertices.size() == 2);
    REQUIRE(back->indices.size() == 3);
    CHECK(back->vertices[0].position == mesh.vertices[0].position);
    CHECK(back->vertices[1].uv == mesh.vertices[1].uv);
    CHECK(back->indices == mesh.indices);
}

TEST_CASE("TextureAsset: source bytes round trip verbatim") {
    const std::vector<uint8_t> source = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 42};
    const auto bytes = TextureAsset::Encode(source, GenerateAssetID());
    const auto back = TextureAsset::Decode(bytes);
    REQUIRE(back.has_value());
    CHECK(*back == source);
}

TEST_CASE("MaterialAsset: albedo reference round trip") {
    const MaterialData material{AssetID{0xABCDULL}};
    const auto bytes = MaterialAsset::Encode(material, GenerateAssetID());
    const auto back = MaterialAsset::Decode(bytes);
    REQUIRE(back.has_value());
    CHECK(back->albedo == material.albedo);
}

TEST_CASE("Asset payloads: cross-type decode and corruption fail cleanly") {
    const auto meshBytes = MeshAsset::Encode(MeshData{}, GenerateAssetID());
    CHECK(!TextureAsset::Decode(meshBytes).has_value());   // wrong type key
    CHECK(!MaterialAsset::Decode(meshBytes).has_value());

    auto truncated = meshBytes;
    truncated.resize(truncated.size() - 1);
    CHECK(!MeshAsset::Decode(truncated).has_value());       // truncated payload

    auto trailing = meshBytes;
    trailing.push_back(0);
    CHECK(!MeshAsset::Decode(trailing).has_value());        // trailing bytes
}