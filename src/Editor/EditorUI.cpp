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

#include "AssetImport.hpp"
#include "FileDialogs.hpp"
#include "Editor/EngineContentGen.hpp"
#include "Lutum/Assets/AssetRegistry.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace {
// Proportionally rescale every node's SizeRef
void ScaleDockTree(ImGuiDockNode* node, float sx, float sy) {
    if (!node)
        return;
    node->SizeRef.x *= sx;
    node->SizeRef.y *= sy;
    ScaleDockTree(node->ChildNodes[0], sx, sy);
    ScaleDockTree(node->ChildNodes[1], sx, sy);
}
} // anonymous namespace

namespace Lutum {

// TODO: replace with dynamic path later
static constexpr const char* kSnapshotPath = "/Game/scene.lsnap";

void EditorUI::Draw(Curia::Registry& registry, Curia::Scheduler& scheduler, RenderTarget& sceneTarget) {
    ProcessDialogResults();

    DrawMainMenuBar(registry);

    const ImGuiID dockspaceId = ImHashStr("LutumDockSpace");
    const ImVec2 work = ImGui::GetMainViewport()->WorkSize;

    if (m_layoutResetRequested) {
        BuildDefaultLayout(dockspaceId);
        m_layoutResetRequested = false;
        m_lastWorkWidth = work.x;
        m_lastWorkHeight = work.y;
    }
    else if (work.x > 0.0f && work.y > 0.0f) {
        if (ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspaceId)) {
            const bool changed = std::fabs(work.x - m_lastWorkWidth) > 0.5f ||
                                 std::fabs(work.y - m_lastWorkHeight) > 0.5f;
            if (m_lastWorkWidth > 0.0f && changed)
                ScaleDockTree(root, work.x / m_lastWorkWidth, work.y / m_lastWorkHeight);
            m_lastWorkWidth = work.x;
            m_lastWorkHeight = work.y;
        }
        else if (m_lastWorkWidth <= 0.0f) {
            m_lastWorkWidth = work.x;
            m_lastWorkHeight = work.y;
        }
    }

    ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport());

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

    if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Generate Engine Content (dev)")) {
            if (GenerateEngineContent())
                Assets::Rescan();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Assets")) {
        if (ImGui::MenuItem("Import Mesh..."))
            FileDialogs::ShowOpenFile(FileDialogs::Purpose::IMPORT_MESH,
                                      "Mesh files", "fbx;obj;gltf;glb", true);
        if (ImGui::MenuItem("Import Texture..."))
            FileDialogs::ShowOpenFile(FileDialogs::Purpose::IMPORT_TEXTURE,
                                      "Image files", "png;jpg;jpeg", true);
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

    // Mirrors the hand-arranged layout: bottom strip first (root Y split),
    // then Inspector column, then the left tab column; Viewport stays central.
    ImGuiID center = dockspaceId;
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.20f, nullptr, &center);
    ImGuiID bottomRight = ImGui::DockBuilderSplitNode(bottom, ImGuiDir_Right, 0.20f, nullptr, &bottom);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
    ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.29f, nullptr, &center);

    ImGui::DockBuilderDockWindow("Viewport", center);
    ImGui::DockBuilderDockWindow("Entities", left);    // tab group, Entities in front
    ImGui::DockBuilderDockWindow("Archetypes", left);
    ImGui::DockBuilderDockWindow("Systems", left);
    ImGui::DockBuilderDockWindow("Resources", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Log", bottom);
    ImGui::DockBuilderDockWindow("Stats", bottomRight);

    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorUI::ProcessDialogResults() {
    for (const FileDialogs::Result& result : FileDialogs::Drain()) {
        if (result.paths.empty())
            continue; // cancelled

        switch (result.purpose) {
            case FileDialogs::Purpose::IMPORT_MESH: {
                bool imported = false;
                for (const std::string& path : result.paths)
                    imported |= AssetImport::ImportMeshFile(path);
                if (imported)
                    Assets::Rescan();
                break;
            }
            case FileDialogs::Purpose::IMPORT_TEXTURE: {
                bool imported = false;
                for (const std::string& path : result.paths)
                    imported |= AssetImport::ImportTextureFile(path);
                if (imported)
                    Assets::Rescan();
                break;
            }
            default:
                LUTUM_WARN("FileDialogs: unhandled dialog purpose {}", static_cast<int>(result.purpose));
                break;
        }
    }
}
} // Lutum
