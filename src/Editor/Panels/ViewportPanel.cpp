/*
* File: ViewportPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/ViewportPanel.hpp"

#include <imgui.h>

#include "Lutum/ECS/Registry.hpp"
#include "Lutum/Graphics/RenderTarget.hpp"
#include "Lutum/Platform/Window.hpp"

namespace Lutum {

static constexpr uint32_t kResizeStableFrames = 10;
static constexpr uint32_t kMinViewportSize = 16;

void ViewportPanel::Draw(Curia::Registry &registry, RenderTarget &sceneTarget) {
    ViewportInfo& viewport = registry.GetResource<ViewportInfo>();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    const bool open = ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    if (open) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const uint32_t wantW = static_cast<uint32_t>(avail.x > 0 ? avail.x : 0);
        const uint32_t wantH = static_cast<uint32_t>(avail.y > 0 ? avail.y : 0);

        // resize the target only after the panel size holds still
        if (wantW >= kMinViewportSize && wantH >= kMinViewportSize && (wantW != sceneTarget.Width() || wantH != sceneTarget.Height())) {
            if (wantW == m_pendingWidth && wantH == m_pendingHeight) {
                if (++m_stableFrames >= kResizeStableFrames) {
                    sceneTarget.Resize(wantW, wantH);
                    m_stableFrames = 0;
                }
            }
            else {
                m_pendingWidth = wantW;
                m_pendingHeight = wantH;
                m_stableFrames = 0;
            }
        }

        // Bind whatever the target currently is
        SDL_GPUTexture* colorTex = sceneTarget.ColorTexture();

        if (colorTex) {
            ImGui::Image(reinterpret_cast<ImTextureID>(colorTex), ImVec2(static_cast<float>(sceneTarget.Width()), static_cast<float>(sceneTarget.Height())));
        }

        viewport.width = sceneTarget.Width();
        viewport.height = sceneTarget.Height();
        viewport.hovered = ImGui::IsItemHovered();
        viewport.focused = ImGui::IsWindowFocused();
    }
    ImGui::End();
}
} // Lutum