/*
* File: AssetRegistry.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ASSETREGISTRY_HPP
#define LUTUM_ASSETREGISTRY_HPP
#include <functional>
#include <string>
#include <vector>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"

namespace Lutum::Assets {

struct AssetInfo {
    AssetID id{};
    StableKey typeKey = 0;
    std::string virtualPath; // .lasset filepath
    std::string name; // filename stem
};

bool Initialize(); // NOTE: requires FileSystem::Initialize
void Shutdown();
void Rescan();

// NOTE: pointer returned remains valid until the next Rescan/Shutdown
[[nodiscard]]
const AssetInfo* Find(AssetID id);

// NOTE: same lifetime rules as FInd
[[nodiscard]]
std::vector<const AssetInfo*> FindByType(StableKey typeKey);

void ForEach(const std::function<void(const AssetInfo&)>& fn);

[[nodiscard]]
size_t Count();

}

#endif //LUTUM_ASSETREGISTRY_HPP
