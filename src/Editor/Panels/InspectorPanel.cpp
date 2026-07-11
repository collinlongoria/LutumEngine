/*
* File: InspectorPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/InspectorPanel.hpp"

#include <algorithm>
#include <cstring>
#include <string>

#include <imgui.h>

#include "Editor/AssetTypeHints.hpp"
#include "Editor/EditorContext.hpp"
#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Assets/AssetRegistry.hpp"
#include "Lutum/Core/Math.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Reflect.hpp"

namespace Lutum {

using namespace Curia;

void InspectorPanel::Draw(Registry& registry, EditorContext& context) {
    bool hasPendingAdd = false;
    bool hasPendingRemove = false;
    ComponentID pendingAdd = 0;
    ComponentID pendingRemove = 0;
    Entity e = INVALID_ENTITY;

    if (ImGui::Begin("Inspector", &context.showInspector, ImGuiWindowFlags_HorizontalScrollbar)) {
        e = context.selectedEntity;
        if (e == INVALID_ENTITY || !registry.Alive(e)) {
            ImGui::TextDisabled(e == INVALID_ENTITY ? "No entity selected" : "Selected entity is dead");
            ImGui::End();
            return;
        }

        ImGui::Text("Entity %u:%u", EntityTraits::Index(e), EntityTraits::Generation(e));
        ImGui::Separator();

        const Archetype* arch = registry.ArchetypeOf(e);
        for (ComponentID cid : arch->Signature()) {
            const ComponentInfo& info = ComponentRegistry::Get(cid);
            ImGui::PushID(static_cast<int>(cid));

            const bool open = ImGui::CollapsingHeader(info.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            // Must follow the header immediately: binds to the last item's ID
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove Component")) {
                    hasPendingRemove = true;
                    pendingRemove = cid;
                }
                ImGui::EndPopup();
            }

            if (open) {
                if (info.size == 0) {
                    ImGui::TextDisabled("(tag)");
                }
                else if (info.fields.empty()) {
                    ImGui::TextDisabled("(no reflected fields, %zu bytes)", info.size);
                }
                else {
                    std::byte* base = static_cast<std::byte*>(registry.GetRaw(e, cid));
                    for (size_t i = 0; i < info.fields.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i));
                        DrawField(info.name, info.fields[i], base);
                        ImGui::PopID();
                    }
                }
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        if (ImGui::Button("Add Component", ImVec2(-FLT_MIN, 0.0f)))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent")) {
            // Every registered component the entity doesn't have, alphabetical
            std::vector<ComponentID> candidates;
            const uint32_t count = ComponentRegistry::Count();
            for (ComponentID cid = 0; cid < count; ++cid) {
                if (!arch->HasComponent(cid))
                    candidates.push_back(cid);
            }
            std::sort(candidates.begin(), candidates.end(), [](ComponentID a, ComponentID b) {
                return ComponentRegistry::Get(a).name < ComponentRegistry::Get(b).name;
            });

            if (candidates.empty())
                ImGui::TextDisabled("(nothing to add)");
            for (ComponentID cid : candidates) {
                if (ImGui::MenuItem(ComponentRegistry::Get(cid).name.c_str())) {
                    hasPendingAdd = true;
                    pendingAdd = cid;
                }
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    if (hasPendingRemove)
        registry.RemoveRaw(e, pendingRemove);
    if (hasPendingAdd)
        registry.AddRaw(e, pendingAdd, nullptr);
}

void InspectorPanel::DrawField(const std::string& componentName, const FieldInfo& field, std::byte* base) {
    if (field.type == FieldType::Char) {
        char* text = reinterpret_cast<char*>(base + field.offset);
        text[field.count - 1] = '\0';
        ImGui::InputText(field.name, text, field.count);
        return;
    }

    for (uint32_t elem = 0; elem < field.count; ++elem) {
        ImGui::PushID(static_cast<int>(elem));

        std::string label = field.name;
        if (field.count > 1)
            label += "[" + std::to_string(elem) + "]";

        std::byte* ptr = base + field.offset + elem * FieldTypeSize(field.type);

        switch (field.type) {
            case FieldType::F32:
                ImGui::DragFloat(label.c_str(), reinterpret_cast<float*>(ptr), 0.05f);
                break;
            case FieldType::I32:
                ImGui::DragScalar(label.c_str(), ImGuiDataType_S32, ptr);
                break;
            case FieldType::U32:
                ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, ptr);
                break;
            case FieldType::U16:
                ImGui::DragScalar(label.c_str(), ImGuiDataType_U16, ptr);
                break;
            case FieldType::U8:
                ImGui::DragScalar(label.c_str(), ImGuiDataType_U8, ptr);
                break;
            case FieldType::Bool:
                ImGui::Checkbox(label.c_str(), reinterpret_cast<bool*>(ptr));
                break;
            case FieldType::Vec2:
                ImGui::DragFloat2(label.c_str(), reinterpret_cast<float*>(ptr), 0.05f);
                break;
            case FieldType::Vec3:
                ImGui::DragFloat3(label.c_str(), reinterpret_cast<float*>(ptr), 0.05f);
                break;
            case FieldType::Vec4:
                ImGui::DragFloat4(label.c_str(), reinterpret_cast<float*>(ptr), 0.05f);
                break;
            case FieldType::Quat: {
                Quat q;
                std::memcpy(&q, ptr, sizeof(Quat));
                Vec3 euler = glm::degrees(glm::eulerAngles(q));
                if (ImGui::DragFloat3(label.c_str(), &euler.x, 0.5f)) {
                    q = Quat(glm::radians(euler));
                    std::memcpy(ptr, &q, sizeof(Quat));
                }
                break;
            }
            case FieldType::EntityRef: {
                Entity target;
                std::memcpy(&target, ptr, sizeof(Entity));
                if (target == INVALID_ENTITY)
                    ImGui::Text("%s: none", label.c_str());
                else
                    ImGui::Text("%s: %u:%u", label.c_str(),
                                EntityTraits::Index(target), EntityTraits::Generation(target));
                break;
            }
            case FieldType::AssetRef: {
                AssetID id;
                std::memcpy(&id, ptr, sizeof(AssetID));

                const StableKey hint = AssetTypeHints::Lookup(componentName, field.name);
                const Assets::AssetInfo* current = Assets::Find(id);
                const char* preview = id.IsNull() ? "(none)"
                    : current ? current->name.c_str() : "(missing)";

                if (ImGui::BeginCombo(label.c_str(), preview)) {
                    if (ImGui::Selectable("(none)", id.IsNull())) {
                        const AssetID null{};
                        std::memcpy(ptr, &null, sizeof(AssetID));
                    }

                    std::vector<const Assets::AssetInfo*> candidates;
                    if (hint != 0) {
                        candidates = Assets::FindByType(hint);
                    }
                    else { // unhinted field: everything (sorted for stable UI)
                        Assets::ForEach([&](const Assets::AssetInfo& info) {
                            candidates.push_back(&info);
                        });
                        std::sort(candidates.begin(), candidates.end(),
                            [](const auto* a, const auto* b) { return a->name < b->name; });
                    }

                    for (const Assets::AssetInfo* info : candidates) {
                        ImGui::PushID(static_cast<int>(info->id.value)); // stems can repeat across folders
                        if (ImGui::Selectable(info->name.c_str(), info->id == id))
                            std::memcpy(ptr, &info->id, sizeof(AssetID));
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("%s\n%016llx", info->virtualPath.c_str(),
                                              static_cast<unsigned long long>(info->id.value));
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::IsItemHovered()) { // closed-combo tooltip: full identity
                    if (current)
                        ImGui::SetTooltip("%s\n%016llx", current->virtualPath.c_str(),
                                          static_cast<unsigned long long>(id.value));
                    else if (!id.IsNull())
                        ImGui::SetTooltip("broken reference\n%016llx",
                                          static_cast<unsigned long long>(id.value));
                }
                break;
            }
            case FieldType::Char:
                break; // unreachable?
        }
        ImGui::PopID();
    }
}
} // Lutum