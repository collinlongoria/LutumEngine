/*
* File: AssetPicker.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/12/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/AssetPicker.hpp"

#include <algorithm>
#include <vector>

#include <imgui.h>

#include "Lutum/Assets/AssetRegistry.hpp"

namespace Lutum {

bool DrawAssetPicker(const char* label, AssetID& value, StableKey typeFilter) {
    bool changed = false;

    const Assets::AssetInfo* current = Assets::Find(value);
    const char* preview = value.IsNull() ? "(none)"
        : current ? current->name.c_str() : "(missing)";

    if (ImGui::BeginCombo(label, preview)) {
        if (ImGui::Selectable("(none)", value.IsNull()) && !value.IsNull()) {
            value = AssetID{};
            changed = true;
        }

        std::vector<const Assets::AssetInfo*> candidates;
        if (typeFilter != 0) {
            candidates = Assets::FindByType(typeFilter);
        }
        else {
            Assets::ForEach([&](const Assets::AssetInfo& info) {
                candidates.push_back(&info);
            });
            std::sort(candidates.begin(), candidates.end(),
                [](const auto* a, const auto* b) { return a->name < b->name; });
        }

        for (const Assets::AssetInfo* info : candidates) {
            ImGui::PushID(static_cast<int>(info->id.value));
            if (ImGui::Selectable(info->name.c_str(), info->id == value) && info->id != value) {
                value = info->id;
                changed = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s\n%016llx", info->virtualPath.c_str(),
                                  static_cast<unsigned long long>(info->id.value));
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    if (ImGui::IsItemHovered()) {
        if (current)
            ImGui::SetTooltip("%s\n%016llx", current->virtualPath.c_str(),
                              static_cast<unsigned long long>(value.value));
        else if (!value.IsNull())
            ImGui::SetTooltip("broken reference\n%016llx",
                              static_cast<unsigned long long>(value.value));
    }
    return changed;
}

} // Lutum