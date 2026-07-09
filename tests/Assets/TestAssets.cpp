/*
* File: TestAssets.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/AssetRegistry.hpp"
#include "Lutum/Core/FileSystem.hpp"

namespace fs = std::filesystem;
using namespace Lutum;

namespace {

constexpr StableKey kMeshType = HashName("StaticMesh");
constexpr StableKey kTextureType = HashName("Texture");

// Raw write, bypassing FileSystem (also how /Engine/ test content gets planted,
// since FileSystem correctly refuses /Engine/ writes)
void WriteRawAsset(const fs::path& absolute, AssetID id, StableKey type) {
    AssetHeader header;
    header.typeKey = type;
    header.id = id;
    const auto bytes = header.Encode();

    fs::create_directories(absolute.parent_path());
    std::ofstream file(absolute, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.put('\x42'); // token payload byte; registry must not care
}

struct AssetFixture {
    fs::path root;

    AssetFixture() {
        root = fs::temp_directory_path() / "lutum_asset_test";
        fs::remove_all(root);
        fs::create_directories(root / "EngineAssets");
        fs::create_directories(root / "Project" / "Content");
        std::ofstream(root / "Project" / "project.lutum") << "name=AssetTest\n";

        REQUIRE(FileSystem::Initialize((root / "EngineAssets").string().c_str()));
        REQUIRE(FileSystem::MountProject((root / "Project").string().c_str()));
    }
    ~AssetFixture() {
        Assets::Shutdown();
        FileSystem::Shutdown();
        fs::remove_all(root);
    }

    [[nodiscard]] fs::path Game() const { return root / "Project" / "Content"; }
    [[nodiscard]] fs::path Engine() const { return root / "EngineAssets"; }
};

} // anonymous namespace

TEST_CASE("AssetRegistry: scan finds assets under /Game/ and /Engine/") {
    AssetFixture fx;
    const AssetID gameId = GenerateAssetID();
    const AssetID engineId = GenerateAssetID();
    WriteRawAsset(fx.Game() / "props" / "rock.lasset", gameId, kMeshType);
    WriteRawAsset(fx.Engine() / "Primitives" / "cube.lasset", engineId, kMeshType);

    REQUIRE(Assets::Initialize());
    CHECK(Assets::Count() == 2);

    const Assets::AssetInfo* rock = Assets::Find(gameId);
    REQUIRE(rock != nullptr);
    CHECK(rock->name == "rock");
    CHECK(rock->typeKey == kMeshType);
    CHECK(rock->virtualPath == "/Game/props/rock.lasset");

    const Assets::AssetInfo* cube = Assets::Find(engineId);
    REQUIRE(cube != nullptr);
    CHECK(cube->virtualPath == "/Engine/Primitives/cube.lasset");

    CHECK(Assets::Find(AssetID{0xDEADBEEFULL}) == nullptr);
}

TEST_CASE("AssetRegistry: FindByType filters and sorts by name") {
    AssetFixture fx;
    WriteRawAsset(fx.Game() / "zebra.lasset", GenerateAssetID(), kMeshType);
    WriteRawAsset(fx.Game() / "apple.lasset", GenerateAssetID(), kMeshType);
    WriteRawAsset(fx.Game() / "skin.lasset", GenerateAssetID(), kTextureType);

    REQUIRE(Assets::Initialize());
    const auto meshes = Assets::FindByType(kMeshType);
    REQUIRE(meshes.size() == 2);
    CHECK(meshes[0]->name == "apple");
    CHECK(meshes[1]->name == "zebra");
    CHECK(Assets::FindByType(kTextureType).size() == 1);
}

TEST_CASE("AssetRegistry: UUID collision keeps the first, junk is skipped") {
    AssetFixture fx;
    const AssetID sharedId = GenerateAssetID();
    WriteRawAsset(fx.Game() / "a.lasset", sharedId, kMeshType);
    WriteRawAsset(fx.Game() / "b.lasset", sharedId, kMeshType); // disk-copied duplicate

    // junk: wrong magic, truncated header, unsupported container version
    { std::ofstream f(fx.Game() / "junk.lasset", std::ios::binary); f << "NOTANASSET~~~~~~~~~~~~~~~~~~~~~~~~"; }
    { std::ofstream f(fx.Game() / "short.lasset", std::ios::binary); f << "LAST"; }
    {
        AssetHeader h;
        h.containerVersion = 999;
        h.id = GenerateAssetID();
        auto bytes = h.Encode();
        std::ofstream f(fx.Game() / "future.lasset", std::ios::binary);
        f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

    REQUIRE(Assets::Initialize());
    CHECK(Assets::Count() == 1); // only ONE of a/b, none of the junk

    const Assets::AssetInfo* kept = Assets::Find(sharedId);
    REQUIRE(kept != nullptr);
    CHECK(kept->virtualPath == "/Game/a.lasset"); // scan is name-sorted: a before b
}

TEST_CASE("AssetRegistry: Rescan picks up new files") {
    AssetFixture fx;
    REQUIRE(Assets::Initialize());
    CHECK(Assets::Count() == 0);

    const AssetID id = GenerateAssetID();
    WriteRawAsset(fx.Game() / "late.lasset", id, kMeshType);
    Assets::Rescan();
    CHECK(Assets::Count() == 1);
    CHECK(Assets::Find(id) != nullptr);
}