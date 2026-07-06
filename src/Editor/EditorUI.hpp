/*
* File: EditorUI.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_EDITORUI_HPP
#define LUTUM_EDITORUI_HPP

#include "Editor/EditorContext.hpp"
#include "Editor/Panels/StatsPanel.hpp"
#include "Editor/Panels/SystemsPanel.hpp"
#include "Editor/Panels/ViewportPanel.hpp"
#include "Editor/Panels/ArchetypesPanel.hpp"
#include "Editor/Panels/EntitiesPanel.hpp"
#include "Editor/Panels/InspectorPanel.hpp"
#include "Editor/Panels/ResourcesPanel.hpp"
#include "Editor/Panels/LogPanel.hpp"

namespace Lutum {
class RenderTarget;
namespace Curia { class Registry; class Scheduler; }

class EditorUI {
public:
    // Build all panels for this frame
    // Call between Debug::UI::BeginFrame and the renderer's UI pass
    void Draw(Curia::Registry& registry, Curia::Scheduler& scheduler, RenderTarget& sceneTarget);

    [[nodiscard]]
    const EditorContext& Context() const { return m_context; }

    // Rebuild the default dock layout on the next Draw (first run / View menu)
    void RequestLayoutReset() { m_layoutResetRequested = true; }

    // Non-const context: Application applies settings / reads relaunch requests
    [[nodiscard]]
    EditorContext& Context() { return m_context; }

    void SetRecentProjects(std::vector<std::string> projects) {
        m_recentProjects = std::move(projects);
    }

    void SetLayoutWorkSize(float w, float h) { m_lastWorkWidth = w; m_lastWorkHeight = h; }
    [[nodiscard]] float LayoutWorkWidth() const { return m_lastWorkWidth; }
    [[nodiscard]] float LayoutWorkHeight() const { return m_lastWorkHeight; }

private:
    void BuildDefaultLayout(unsigned int dockspaceId);

    bool m_layoutResetRequested = false;

    void DrawMainMenuBar(Curia::Registry& registry);
    void SaveSnapshotToDisk(Curia::Registry& registry);
    void LoadSnapshotFromDisk(Curia::Registry& registry);

    EditorContext m_context;
    ViewportPanel m_viewportPanel;
    StatsPanel m_statsPanel;
    SystemsPanel m_systemsPanel;
    EntitiesPanel m_entitiesPanel;
    InspectorPanel m_inspectorPanel;
    ArchetypesPanel m_archetypesPanel;
    ResourcesPanel m_resourcesPanel;
    LogPanel m_logPanel;

    std::vector<std::string> m_recentProjects;

    float m_lastWorkWidth = 0.0f;
    float m_lastWorkHeight = 0.0f;
};
} // Lutum

#endif //LUTUM_EDITORUI_HPP
