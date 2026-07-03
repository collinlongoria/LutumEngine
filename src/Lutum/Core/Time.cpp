/*
* File: Time.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Core/Time.hpp"

#include <SDL3/SDL_timer.h>

namespace Lutum {
Time::Time() {
    m_lastTicksNS = SDL_GetTicksNS();
}

void Time::Tick() {
    const uint64_t now = SDL_GetTicksNS();
    const uint64_t deltaNS = now - m_lastTicksNS;
    m_lastTicksNS = now;

    m_deltaSeconds = static_cast<float>(deltaNS) * 1e-9f;
    m_elapsedSeconds += static_cast<double>(deltaNS) * 1e-9f;

    // Clamp avoids huge steps after breakpoints / window drags
    if (m_deltaSeconds > 0.25f)
        m_deltaSeconds = 0.25f;
}
} // Lutum