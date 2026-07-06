/*
* File: StatsPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_STATSPANEL_HPP
#define LUTUM_STATSPANEL_HPP

namespace Lutum {
struct EditorContext;
namespace Curia { class Registry; }

class StatsPanel {
public:
    void Draw(Curia::Registry& registry, EditorContext& context);

private:
    static constexpr int kHistorySize = 240;
    float m_history[kHistorySize] = {};
    int m_offset = 0;
};
} // Lutum

#endif //LUTUM_STATSPANEL_HPP
