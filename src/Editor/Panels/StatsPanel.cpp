/*
* File: StatsPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/StatsPanel.hpp"

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/Core/Time.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace Lutum {

void StatsPanel::Draw(Curia::Registry& registry, EditorContext& context) {
    if (ImGui::Begin("Stats", &context.showStats)) {
        const Time& time = registry.GetResource<Time>();
        ImGui::Text("Frame: %.2f ms (%.0f fps)",
                    time.DeltaSeconds() * 1000.0f,
                    time.DeltaSeconds() > 0.0f ? 1.0f / time.DeltaSeconds() : 0.0f);
        ImGui::Text("Entities: %zu", registry.EntityCount());
        ImGui::Text("Archetypes: %zu", registry.ArchetypeCount());

        if (!context.statusMessage.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", context.statusMessage.c_str());
        }
    }
    ImGui::End();
}
} // Lutum