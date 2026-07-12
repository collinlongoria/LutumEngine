/*
* File: EditorTags.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/12/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_EDITORTAGS_HPP
#define LUTUM_EDITORTAGS_HPP

namespace Lutum {

// Entities carrying this never enter level files
struct EditorOnly {
    static constexpr const char* kCuriaName = "EditorOnly";
};

} // Lutum
#endif //LUTUM_EDITORTAGS_HPP