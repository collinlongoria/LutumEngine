/*
* File: ArchetypesPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/ArchetypesPanel.hpp"

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Editor/Panels/EntitiesPanel.hpp" // SignatureLabel
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Slab.hpp"

namespace Lutum {

void ArchetypesPanel::Draw(Curia::Registry& registry, EditorContext& context) {
    if (ImGui::Begin("Archetypes", &context.showArchetypes)) {
        const auto& archetypes = registry.Archetypes();
        for (size_t a = 0; a < archetypes.size(); ++a) {
            const Curia::Archetype& arch = *archetypes[a];

            uint32_t rows = 0;
            for (uint32_t s = 0; s < arch.SlabCount(); ++s)
                rows += arch.GetSlab(s)->entityCount;

            size_t stride = 0;
            for (Curia::ComponentID cid : arch.Signature())
                stride += arch.ComponentSize(cid);

            ImGui::PushID(static_cast<int>(a));
            ImGui::SeparatorText(SignatureLabel(arch).c_str());
            ImGui::Text("Entities: %u   Slabs: %u   Capacity/slab: %u",
                        rows, arch.SlabCount(), arch.Capacity());
            if (arch.SlabCount() > 0) {
                const float util = arch.Capacity() > 0
                    ? static_cast<float>(rows) / (arch.SlabCount() * arch.Capacity()) : 0.0f;
                ImGui::Text("Memory: %zu KB   Row stride: %zu B   Utilization: %.0f%%",
                            arch.SlabCount() * Curia::Slab::SIZE / 1024, stride, util * 100.0f);
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}
} // Lutum