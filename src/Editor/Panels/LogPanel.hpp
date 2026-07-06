/*
* File: LogPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_LOGPANEL_HPP
#define LUTUM_LOGPANEL_HPP
#include <cstdint>
#include <vector>

#include <imgui.h>

#include "Lutum/Core/Log.hpp"

namespace Lutum {
struct EditorContext;

class LogPanel {
public:
    void Draw(EditorContext& context);

private:
    std::vector<Log::RingEntry> m_entries;
    uint64_t m_serial = 0;
    int m_minLevel = 0; // Trace
    bool m_autoScroll = true;
    ImGuiTextFilter m_filter;
};
} // Lutum

#endif
