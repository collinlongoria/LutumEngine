/*
* File: PlatformContext.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "PlatformContext.hpp"

#include <SDL3/SDL.h>

namespace Lutum {
PlatformContext::PlatformContext() {
    m_valid = SDL_Init(SDL_INIT_VIDEO);
    if (!m_valid) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
    }
}

PlatformContext::~PlatformContext() {
    if (m_valid) {
        SDL_Quit();
    }
}
} // Lutum