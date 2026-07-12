/*
* File: FileDialogs.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_FILEDIALOGS_HPP
#define LUTUM_FILEDIALOGS_HPP
#include <cstdint>
#include <string>
#include <vector>

namespace Lutum::FileDialogs {

enum class Purpose : uint8_t {
    IMPORT_MESH,
    IMPORT_TEXTURE,
    IMPORT_DESTINATION,
    IMPORT_TARGET_FILE,
    NEW_MATERIAL,
    OPEN_LEVEL,
    SAVE_LEVEL,
    OPEN_PROJECT
};

struct Result {
    Purpose purpose{};
    std::vector<std::string> paths; // empty = cancelled or error
};

void ShowOpenFile(Purpose purpose, const char* filterName, const char* filterPattern, bool allowMany, const char* defaultLocation = nullptr);
void ShowSaveFile(Purpose purpose, const char* filterName, const char* filterPattern, const char* defaultLocation = nullptr);
void ShowOpenFolder(Purpose purpose, const char* defaultLocation = nullptr);

// NOTE: main thread ONLY
[[nodiscard]]
std::vector<Result> Drain();

} // Lutum::FileDialogs
#endif //LUTUM_FILEDIALOGS_HPP
