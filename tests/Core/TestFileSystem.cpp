/*
* File: TestFileSystem.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "Lutum/Core/FileSystem.hpp"

namespace fs = std::filesystem;
using namespace Lutum;

namespace {

// Temp engine + project trees; FileSystem state is global, so Shutdown on scope exit
struct FileSystemFixture {
    fs::path root;

    FileSystemFixture() {
        root = fs::temp_directory_path() / "lutum_fs_test";
        fs::remove_all(root);
        fs::create_directories(root / "EngineAssets");
        fs::create_directories(root / "Project" / "Content");
        std::ofstream(root / "Project" / "project.lutum") << "name=FsTest\n";

        REQUIRE(FileSystem::Initialize((root / "EngineAssets").string().c_str()));
        REQUIRE(FileSystem::MountProject((root / "Project").string().c_str()));
    }
    ~FileSystemFixture() {
        FileSystem::Shutdown();
        fs::remove_all(root);
    }
};

} // anonymous namespace

TEST_CASE("FileSystem: write/read round trip in /Game/ and /Saved/") {
    FileSystemFixture fx;

    const std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    CHECK(FileSystem::WriteBytes("/Game/Levels/test.bin", payload)); // nested dir creation
    auto back = FileSystem::ReadBytes("/Game/Levels/test.bin");
    REQUIRE(back.has_value());
    CHECK(*back == payload);

    CHECK(FileSystem::WriteText("/Saved/editor.lutum", "speed=5\n"));
    CHECK(fs::exists(fx.root / "Project" / "Saved" / "editor.lutum")); // lazily created
    auto text = FileSystem::ReadText("/Saved/editor.lutum");
    REQUIRE(text.has_value());
    CHECK(*text == "speed=5\n");

    CHECK(FileSystem::ProjectName() == "FsTest");
}

TEST_CASE("FileSystem: write policy") {
    FileSystemFixture fx;

    const std::vector<uint8_t> payload = {1};
    CHECK_FALSE(FileSystem::WriteBytes("/Engine/nope.bin", payload));   // read-only root
    CHECK_FALSE(FileSystem::WriteBytes("/Game/../escape.bin", payload)); // unsafe path
    CHECK_FALSE(FileSystem::WriteBytes("/Bogus/x.bin", payload));        // unknown prefix
}