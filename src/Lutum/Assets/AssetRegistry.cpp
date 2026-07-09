/*
* File: AssetRegistry.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Assets/AssetRegistry.hpp"

#include <algorithm>
#include <unordered_map>

#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"

namespace Lutum::Assets {
namespace {

    constexpr std::string_view kAssetExtension = ".lasset";

    struct RegistryState {
        std::unordered_map<AssetID, AssetInfo> assets;
        bool initialized = false;
    };

    RegistryState& State() {
        static RegistryState state;
        return state;
    }

    bool HasAssetExtension(std::string_view name) {
        return name.size() > kAssetExtension.size() && name.ends_with(kAssetExtension);
    }

    std::string Join(const std::string& dir, const std::string& name) {
        if (!dir.empty() && dir.back() == '/')
            return dir + name;
        return dir + "/" + name;
    }

    void ScanFile(const std::string& virtualPath, const std::string& fileName) {
        const auto bytes = FileSystem::ReadBytesPrefix(virtualPath, AssetHeader::kSize);
        if (!bytes)
            return; // ReadBytesPrefix already warned

        const auto header = AssetHeader::Decode(*bytes);
        if (!header) {
            LUTUM_WARN("AssetRegistry: '{}' has an invalid or unsupported header — skipped", virtualPath);
            return;
        }
        if (header->id.IsNull()) {
            LUTUM_WARN("AssetRegistry: '{}' has a null UUID — skipped", virtualPath);
            return;
        }

        RegistryState& state = State();
        if (const auto it = state.assets.find(header->id); it != state.assets.end()) {
            LUTUM_ERROR("AssetRegistry: UUID collision — '{}' and '{}' share {:#018x}; "
                        "keeping the first (file copied on disk?)",
                        it->second.virtualPath, virtualPath, header->id.value);
            return;
        }

        AssetInfo info;
        info.id = header->id;
        info.typeKey = header->typeKey;
        info.virtualPath = virtualPath;
        info.name = fileName.substr(0, fileName.size() - kAssetExtension.size());
        state.assets.emplace(header->id, std::move(info));
    }

    void ScanDirectory(const std::string& virtualDir) {
        const auto entries = FileSystem::ListDirectory(virtualDir);
        if (!entries)
            return;

        for (const FileSystem::DirEntry& entry : *entries) {
            const std::string childPath = Join(virtualDir, entry.name);
            if (entry.isDirectory)
                ScanDirectory(childPath);
            else if (HasAssetExtension(entry.name))
                ScanFile(childPath, entry.name);
        }
    }

} // anonymous namespace

bool Initialize() {
    State().initialized = true;
    Rescan();
    return true;
}

void Shutdown() {
    State() = RegistryState{};
}

void Rescan() {
    RegistryState& state = State();
    LUTUM_ASSERT(state.initialized, "Assets::Initialize must be called first");

    state.assets.clear();
    ScanDirectory("/Engine/");
    if (FileSystem::IsProjectMounted())
        ScanDirectory("/Game/");

    LUTUM_INFO("AssetRegistry: {} asset(s) registered", state.assets.size());
}

const AssetInfo* Find(AssetID id) {
    const RegistryState& state = State();
    const auto it = state.assets.find(id);
    return it != state.assets.end() ? &it->second : nullptr;
}

std::vector<const AssetInfo*> FindByType(StableKey typeKey) {
    std::vector<const AssetInfo*> result;
    for (const auto& [id, info] : State().assets) {
        if (info.typeKey == typeKey)
            result.push_back(&info);
    }
    std::sort(result.begin(), result.end(),
        [](const AssetInfo* a, const AssetInfo* b) { return a->name < b->name; });
    return result;
}

void ForEach(const std::function<void(const AssetInfo&)>& fn) {
    for (const auto& [id, info] : State().assets)
        fn(info);
}

size_t Count() {
    return State().assets.size();
}

} // Lutum::Assets