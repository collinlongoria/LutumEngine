/*
* File: EditorUI.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/EditorUI.hpp"

#include <imgui.h>

#include "Lutum/Core/Time.hpp"
#include "Lutum/Debug/DebugUI.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Scheduler.hpp"
#include "Lutum/Graphics/RenderTarget.hpp"
#include "Lutum/Platform/Window.hpp"
#include "Lutum/Scene/Camera.hpp"

namespace Lutum {

static constexpr uint32_t kResizeStableFrames = 10;
static constexpr uint32_t kMinViewportSize = 16;

void EditorUI::Draw(Curia::Registry& registry, Curia::Scheduler& scheduler, RenderTarget& sceneTarget) {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    DrawViewport(registry, sceneTarget);
    DrawStats(registry);

    // Scheduler phases, straight from LogGraph's data (see Scheduler accessor)
    if (ImGui::Begin("Systems")) {
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

void EditorUI::DrawViewport(Curia::Registry& registry, RenderTarget& sceneTarget) {
    ViewportInfo& viewport = registry.GetResource<ViewportInfo>();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    const bool open = ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    if (open) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const uint32_t wantW = static_cast<uint32_t>(avail.x > 0 ? avail.x : 0);
        const uint32_t wantH = static_cast<uint32_t>(avail.y > 0 ? avail.y : 0);

        // Debounce: resize the target only after the panel size holds still.
        if (wantW >= kMinViewportSize && wantH >= kMinViewportSize &&
            (wantW != sceneTarget.Width() || wantH != sceneTarget.Height())) {
            if (wantW == m_pendingWidth && wantH == m_pendingHeight) {
                if (++m_stableFrames >= kResizeStableFrames) {
                    sceneTarget.Resize(wantW, wantH);
                    m_stableFrames = 0;
                }
            } else {
                m_pendingWidth = wantW;
                m_pendingHeight = wantH;
                m_stableFrames = 0;
            }
        }

        // Bind whatever the target currently is
        SDL_GPUTexture* colorTex = sceneTarget.ColorTexture();

        if (colorTex) {
            // Pass the SDL_GPUTexture* directly
            ImGui::Image(reinterpret_cast<ImTextureID>(colorTex),
                         ImVec2(static_cast<float>(sceneTarget.Width()),
                                static_cast<float>(sceneTarget.Height())));
        }

        viewport.width = sceneTarget.Width();
        viewport.height = sceneTarget.Height();
        viewport.hovered = ImGui::IsItemHovered();
        viewport.focused = ImGui::IsWindowFocused();
    }
    ImGui::End();
}

void EditorUI::DrawStats(Curia::Registry& registry) {
    if (ImGui::Begin("Stats")) {
        const Time& time = registry.GetResource<Time>();
        ImGui::Text("Frame: %.2f ms (%.0f fps)",
                    time.DeltaSeconds() * 1000.0f,
                    time.DeltaSeconds() > 0.0f ? 1.0f / time.DeltaSeconds() : 0.0f);
        ImGui::Text("Entities: %zu", registry.EntityCount());
        ImGui::Text("Archetypes: %zu", registry.ArchetypeCount());
    }
    ImGui::End();
}
} // Lutum
