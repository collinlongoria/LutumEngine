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

#include <string>

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace Lutum {

// Shared by EntitiesPanel and ArchetypesPanel
std::string SignatureLabel(const Curia::Archetype& arch) {
    if (arch.Signature().empty())
        return "(no components)";
    std::string label;
    for (Curia::ComponentID cid : arch.Signature()) {
        if (!label.empty())
            label += " + ";
        label += Curia::ComponentRegistry::Get(cid).name;
    }
    return label;
}

void EntitiesPanel::Draw(Curia::Registry& registry, EditorContext& context) {
    if (ImGui::Begin("Entities", &context.showEntities, ImGuiWindowFlags_HorizontalScrollbar)) {
        const auto& archetypes = registry.Archetypes();
        for (size_t a = 0; a < archetypes.size(); ++a) {
            const Curia::Archetype& arch = *archetypes[a];

            uint32_t rows = 0;
            for (uint32_t s = 0; s < arch.SlabCount(); ++s)
                rows += arch.GetSlab(s)->entityCount;
            if (rows == 0)
                continue;

            ImGui::PushID(static_cast<int>(a));
            const std::string label = SignatureLabel(arch) + " (" + std::to_string(rows) + ")";
            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (uint32_t s = 0; s < arch.SlabCount(); ++s) {
                    const uint32_t count = arch.GetSlab(s)->entityCount;
                    for (uint32_t row = 0; row < count; ++row) {
                        const Curia::Entity e = arch.EntityAt(s, row);
                        char buf[32];
                        std::snprintf(buf, sizeof(buf), "%u:%u",
                                      Curia::EntityTraits::Index(e),
                                      Curia::EntityTraits::Generation(e));
                        if (ImGui::Selectable(buf, e == context.selectedEntity))
                            context.selectedEntity = e;
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}
} // Lutum
