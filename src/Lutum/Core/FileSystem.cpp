/*
* File: FileSystem.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Core/FileSystem.hpp"

#include <cstdio>
#include <filesystem>

#include <SDL3/SDL_filesystem.h>

#include "Lutum/Core/Log.hpp"

namespace fs = std::filesystem;

namespace Lutum::FileSystem {
namespace {

    struct FileSystemState {
        fs::path engineRoot; // .../EngineAssets
        fs::path contentRoot; // <project>/Content
        fs::path savedRoot; // <project>/Saved
        std::string projectName;
        bool initialized = false;
        bool projectMounted = false;
    };

    FileSystemState& State() {
        static FileSystemState state;
        return state;
    }

    // Reject anything that could escape a root
    bool IsPathSafe(std::string_view path) {
        if (path.empty())
            return false;

        size_t start = 0;
        while (start <= path.size()) {
            const size_t end = path.find('/', start);
            const std::string_view part = path.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);

            if (part == "..")
                return false;

            if (end == std::string_view::npos)
                break;
            start = end + 1;
        }
        return true;
    }

} // anonymous namespace

bool Initialize(const char* engineAssetOverride) {
    FileSystemState& state = State();

    if (engineAssetOverride) {
        state.engineRoot = fs::path(engineAssetOverride);
    }
    else {
        // Search upward from the executable for EngineAssets/
        const char* basePath = SDL_GetBasePath();
        fs::path search = basePath ? fs::path(basePath) : fs::current_path();

        constexpr int kMaxLevels = 6;
        for (int i = 0; i < kMaxLevels; ++i) {
            if (fs::exists(search / "EngineAssets")) {
                state.engineRoot = search / "EngineAssets";
                break;
            }
            if (!search.has_parent_path() || search.parent_path() == search)
                break;
            search = search.parent_path();
        }
    }

    if (state.engineRoot.empty() || !fs::exists(state.engineRoot)) {
        LUTUM_ERROR("FileSystem: EngineAssets not found (searched upward from executable)");
        return false;
    }

    state.initialized = true;
    LUTUM_INFO("FileSystem: engine root '{}'", state.engineRoot.string());
    return true;
}

void Shutdown() {
    State() = FileSystemState{};
}

bool MountProject(const char* projectDir) {
    FileSystemState& state = State();
    LUTUM_ASSERT(state.initialized, "FileSystem::Initialize must be called first");

    const fs::path root = fs::absolute(fs::path(projectDir));
    const fs::path projectFile = root / "project.lutum";
    const fs::path content = root / "Content";

    if (!fs::exists(projectFile)) {
        LUTUM_ERROR("FileSystem: no project.lutum in '{}'", root.string());
        return false;
    }
    if (!fs::exists(content) || !fs::is_directory(content)) {
        LUTUM_ERROR("FileSystem: no Content/ directory in '{}'", root.string());
        return false;
    }

    state.projectName = root.filename().string();
    if (std::FILE* file = std::fopen(projectFile.string().c_str(), "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), file)) {
            std::string_view view(line);
            if (view.rfind("name=", 0) == 0) {
                std::string value(view.substr(5));
                while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
                    value.pop_back();
                if (!value.empty())
                    state.projectName = value;
            }
        }
        std::fclose(file);
    }

    state.contentRoot = content;
    state.savedRoot = root / "Saved"; // created lazily on first write
    state.projectMounted = true;
    LUTUM_INFO("FileSystem: mounted project '{}' at '{}'", state.projectName, root.string());
    return true;
}

bool IsProjectMounted() {
    return State().projectMounted;
}
const std::string& ProjectName() {
    return State().projectName;
}

std::string Resolve(std::string_view virtualPath) {
    FileSystemState& state = State();

    std::string_view rest = virtualPath;
    const fs::path* root = nullptr;

    if (rest.rfind("/Engine/", 0) == 0) {
        rest.remove_prefix(8);
        root = &state.engineRoot;
    }
    else if (rest.rfind("/Game/", 0) == 0) {
        rest.remove_prefix(6);
        root = state.projectMounted ? &state.contentRoot : nullptr;
    }
    else if (rest.rfind("/Saved/", 0) == 0) {
        rest.remove_prefix(7);
        root = state.projectMounted ? &state.savedRoot : nullptr;
    }
    else if (!rest.empty() && rest.front() == '/') {
        LUTUM_WARN("FileSystem: unknown path prefix in '{}'", virtualPath);
        return {};
    }
    else {
        root = state.projectMounted ? &state.contentRoot : nullptr;
    }

    if (!root || root->empty()) {
        LUTUM_WARN("FileSystem: root not mounted for '{}'", virtualPath);
        return {};
    }
    if (!IsPathSafe(rest)) {
        LUTUM_WARN("FileSystem: rejected unsafe path '{}'", virtualPath);
        return {};
    }

    return (*root / fs::path(rest)).string();
}

bool Exists(std::string_view virtualPath) {
    const std::string resolved = Resolve(virtualPath);
    return !resolved.empty() && fs::exists(resolved);
}

std::optional<std::vector<uint8_t>> ReadBytes(std::string_view virtualPath) {
    const std::string resolved = Resolve(virtualPath);
    if (resolved.empty())
        return std::nullopt;

    std::FILE* file = std::fopen(resolved.c_str(), "rb");
    if (!file) {
        LUTUM_WARN("FileSystem: failed to open '{}'", virtualPath);
        return std::nullopt;
    }

    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> data(size > 0 ? static_cast<size_t>(size) : 0);
    if (!data.empty() && std::fread(data.data(), 1, data.size(), file) != data.size()) {
        std::fclose(file);
        LUTUM_WARN("FileSystem: short read on '{}'", virtualPath);
        return std::nullopt;
    }

    std::fclose(file);
    return data;
}

std::optional<std::string> ReadText(std::string_view virtualPath) {
    auto bytes = ReadBytes(virtualPath);
    if (!bytes)
        return std::nullopt;
    return std::string(bytes->begin(), bytes->end());
}

bool WriteBytes(std::string_view virtualPath, std::span<const uint8_t> data) {
    if (virtualPath.rfind("/Engine/", 0) == 0) {
        LUTUM_ERROR("FileSystem: refusing write to read-only /Engine/ path '{}'", virtualPath);
        return false;
    }

    const std::string resolved = Resolve(virtualPath);
    if (resolved.empty())
        return false;

    std::error_code ec;
    fs::create_directories(fs::path(resolved).parent_path(), ec);
    if (ec) {
        LUTUM_ERROR("FileSystem: failed to create directories for '{}': {}", virtualPath, ec.message());
        return false;
    }

    std::FILE* file = std::fopen(resolved.c_str(), "wb");
    if (!file) {
        LUTUM_ERROR("FileSystem: failed to open '{}' for writing", virtualPath);
        return false;
    }

    const bool ok = data.empty() || std::fwrite(data.data(), 1, data.size(), file) == data.size();
    std::fclose(file);

    if (!ok)
        LUTUM_ERROR("FileSystem: short write on '{}'", virtualPath);
    return ok;
}

bool WriteText(std::string_view virtualPath, std::string_view text) {
    return WriteBytes(virtualPath,
        std::span(reinterpret_cast<const uint8_t*>(text.data()), text.size()));
}

} // Lutum::Filesystem
