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
    if (ImGui::Begin("Stats", &context.showStats, ImGuiWindowFlags_HorizontalScrollbar)) {
        const Time& time = registry.GetResource<Time>();
        ImGui::Text("Frame: %.2f ms (%.0f fps)",
                    time.DeltaSeconds() * 1000.0f,
                    time.DeltaSeconds() > 0.0f ? 1.0f / time.DeltaSeconds() : 0.0f);
        const float ms = time.DeltaSeconds() * 1000.0f;
        m_history[m_offset] = ms;
        m_offset = (m_offset + 1) % kHistorySize;

        float sum = 0.0f, worst = 0.0f;
        for (float v : m_history) { sum += v; worst = v > worst ? v : worst; }
        char overlay[64];
        std::snprintf(overlay, sizeof(overlay), "avg %.2f ms  worst %.2f ms",
                      sum / kHistorySize, worst);
        ImGui::PlotLines("##frametimes", m_history, kHistorySize, m_offset,
                         overlay, 0.0f, worst * 1.2f, ImVec2(-1, 60));
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