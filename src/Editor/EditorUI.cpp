/*
* File: EditorUI.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/EditorUI.hpp"

#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace Lutum {

// TODO: replace with dynamic path later
static constexpr const char* kSnapshotPath = "/Game/scene.lsnap";

void EditorUI::Draw(Curia::Registry& registry, Curia::Scheduler& scheduler, RenderTarget& sceneTarget) {
    DrawMainMenuBar(registry);

    const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    if (m_layoutResetRequested) {
        BuildDefaultLayout(dockspaceId);
        m_layoutResetRequested = false;
    }

    m_viewportPanel.Draw(registry, sceneTarget);
    if (m_context.showStats)
        m_statsPanel.Draw(registry, m_context);
    if (m_context.showSystems)
        m_systemsPanel.Draw(scheduler, m_context);
    if (m_context.showEntities)
        m_entitiesPanel.Draw(registry, m_context);
    if (m_context.showInspector)
        m_inspectorPanel.Draw(registry, m_context);
    if (m_context.showArchetypes)
        m_archetypesPanel.Draw(registry, m_context);
    if (m_context.showResources)
        m_resourcesPanel.Draw(registry, m_context);
    if (m_context.showLog)
        m_logPanel.Draw(m_context);
}

void EditorUI::DrawMainMenuBar(Curia::Registry& registry) {
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save Snapshot"))
            SaveSnapshotToDisk(registry);
        if (ImGui::MenuItem("Load Snapshot", nullptr, false, FileSystem::Exists(kSnapshotPath)))
            LoadSnapshotFromDisk(registry);
        if (ImGui::BeginMenu("Recent Projects", !m_recentProjects.empty())) {
            for (const std::string& path : m_recentProjects) {
                if (ImGui::MenuItem(path.c_str()))
                    m_context.relaunchProjectPath = path; // Application relaunches
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
            m_context.exitRequested = true;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Stats", nullptr, &m_context.showStats);
        ImGui::MenuItem("Systems", nullptr, &m_context.showSystems);
        ImGui::MenuItem("Entities", nullptr, &m_context.showEntities);
        ImGui::MenuItem("Inspector", nullptr, &m_context.showInspector);
        ImGui::MenuItem("Archetypes", nullptr, &m_context.showArchetypes);
        ImGui::MenuItem("Resources", nullptr, &m_context.showResources);
        ImGui::MenuItem("Log", nullptr, &m_context.showLog);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout"))
            m_layoutResetRequested = true;
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorUI::SaveSnapshotToDisk(Curia::Registry& registry) {
    const std::vector<uint8_t> bytes = registry.SaveSnapshot();
    if (FileSystem::WriteBytes(kSnapshotPath, bytes)) {
        m_context.statusMessage = std::format("Saved snapshot ({} bytes)", bytes.size());
        LUTUM_INFO("Editor: saved snapshot to '{}' ({} bytes)", kSnapshotPath, bytes.size());
    }
    else {
        m_context.statusMessage = "Snapshot save FAILED (see log)";
    }
}

void EditorUI::LoadSnapshotFromDisk(Curia::Registry& registry) {
    const auto bytes = FileSystem::ReadBytes(kSnapshotPath);
    if (!bytes) {
        m_context.statusMessage = "Snapshot load FAILED: file unreadable";
        return;
    }

    const std::vector<uint8_t> backup = registry.SaveSnapshot();

    registry.Clear();
    if (registry.LoadSnapshot(*bytes)) {
        m_context.statusMessage = std::format("Loaded snapshot ({} bytes)", bytes->size());
        LUTUM_INFO("Editor: loaded snapshot from '{}'", kSnapshotPath);
    }
    else {
        [[maybe_unused]] const bool restored = registry.LoadSnapshot(backup);
        LUTUM_ASSERT(restored, "backup snapshot restore failed");
        m_context.statusMessage = "Snapshot load FAILED: incompatible data (scene restored)";
    }
}

void EditorUI::BuildDefaultLayout(unsigned int dockspaceId) {
    m_context.showStats = true;
    m_context.showSystems = true;
    m_context.showEntities = true;
    m_context.showInspector = true;
    m_context.showArchetypes = true;
    m_context.showResources = true;
    m_context.showLog = true;

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    ImGuiID center = dockspaceId;
    ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.30f, nullptr, &center);
    ImGuiID rightBottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.45f, nullptr, &right);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.22f, nullptr, &center);

    ImGui::DockBuilderDockWindow("Viewport", center);
    ImGui::DockBuilderDockWindow("Entities", left);
    ImGui::DockBuilderDockWindow("Archetypes", left);     // tabbed with Entities
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Stats", rightBottom);   // tabbed group:
    ImGui::DockBuilderDockWindow("Systems", rightBottom);
    ImGui::DockBuilderDockWindow("Resources", rightBottom);
    ImGui::DockBuilderDockWindow("Log", bottom);

    ImGui::DockBuilderFinish(dockspaceId);
}
} // Lutum
