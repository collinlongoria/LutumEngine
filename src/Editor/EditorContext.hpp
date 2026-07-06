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

#include "Lutum/ECS/Entity.hpp"

namespace Lutum {

struct EditorContext {
    bool showStats = true;
    bool showSystems = true;
    bool showEntities = true;
    bool showInspector = true;
    bool showArchetypes = true;
    bool showResources = true;
    bool exitRequested = false;

    Curia::Entity selectedEntity = Curia::INVALID_ENTITY;

    // Last menu-action result
    std::string statusMessage;
};

} // Lutum

#endif //LUTUM_EDITORCONTEXT_HPP
