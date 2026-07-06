/*
* File: LogPanel.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Panels/LogPanel.hpp"

#include "Editor/EditorContext.hpp"

namespace Lutum {

static constexpr size_t kMaxLocalEntries = 8192;

static ImVec4 LevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return {0.5f, 0.5f, 0.5f, 1.0f};
        case LogLevel::Debug: return {0.4f, 0.8f, 0.9f, 1.0f};
        case LogLevel::Warn:  return {0.9f, 0.8f, 0.3f, 1.0f};
        case LogLevel::Error: return {0.9f, 0.4f, 0.4f, 1.0f};
        case LogLevel::Fatal: return {1.0f, 0.2f, 0.2f, 1.0f};
        default:              return {0.9f, 0.9f, 0.9f, 1.0f};
    }
}

void LogPanel::Draw(EditorContext& context) {
    if (ImGui::Begin("Log", &context.showLog)) {
        m_serial = Log::ReadRing(m_serial, m_entries);
        if (m_entries.size() > kMaxLocalEntries)
            m_entries.erase(m_entries.begin(),
                            m_entries.begin() + static_cast<long>(m_entries.size() / 2));

        static const char* kLevels[] = {"Trace", "Debug", "Info", "Warn", "Error", "Fatal"};
        ImGui::SetNextItemWidth(90.0f);
        ImGui::Combo("##minlevel", &m_minLevel, kLevels, 6);
        ImGui::SameLine();
        m_filter.Draw("##filter", 160.0f);
        ImGui::SameLine();
        ImGui::Checkbox("Autoscroll", &m_autoScroll);
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            m_entries.clear();

        ImGui::Separator();
        if (ImGui::BeginChild("##scroll", ImVec2(0, 0), 0,
                              ImGuiWindowFlags_HorizontalScrollbar)) {
            for (const Log::RingEntry& entry : m_entries) {
                if (static_cast<int>(entry.level) < m_minLevel)
                    continue;
                if (!m_filter.PassFilter(entry.text.c_str()))
                    continue;
                ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(entry.level));
                ImGui::TextUnformatted(entry.text.c_str());
                ImGui::PopStyleColor();
            }
            if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
} // Lutum