/*
* File: Input.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Platform/Input.hpp"

#include <cstring>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_log.h>

#include "Lutum/Platform/Window.hpp"

namespace Lutum {

static SDL_Scancode ToSDLScancode(Key key) {
    switch (key) {
        case Key::W:
            return SDL_SCANCODE_W;
        case Key::A:
            return SDL_SCANCODE_A;
        case Key::S:
            return SDL_SCANCODE_S;
        case Key::D:
            return SDL_SCANCODE_D;
        case Key::Q:
            return SDL_SCANCODE_Q;
        case Key::E:
            return SDL_SCANCODE_E;
        case Key::SPACE:
            return SDL_SCANCODE_SPACE;
        case Key::LSHIFT:
            return SDL_SCANCODE_LSHIFT;
        case Key::LCTRL:
            return SDL_SCANCODE_LCTRL;
        case Key::ESCAPE:
            return SDL_SCANCODE_ESCAPE;
        case Key::COUNT:
            break;
    }

    return SDL_SCANCODE_UNKNOWN;
}

void Input::Update() {
    m_wheelDelta = m_wheelAccum;
    m_wheelAccum = 0.0f;

    std::memcpy(m_prevKeys, m_keys, sizeof(m_keys));
    std::memcpy(m_prevMouseButtons, m_mouseButtons, sizeof(m_mouseButtons));

    // Keyboard
    const bool* sdlKeys = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < static_cast<int>(Key::COUNT); ++i) {
        const SDL_Scancode scancode = ToSDLScancode(static_cast<Key>(i));
        m_keys[i] = (scancode != SDL_SCANCODE_UNKNOWN) && sdlKeys[scancode];
    }

    // Mouse
    float dx = 0.0f, dy = 0.0f;
    const SDL_MouseButtonFlags buttons = SDL_GetRelativeMouseState(&dx, &dy);
    m_mouseDelta = m_relativeMode ? Vec2(dx, dy) : Vec2(0.0f);

    m_mouseButtons[static_cast<int>(MouseButton::LEFT)]   = (buttons & SDL_BUTTON_LMASK) != 0;
    m_mouseButtons[static_cast<int>(MouseButton::RIGHT)]  = (buttons & SDL_BUTTON_RMASK) != 0;
    m_mouseButtons[static_cast<int>(MouseButton::MIDDLE)] = (buttons & SDL_BUTTON_MMASK) != 0;
}

bool Input::IsKeyDown(Key key) const {
    return m_keys[static_cast<int>(key)];
}

bool Input::WasKeyPressed(Key key) const {
    const int i = static_cast<int>(key);
    return m_keys[i] && !m_prevKeys[i];
}

bool Input::IsMouseButtonDown(MouseButton button) const {
    return m_mouseButtons[static_cast<int>(button)];
}

bool Input::WasMouseButtonPressed(MouseButton button) const {
    const int i = static_cast<int>(button);
    return m_mouseButtons[i] && !m_prevMouseButtons[i];
}

void Input::SetRelativeMouseMode(Window &window, bool enabled) {
    if (!SDL_SetWindowRelativeMouseMode(window.NativeHandle(), enabled)) {
        SDL_Log("SDL_SetWindowRelativeMouseMode failed: %s", SDL_GetError());
        return;
    }
    m_relativeMode = enabled;

    // Flush accumulated delta so the camera doesn't jump on capture
    float dx, dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    m_mouseDelta = Vec2(0.0f);
}

} // Lutum