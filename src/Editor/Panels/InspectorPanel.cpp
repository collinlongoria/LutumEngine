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

#include <cstring>
#include <string>

#include <imgui.h>

#include "Editor/EditorContext.hpp"
#include "Lutum/Core/Math.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Reflect.hpp"

namespace Lutum {

using namespace Curia;

void InspectorPanel::Draw(Registry& registry, EditorContext& context) {
    if (ImGui::Begin("Inspector", &context.showInspector)) {
        const Entity e = context.selectedEntity;
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

            if (ImGui::CollapsingHeader(info.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                if (info.size == 0) {
                    ImGui::TextDisabled("(tag)");
                } else if (info.fields.empty()) {
                    ImGui::TextDisabled("(no reflected fields, %zu bytes)", info.size);
                } else {
                    std::byte* base = static_cast<std::byte*>(registry.GetRaw(e, cid));
                    for (size_t i = 0; i < info.fields.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i));
                        DrawField(info.fields[i], base);
                        ImGui::PopID();
                    }
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void InspectorPanel::DrawField(const FieldInfo& field, std::byte* base) {
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
        }
        ImGui::PopID();
    }
}
} // Lutum