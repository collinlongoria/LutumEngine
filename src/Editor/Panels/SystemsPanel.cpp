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
#include "Lutum/ECS/Resource.hpp"

namespace Lutum {

namespace {
    std::string JoinComponentNames(std::span<const Curia::ComponentID> ids) {
        std::string out;
        for (Curia::ComponentID cid : ids) {
            if (!out.empty()) out += ", ";
            out += Curia::ComponentRegistry::Get(cid).name;
        }
        return out.empty() ? "-" : out;
    }
    std::string JoinResourceNames(std::span<const Curia::ResourceID> ids) {
        std::string out;
        for (Curia::ResourceID rid : ids) {
            if (!out.empty()) out += ", ";
            out += Curia::ResourceRegistry::Name(rid);
        }
        return out.empty() ? "-" : out;
    }
} // anonymous namespace

void SystemsPanel::Draw(Curia::Scheduler& scheduler, EditorContext& context) {
    if (ImGui::Begin("Systems", &context.showSystems, ImGuiWindowFlags_HorizontalScrollbar)) {
        const size_t phaseCount = scheduler.PhaseCount();
        for (size_t phase = 0; phase < phaseCount; ++phase) {
            ImGui::SeparatorText(("Phase " + std::to_string(phase)).c_str());

            for (size_t i = 0; i < scheduler.SystemCount(); ++i) {
                const auto view = scheduler.GetSystemView(i);
                if (view.phase != static_cast<int>(phase))
                    continue;

                ImGui::PushID(static_cast<int>(i));
                if (ImGui::TreeNode(std::string(view.name).c_str())) {
                    ImGui::Text("Reads:  %s", JoinComponentNames(view.reads).c_str());
                    ImGui::Text("Writes: %s", JoinComponentNames(view.writes).c_str());
                    ImGui::Text("Resource reads:  %s", JoinResourceNames(view.resourceReads).c_str());
                    ImGui::Text("Resource writes: %s", JoinResourceNames(view.resourceWrites).c_str());
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
        if (phaseCount == 0)
            ImGui::TextDisabled("Scheduler not compiled");
    }
    ImGui::End();
}
} // Lutum