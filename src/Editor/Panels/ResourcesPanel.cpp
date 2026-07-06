/*
* File: ResourcesPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/ResourcesPanel.hpp"

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Resource.hpp"

namespace Lutum {

void ResourcesPanel::Draw(Curia::Registry& registry, EditorContext& context) {
    if (ImGui::Begin("Resources", &context.showResources, ImGuiWindowFlags_HorizontalScrollbar)) {
        const uint32_t count = Curia::ResourceRegistry::Count();
        for (Curia::ResourceID id = 0; id < count; ++id) {
            const bool set = registry.HasResource(id);
            ImGui::Text("%s", Curia::ResourceRegistry::Name(id).c_str());
            ImGui::SameLine();
            if (set) ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "set");
            else     ImGui::TextDisabled("not set");
        }
        if (count == 0)
            ImGui::TextDisabled("No resources registered");
    }
    ImGui::End();
}
} // Lutum