/*
* File: EntitiesPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/EntitiesPanel.hpp"

#include <cfloat>
#include <cstdio>
#include <string>

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace {

// Next unused "Entity<N>" name, N starting at 1
// Scans existing names so numbering survives relaunches and snapshot loads
Lutum::Name NextDefaultName(Lutum::Curia::Registry& registry) {
    using namespace Lutum::Curia;

    uint32_t next = 1;
    const ComponentID nameId = ComponentType<Lutum::Name>::Id();

    for (const auto& archPtr : registry.Archetypes()) {
        if (!archPtr->HasComponent(nameId))
            continue;
        for (uint32_t s = 0; s < archPtr->SlabCount(); ++s) {
            const uint32_t count = archPtr->GetSlab(s)->entityCount;
            for (uint32_t row = 0; row < count; ++row) {
                const Lutum::Name* name = registry.Get<Lutum::Name>(archPtr->EntityAt(s, row));
                if (name == nullptr || std::strncmp(name->value, "Entity", 6) != 0 || name->value[6] == '\0')
                    continue;
                char* end = nullptr;
                const unsigned long n = std::strtoul(name->value + 6, &end, 10);
                if (end != nullptr && *end == '\0' && n >= next)
                    next = static_cast<uint32_t>(n) + 1;
            }
        }
    }

    char buf[Lutum::Name::kCapacity];
    std::snprintf(buf, sizeof(buf), "Entity%u", next);
    return Lutum::MakeName(buf);
}

} // anonymous namespace

namespace Lutum {

using namespace Curia;

// Shared by EntitiesPanel and ArchetypesPanel
std::string SignatureLabel(const Archetype& arch) {
    if (arch.Signature().empty())
        return "(no components)";
    std::string label;
    for (ComponentID cid : arch.Signature()) {
        if (!label.empty())
            label += " + ";
        label += ComponentRegistry::Get(cid).name;
    }
    return label;
}

void EntitiesPanel::Draw(Registry& registry, EditorContext& context) {
    bool createRequested = false;
    bool commitRename = false;
    Entity destroyTarget = INVALID_ENTITY;
    Entity duplicateTarget = INVALID_ENTITY;

    // Rename target can die externally
    if (m_renameTarget != INVALID_ENTITY && !registry.Alive(m_renameTarget))
        m_renameTarget = INVALID_ENTITY;

    if (ImGui::Begin("Entities", &context.showEntities, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (ImGui::Button("Create Entity"))
            createRequested = true;
        ImGui::Separator();

        const auto& archetypes = registry.Archetypes();
        for (size_t a = 0; a < archetypes.size(); ++a) {
            const Archetype& arch = *archetypes[a];

            uint32_t rows = 0;
            for (uint32_t s = 0; s < arch.SlabCount(); ++s)
                rows += arch.GetSlab(s)->entityCount;
            if (rows == 0)
                continue;

            ImGui::PushID(static_cast<int>(a));
            const std::string header = SignatureLabel(arch) + " (" + std::to_string(rows) + ")";
            if (ImGui::TreeNodeEx(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (uint32_t s = 0; s < arch.SlabCount(); ++s) {
                    const uint32_t count = arch.GetSlab(s)->entityCount;
                    for (uint32_t row = 0; row < count; ++row) {
                        const Entity e = arch.EntityAt(s, row);
                        const uint32_t index = EntityTraits::Index(e);
                        const uint32_t gen = EntityTraits::Generation(e);
                        const Name* name = registry.Get<Name>(e);

                        // Inline rename editor
                        if (e == m_renameTarget) {
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            if (m_renameFocusPending) {
                                ImGui::SetKeyboardFocusHere();
                                m_renameFocusPending = false;
                            }
                            const bool entered = ImGui::InputText("##rename", m_renameBuffer,
                                sizeof(m_renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
                            if (entered) {
                                commitRename = true;
                            }
                            else if (ImGui::IsItemDeactivated()) {
                                m_renameTarget = INVALID_ENTITY; // click-away / Escape = cancel
                            }
                            continue;
                        }

                        // Normal row
                        char label[64];
                        if (name != nullptr && name->value[0] != '\0')
                            std::snprintf(label, sizeof(label), "%s##%u_%u", name->value, index, gen);
                        else
                            std::snprintf(label, sizeof(label), "%u:%u", index, gen);

                        if (ImGui::Selectable(label, e == context.selectedEntity))
                            context.selectedEntity = e;

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            BeginRename(e, name);

                        if (ImGui::BeginPopupContextItem()) {
                            context.selectedEntity = e;
                            if (ImGui::MenuItem("Rename"))
                                BeginRename(e, name);
                            if (ImGui::MenuItem("Duplicate"))
                                duplicateTarget = e;
                            ImGui::Separator();
                            if (ImGui::MenuItem("Destroy"))
                                destroyTarget = e;
                            ImGui::EndPopup();
                        }
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        // Empty-space context menu
        if (ImGui::BeginPopupContextWindow("EntitiesContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Entity"))
                createRequested = true;
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    if (createRequested) {
        const Entity e = registry.Create();
        registry.Add<Name>(e, NextDefaultName(registry));
        context.selectedEntity = e;
    }

    if (duplicateTarget != INVALID_ENTITY)
        context.selectedEntity = registry.Duplicate(duplicateTarget);

    if (commitRename && registry.Alive(m_renameTarget)) {
        if (registry.Has<Name>(m_renameTarget))
            registry.Set(m_renameTarget, MakeName(m_renameBuffer));
        else
            registry.Add<Name>(m_renameTarget, MakeName(m_renameBuffer));
        m_renameTarget = INVALID_ENTITY;
    }

    if (destroyTarget != INVALID_ENTITY) {
        registry.Destroy(destroyTarget);
        if (context.selectedEntity == destroyTarget)
            context.selectedEntity = INVALID_ENTITY;
        if (m_renameTarget == destroyTarget)
            m_renameTarget = INVALID_ENTITY;
    }
}

void EntitiesPanel::BeginRename(Entity e, const Name* current) {
    m_renameTarget = e;
    m_renameFocusPending = true;
    std::snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s",
                  current != nullptr ? current->value : "");
}
} // Lutum
