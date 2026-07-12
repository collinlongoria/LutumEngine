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

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/ECS/Entity.hpp"

namespace Lutum {

struct EditorContext {
    // --- Editor Panel Visibility ---
    bool showStats = true;
    bool showSystems = true;
    bool showEntities = true;
    bool showInspector = true;
    bool showArchetypes = true;
    bool showResources = true;
    bool exitRequested = false;
    bool showLog = true;

    // --- Misc Data IDK rename this LOL ---
    // Set by the Recent Projects menu; Application relaunches and exits
    std::string relaunchProjectPath;
    Curia::Entity selectedEntity = Curia::INVALID_ENTITY;
    // Last menu-action result
    std::string statusMessage;
    // Renderer cache invalidation (material asset edited on disk)
    bool reloadMaterials = false;

    // --- Current level ---
    AssetID currentLevelId{};
    std::string currentLevelPath; // virtual; empty = untitled
    bool levelDirty = false;
    bool newLevelRequested = false;
    std::string openLevelPath; // virtual; non-empty = pending open
    std::string saveLevelPath; // virtual; non-empty = pending save
};

} // Lutum

#endif //LUTUM_EDITORCONTEXT_HPP
