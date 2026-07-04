/*
* File: EditorUI.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_EDITORUI_HPP
#define LUTUM_EDITORUI_HPP
#include <cstdint>

namespace Lutum {
class RenderTarget;
namespace Curia { class Registry; class Scheduler; }

class EditorUI {
public:
    // Build all panels for this frame
    // Call between Debug::UI::BeginFrame and the renderer's UI pass
    void Draw(Curia::Registry& registry, Curia::Scheduler& scheduler, RenderTarget& sceneTarget);

private:
    void DrawViewport(Curia::Registry& registry, RenderTarget& sceneTarget);
    void DrawStats(Curia::Registry& registry);

    // Resize debounce: apply only after the requested size is stable
    uint32_t m_pendingWidth = 0;
    uint32_t m_pendingHeight = 0;
    uint32_t m_stableFrames = 0;
};
} // Lutum

#endif //LUTUM_EDITORUI_HPP
