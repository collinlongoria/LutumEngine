/*
* File: EditorContext.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_EDITORCONTEXT_HPP
#define LUTUM_EDITORCONTEXT_HPP
#include <string>

namespace Lutum {

struct EditorContext {
    bool showStats = true;
    bool showSystems = true;
    bool exitRequested = false;

    // last menu-action result
    std::string statusMessage;
};

} // Lutum

#endif //LUTUM_EDITORCONTEXT_HPP
