/*
* File: SystemsPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/SystemsPanel.hpp"

#include <string>

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/ECS/Scheduler.hpp"

namespace Lutum {

void SystemsPanel::Draw(Curia::Scheduler& scheduler, EditorContext& context) {
    if (ImGui::Begin("Systems", &context.showSystems)) {
        const auto phases = scheduler.PhaseNames();
        for (size_t i = 0; i < phases.size(); ++i) {
            ImGui::SeparatorText(("Phase " + std::to_string(i)).c_str());
            for (const auto& name : phases[i]) {
                ImGui::BulletText("%.*s", static_cast<int>(name.size()), name.data());
            }
        }
    }
    ImGui::End();
}
} // Lutum