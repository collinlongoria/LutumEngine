/*
* File: Input.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_INPUT_HPP
#define LUTUM_INPUT_HPP
#include <Lutum/Core/Math.hpp>

namespace Lutum {

class Window;

// TODO: Implement all key bindings
enum class Key {
    W, A, S, D,
    Q, E,
    SPACE,
    LSHIFT,
    LCTRL,
    ESCAPE,

    COUNT
};

// TODO: Implement support for extra mouse buttons
enum class MouseButton {
    LEFT,
    RIGHT,
    MIDDLE,

    COUNT
};

class Input {
public:
    static constexpr const char* kCuriaName = "Input";

    Input() = default;

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    // Called once per frame, after Window::PollEvents
    void Update();

    [[nodiscard]]
    bool IsKeyDown(Key key) const;
    [[nodiscard]]
    bool WasKeyPressed(Key key) const; // i.e. down this frame, up last frame
    [[nodiscard]]
    bool IsMouseButtonDown(MouseButton button) const;
    [[nodiscard]]
    bool WasMouseButtonPressed(MouseButton button) const;

    // Per-frame mouse movement
    [[nodiscard]]
    Vec2 MouseDelta() const { return m_mouseDelta; }

    // Captures the cursor and hides it
    void SetRelativeMouseMode(Window& window, bool enabled);
    [[nodiscard]]
    bool IsRelativeMouseMode() const { return m_relativeMode; }

private:
    bool m_keys[static_cast<int>(Key::COUNT)] = {};
    bool m_prevKeys[static_cast<int>(Key::COUNT)] = {};

    bool m_mouseButtons[static_cast<int>(MouseButton::COUNT)] = {};
    bool m_prevMouseButtons[static_cast<int>(MouseButton::COUNT)] = {};

    Vec2 m_mouseDelta = Vec2(0.0f);
    bool m_relativeMode = false;

};
} // Lutum

#endif //LUTUM_INPUT_HPP
