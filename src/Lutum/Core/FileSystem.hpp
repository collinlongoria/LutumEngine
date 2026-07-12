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
#include <span>

namespace Lutum {

/*
 * Project-space file-access system
 *
 * virtual prefixes:
 *      /Engine/ -> EngineAssets/           (read-only at runtime)
 *      /Game/   -> <project>/Content/      (read/write)
 *      /Saved/  -> <project>/Saved/        (read/write; editor state, gitignored)
 *
 *  NOTE: prefix-less paths resolve against /Game/
 */
namespace FileSystem {
    // Locates EngineAssets by searching upward from the executable directory
    // Call before MountProject
    // NOTE: directory can be overriden for unusual setups
    bool Initialize(const char* engineAssetOverride = nullptr);
    void Shutdown();

    // Validates project.lutum and Content/, then roots /Game/ and /Saved/ there
    bool MountProject(const char* projectDir);
    [[nodiscard]]
    bool IsProjectMounted();
    [[nodiscard]]
    const std::string& ProjectName();

    // Virtual Path -> Absolute Path
    // Empty string returned if the path is malformed or the relevant root is not mounted
    [[nodiscard]]
    std::string Resolve(std::string_view virtualPath);
    // absolute path -> virtual path (inverse of resolve)
    [[nodiscard]]
    std::optional<std::string> ToVirtual(std::string_view absolutePath);

    [[nodiscard]]
    bool Exists(std::string_view virtualPath);
    [[nodiscard]]
    std::optional<std::vector<uint8_t>> ReadBytes(std::string_view virtualPath);
    [[nodiscard]]
    std::optional<std::string> ReadText(std::string_view virtualPath);

    // Writing: /Game/ and /Saved/ only
    // parent directories are created as needed
    bool WriteBytes(std::string_view virtualPath, std::span<const uint8_t> data);
    bool WriteText(std::string_view virtualPath, std::string_view text);

    // Creates a directory (and parents) for a virtual path
    bool EnsureDirectory(std::string_view virtualPath);

    struct DirEntry {
        std::string name; // filename only (no path)
        bool isDirectory = false;
    };

    [[nodiscard]]
    std::optional<std::vector<DirEntry>> ListDirectory(std::string_view virtualPath);

    [[nodiscard]]
    std::optional<std::vector<uint8_t>> ReadBytesPrefix(std::string_view virtualPath, size_t maxBytes);
} // Filesystem
} // Lutum
#endif //LUTUM_FILESYSTEM_HPP
