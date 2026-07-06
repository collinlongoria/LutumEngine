/*
* File: ViewportPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_VIEWPORTPANEL_HPP
#define LUTUM_VIEWPORTPANEL_HPP
#include <cstdint>

namespace Lutum {
class RenderTarget;
namespace Curia { class Registry; }

class ViewportPanel {
public:
    void Draw(Curia::Registry& registry, RenderTarget& sceneTarget);

private:
    uint32_t m_pendingWidth = 0;
    uint32_t m_pendingHeight = 0;
    uint32_t m_stableFrames = 0;
};
} // Lutum

#endif //LUTUM_VIEWPORTPANEL_HPP
