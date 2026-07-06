/*
* File: EditorSettings.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/EditorSettings.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>

#include <SDL3/SDL_filesystem.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/Core/FileSystem.hpp"

namespace Lutum {

static constexpr const char* kSettingsPath = "/Saved/editor.lutum";
static constexpr size_t kMaxRecent = 8;

EditorSettings EditorSettings::Load() {
    EditorSettings s;
    const auto text = FileSystem::ReadText(kSettingsPath);
    if (!text)
        return s; // defaults

    std::istringstream stream(*text);
    std::string line;
    while (std::getline(stream, line)) {
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "camera.moveSpeed") s.cameraMoveSpeed = std::strtof(value.c_str(), nullptr);
        else if (key == "panel.stats")      s.showStats      = (value == "1");
        else if (key == "panel.systems")    s.showSystems    = (value == "1");
        else if (key == "panel.entities")   s.showEntities   = (value == "1");
        else if (key == "panel.inspector")  s.showInspector  = (value == "1");
        else if (key == "panel.archetypes") s.showArchetypes = (value == "1");
        else if (key == "panel.resources")  s.showResources  = (value == "1");
        else if (key == "panel.log")        s.showLog        = (value == "1");
        else if (key == "layout.workWidth")  s.layoutWorkWidth  = std::strtof(value.c_str(), nullptr);
        else if (key == "layout.workHeight") s.layoutWorkHeight = std::strtof(value.c_str(), nullptr);
        else if (key == "window.width")      s.windowWidth  = std::atoi(value.c_str());
        else if (key == "window.height")     s.windowHeight = std::atoi(value.c_str());
        else if (key == "window.maximized")  s.windowMaximized = (value == "1");
    }

    // clamp
    s.windowWidth = std::max(s.windowWidth, 640);
    s.windowHeight = std::max(s.windowHeight, 480);

    return s;
}

void EditorSettings::Save() const {
    std::string out;
    out += std::format("camera.moveSpeed={}\n", cameraMoveSpeed);
    out += std::format("panel.stats={}\n",      showStats      ? 1 : 0);
    out += std::format("panel.systems={}\n",    showSystems    ? 1 : 0);
    out += std::format("panel.entities={}\n",   showEntities   ? 1 : 0);
    out += std::format("panel.inspector={}\n",  showInspector  ? 1 : 0);
    out += std::format("panel.archetypes={}\n", showArchetypes ? 1 : 0);
    out += std::format("panel.resources={}\n",  showResources  ? 1 : 0);
    out += std::format("panel.log={}\n",        showLog        ? 1 : 0);
    out += std::format("layout.workWidth={}\n", layoutWorkWidth);
    out += std::format("layout.workHeight={}\n", layoutWorkHeight);
    out += std::format("window.width={}\n", windowWidth);
    out += std::format("window.height={}\n", windowHeight);
    out += std::format("window.maximized={}\n", windowMaximized ? 1 : 0);
    FileSystem::WriteText(kSettingsPath, out);
}

void EditorSettings::ApplyTo(EditorContext& context) const {
    context.showStats = showStats;
    context.showSystems = showSystems;
    context.showEntities = showEntities;
    context.showInspector = showInspector;
    context.showArchetypes = showArchetypes;
    context.showResources = showResources;
    context.showLog = showLog;
}

void EditorSettings::CaptureFrom(const EditorContext& context) {
    showStats = context.showStats;
    showSystems = context.showSystems;
    showEntities = context.showEntities;
    showInspector = context.showInspector;
    showArchetypes = context.showArchetypes;
    showResources = context.showResources;
    showLog = context.showLog;
}

static std::string RecentFilePath() {
    char* pref = SDL_GetPrefPath("Lutum", "LutumEditor");
    if (!pref)
        return {};
    std::string path = std::string(pref) + "recent.lutum";
    SDL_free(pref);
    return path;
}

std::vector<std::string> LoadRecentProjects() {
    std::vector<std::string> list;
    const std::string path = RecentFilePath();
    if (path.empty())
        return list;

    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line) && list.size() < kMaxRecent) {
        if (!line.empty())
            list.push_back(line);
    }
    return list;
}

void AddRecentProject(std::vector<std::string>& list, const std::string& absolutePath) {
    std::erase(list, absolutePath);          // dedupe
    list.insert(list.begin(), absolutePath); // most recent first
    if (list.size() > kMaxRecent)
        list.resize(kMaxRecent);
}

void SaveRecentProjects(const std::vector<std::string>& list) {
    const std::string path = RecentFilePath();
    if (path.empty())
        return;
    std::ofstream file(path);
    for (const std::string& p : list)
        file << p << '\n';
}
} // Lutum