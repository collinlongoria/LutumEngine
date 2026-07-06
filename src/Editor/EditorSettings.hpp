/*
* File: EditorSettings.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_EDITORSETTINGS_HPP
#define LUTUM_EDITORSETTINGS_HPP
#include <string>
#include <vector>

namespace Lutum {
struct EditorContext;

// Per-project editor settings: /Saved/editor.lutum (text key=value)
struct EditorSettings {
    float cameraMoveSpeed = 5.0f;
    bool showStats = true;
    bool showSystems = true;
    bool showEntities = true;
    bool showInspector = true;
    bool showArchetypes = true;
    bool showResources = true;
    bool showLog = true;

    static EditorSettings Load();
    void Save() const;

    void ApplyTo(EditorContext& context) const;
    void CaptureFrom(const EditorContext& context);
};

// Per-USER recent projects
std::vector<std::string> LoadRecentProjects();
void AddRecentProject(std::vector<std::string>& list, const std::string& absolutePath);
void SaveRecentProjects(const std::vector<std::string>& list);

} // Lutum

#endif