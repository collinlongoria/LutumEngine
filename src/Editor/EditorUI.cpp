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

#include <filesystem>
#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include "AssetImport.hpp"
#include "AssetPicker.hpp"
#include "FileDialogs.hpp"
#include "Editor/EngineContentGen.hpp"
#include "Lutum/Assets/AssetHeader.hpp"
#include "Lutum/Assets/AssetRegistry.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/Scene/MeshRenderer.hpp"
#include "Lutum/Scene/Transform.hpp"

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

void EditorUI::Draw(Curia::Registry& registry, Curia::Scheduler& scheduler, RenderTarget& sceneTarget) {
    ProcessDialogResults();

    DrawMainMenuBar(registry);
    ResolvePendingLevelAction();

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
    DrawMaterialEditor();
}

void EditorUI::DrawMainMenuBar(Curia::Registry& registry) {
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Level"))
            m_pendingLevelAction = PendingLevelAction::NEW_LEVEL; // confirm below if dirty
        if (ImGui::MenuItem("Open Level..."))
            m_pendingLevelAction = PendingLevelAction::OPEN_LEVEL;
        ImGui::Separator();
        if (ImGui::MenuItem("Save Level")) {
            if (m_context.currentLevelPath.empty())
                FileDialogs::ShowSaveFile(FileDialogs::Purpose::SAVE_LEVEL, "Level assets", "lasset");
            else
                m_context.saveLevelPath = m_context.currentLevelPath;
        }
        if (ImGui::MenuItem("Save Level As..."))
            FileDialogs::ShowSaveFile(FileDialogs::Purpose::SAVE_LEVEL, "Level assets", "lasset");
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
        if (ImGui::MenuItem("Spawn Cube (dev)")) {
            const auto* cube = Assets::FindByPath("/Engine/Primitives/Cube.lasset");
            const auto* mat = Assets::FindByPath("/Engine/Materials/DefaultMaterial.lasset");
            if (cube) {
                const Curia::Entity e = registry.Create();
                registry.Add(e, Transform{});
                registry.Add(e, MeshRenderer{cube->id, mat ? mat->id : AssetID{}});
                registry.Add(e, MakeName("Cube"));
                m_context.levelDirty = true;
            }
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
        ImGui::Separator();
        if (ImGui::MenuItem("New Material...")) {
            const std::string contentDir = FileSystem::Resolve("/Game/");
            FileDialogs::ShowSaveFile(FileDialogs::Purpose::NEW_MATERIAL,
                                      "Material assets", "lasset",
                                      contentDir.empty() ? nullptr : contentDir.c_str());
        }
        if (ImGui::BeginMenu("Edit Material")) {
            const auto materials = Assets::FindByType(HashName("Material"));
            if (materials.empty())
                ImGui::TextDisabled("(no materials)");
            for (const Assets::AssetInfo* info : materials) {
                ImGui::PushID(static_cast<int>(info->id.value));
                if (ImGui::MenuItem(info->name.c_str()))
                    OpenMaterialEditor(*info);
                ImGui::PopID();
            }
            ImGui::EndMenu();
        }
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

    // level indicator
    const std::string levelLabel =
        (m_context.currentLevelPath.empty() ? std::string("Untitled")
            : m_context.currentLevelPath.substr(m_context.currentLevelPath.find_last_of('/') + 1))
        + (m_context.levelDirty ? " *" : "");
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(levelLabel.c_str()).x - 12.0f);
    ImGui::TextDisabled("%s", levelLabel.c_str());

    ImGui::EndMainMenuBar();
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

void EditorUI::ResolvePendingLevelAction() {
    if (m_pendingLevelAction == PendingLevelAction::NONE)
        return;

    if (!m_context.levelDirty) {
        ExecutePendingLevelAction();
        return;
    }
    ImGui::OpenPopup("Unsaved Changes");
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The current level has unsaved changes.\nThey will be lost.");
        if (ImGui::Button("Discard and Continue")) {
            ExecutePendingLevelAction();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_pendingLevelAction = PendingLevelAction::NONE;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorUI::ExecutePendingLevelAction() {
    if (m_pendingLevelAction == PendingLevelAction::NEW_LEVEL)
        m_context.newLevelRequested = true;
    else if (m_pendingLevelAction == PendingLevelAction::OPEN_LEVEL)
        FileDialogs::ShowOpenFile(FileDialogs::Purpose::OPEN_LEVEL, "Level assets", "lasset", false);
    m_pendingLevelAction = PendingLevelAction::NONE;
}

void EditorUI::ProcessDialogResults() {
    for (const FileDialogs::Result& result : FileDialogs::Drain()) {
        if (result.paths.empty())
            continue; // cancelled

        switch (result.purpose) {
            case FileDialogs::Purpose::IMPORT_MESH:
            case FileDialogs::Purpose::IMPORT_TEXTURE: {
                m_pendingImportSources = result.paths;
                m_pendingImportIsMesh = (result.purpose == FileDialogs::Purpose::IMPORT_MESH);

                const std::string contentDir = FileSystem::Resolve("/Game/");
                if (result.paths.size() == 1) {
                    // Single file: save dialog = rename + destination in one step,
                    // prefilled with "<source stem>.lasset"
                    const std::string defaultPath = contentDir.empty() ? std::string{}
                    : (std::filesystem::path(contentDir)
                       / AssetImport::DefaultTargetName(result.paths.front())).string();
                    FileDialogs::ShowSaveFile(FileDialogs::Purpose::IMPORT_TARGET_FILE,
                                              "Lutum assets", "lasset",
                                              defaultPath.empty() ? nullptr : defaultPath.c_str());
                }
                else {
                    // Multi-select: pick a folder, default names apply
                    FileDialogs::ShowOpenFolder(FileDialogs::Purpose::IMPORT_DESTINATION,
                                                contentDir.empty() ? nullptr : contentDir.c_str());
                }
                break;
            }
            case FileDialogs::Purpose::IMPORT_TARGET_FILE: {
                if (m_pendingImportSources.empty())
                    break; // stale result; nothing staged

                std::string path = result.paths.front();
                if (!path.ends_with(".lasset"))
                    path += ".lasset";
                const auto virtualPath = FileSystem::ToVirtual(path);
                if (!virtualPath || virtualPath->rfind("/Game/", 0) != 0) {
                    LUTUM_ERROR("Import: '{}' is outside the project Content folder", path);
                    m_pendingImportSources.clear();
                    break;
                }

                const std::string source = m_pendingImportSources.front();
                m_pendingImportSources.clear();
                const bool ok = m_pendingImportIsMesh
                    ? AssetImport::ImportMeshFile(source, *virtualPath)
                    : AssetImport::ImportTextureFile(source, *virtualPath);
                if (ok)
                    Assets::Rescan();
                break;
            }
            case FileDialogs::Purpose::IMPORT_DESTINATION: {
                const auto folder = FileSystem::ToVirtual(result.paths.front());
                if (!folder || folder->rfind("/Game/", 0) != 0) {
                    LUTUM_ERROR("Import: destination '{}' is outside the project Content folder",
                                result.paths.front());
                    m_pendingImportSources.clear();
                    break;
                }
                bool imported = false;
                for (const std::string& source : m_pendingImportSources) {
                    const std::string target = (folder->back() == '/' ? *folder : *folder + "/")
                                             + AssetImport::DefaultTargetName(source);
                    imported |= m_pendingImportIsMesh
                        ? AssetImport::ImportMeshFile(source, target)
                        : AssetImport::ImportTextureFile(source, target);
                }
                m_pendingImportSources.clear();
                if (imported)
                    Assets::Rescan();
                break;
            }
            case FileDialogs::Purpose::NEW_MATERIAL: {
                std::string path = result.paths.front();
                if (!path.ends_with(".lasset"))
                    path += ".lasset";
                const auto virtualPath = FileSystem::ToVirtual(path);
                if (!virtualPath || virtualPath->rfind("/Game/", 0) != 0) {
                    LUTUM_ERROR("New Material: '{}' is outside the project Content folder", path);
                    break;
                }

                AssetID id{}; // overwrite keeps identity, same rule as everywhere
                if (FileSystem::Exists(*virtualPath)) {
                    if (const auto head = FileSystem::ReadBytesPrefix(*virtualPath, AssetHeader::kSize))
                        if (const auto header = AssetHeader::Decode(*head))
                            id = header->id;
                }
                if (id.IsNull())
                    id = GenerateAssetID();

                if (FileSystem::WriteBytes(*virtualPath, MaterialAsset::Encode(MaterialData{}, id))) {
                    Assets::Rescan();
                    if (const Assets::AssetInfo* info = Assets::Find(id))
                        OpenMaterialEditor(*info);
                }
                break;
            }
            case FileDialogs::Purpose::OPEN_LEVEL: {
                const auto virtualPath = FileSystem::ToVirtual(result.paths.front());
                if (virtualPath && virtualPath->rfind("/Game/", 0) == 0)
                    m_context.openLevelPath = *virtualPath;
                else
                    LUTUM_ERROR("Open Level: '{}' is outside the project Content folder", result.paths.front());
                break;
            }
            case FileDialogs::Purpose::SAVE_LEVEL: {
                std::string path = result.paths.front();
                if (!path.ends_with(".lasset"))
                    path += ".lasset";
                const auto virtualPath = FileSystem::ToVirtual(path);
                if (virtualPath && virtualPath->rfind("/Game/", 0) == 0)
                    m_context.saveLevelPath = *virtualPath;
                else
                    LUTUM_ERROR("Save Level: '{}' is outside the project Content folder", path);
                break;
            }
            default:
                LUTUM_WARN("FileDialogs: unhandled dialog purpose {}", static_cast<int>(result.purpose));
                break;
        }
    }
}

void EditorUI::OpenMaterialEditor(const Assets::AssetInfo& info) {
    const auto data = MaterialAsset::Load(info.virtualPath);
    if (!data) {
        LUTUM_ERROR("Material Editor: failed to load '{}'", info.virtualPath);
        return;
    }
    m_materialEditor.open = true;
    m_materialEditor.id = info.id;
    m_materialEditor.virtualPath = info.virtualPath;
    m_materialEditor.data = *data;
    m_materialEditor.dirty = false;
}

void EditorUI::DrawMaterialEditor() {
    if (!m_materialEditor.open)
        return;

    if (ImGui::Begin("Material Editor", &m_materialEditor.open,
                     ImGuiWindowFlags_AlwaysAutoResize)) {
        const Assets::AssetInfo* info = Assets::Find(m_materialEditor.id);
        ImGui::TextDisabled("%s", info ? info->virtualPath.c_str()
                                       : m_materialEditor.virtualPath.c_str());
        ImGui::Separator();

        if (DrawAssetPicker("albedo", m_materialEditor.data.albedo, HashName("Texture")))
            m_materialEditor.dirty = true;

        ImGui::Separator();
        ImGui::BeginDisabled(!m_materialEditor.dirty);
        if (ImGui::Button("Save")) {
            if (FileSystem::WriteBytes(m_materialEditor.virtualPath,
                    MaterialAsset::Encode(m_materialEditor.data, m_materialEditor.id))) {
                m_materialEditor.dirty = false;
                m_context.reloadMaterials = true; // renderer's MaterialPool re-resolves
                Assets::Rescan();
                    }
        }
        ImGui::SameLine();
        if (ImGui::Button("Revert")) {
            if (const auto data = MaterialAsset::Load(m_materialEditor.virtualPath)) {
                m_materialEditor.data = *data;
                m_materialEditor.dirty = false;
            }
        }
        ImGui::EndDisabled();
                     }
    ImGui::End();
}
} // Lutum
