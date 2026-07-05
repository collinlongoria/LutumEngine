/*
* File: FileSystem.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_FILESYSTEM_HPP
#define LUTUM_FILESYSTEM_HPP
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Lutum {

/*
 * Project-space file-access system
 */
namespace FileSystem {
    // Locates EngineAssets by searching upward from the executable directory
    // Call before MountProject
    // NOTE: directory can be overriden for unusual setups
    bool Initialize(const char* engineAssetOverride = nullptr);
    void Shutdown();

    // Validates project.lutum and Content/, then roots /Game/ there
    bool MountProject(const char* projectDir);
    [[nodiscard]]
    bool IsProjectMounted();
    [[nodiscard]]
    const std::string& ProjectName();

    // Virtual Path -> Absolute Path
    // Empty string returned if the path is malformed or the relevant root is not mounted
    [[nodiscard]]
    std::string Resolve(std::string_view virtualPath);

    [[nodiscard]]
    bool Exists(std::string_view virtualPath);
    [[nodiscard]]
    std::optional<std::vector<uint8_t>> ReadBytes(std::string_view virtualPath);
    [[nodiscard]]
    std::optional<std::string> ReadText(std::string_view virtualPath);
} // Filesystem

} // Lutum

#endif //LUTUMENGINE_FILESYSTEM_HPP
