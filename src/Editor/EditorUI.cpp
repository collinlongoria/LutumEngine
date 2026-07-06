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
}

void EditorUI::DrawMainMenuBar(Curia::Registry& registry) {
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save Snapshot"))
            SaveSnapshotToDisk(registry);
        if (ImGui::MenuItem("Load Snapshot", nullptr, false, FileSystem::Exists(kSnapshotPath)))
            LoadSnapshotFromDisk(registry);
        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
            m_context.exitRequested = true;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Stats", nullptr, &m_context.showStats);
        ImGui::MenuItem("Systems", nullptr, &m_context.showSystems);
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

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    ImGuiID center = dockspaceId;
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
    ImGuiID rightBottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.5f, nullptr, &right);

    ImGui::DockBuilderDockWindow("Viewport", center);
    ImGui::DockBuilderDockWindow("Stats", right);
    ImGui::DockBuilderDockWindow("Systems", rightBottom);

    ImGui::DockBuilderFinish(dockspaceId);
}

} // Lutum
