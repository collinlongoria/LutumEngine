/*
* File: Window.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_WINDOW_HPP
#define LUTUM_WINDOW_HPP
#include <cstdint>

struct SDL_Window;

namespace Lutum {

// Resource: written by Application each frame, read by systems
struct WindowInfo {
    uint32_t drawableWidth = 0;
    uint32_t drawableHeight = 0;
};

class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]]
    SDL_Window* NativeHandle() const { return m_window; }

    [[nodiscard]]
    bool ShouldClose() const { return m_shouldClose; }
    void PollEvents();

    [[nodiscard]]
    int Width() const;
    [[nodiscard]]
    int Height() const;
    [[nodiscard]]
    int DrawableWidth() const;
    [[nodiscard]]
    int DrawableHeight() const;
    [[nodiscard]]
    bool IsMinimized() const { return m_minimized; }

private:
    SDL_Window *m_window = nullptr;
    bool m_minimized = false;
    bool m_shouldClose = false;
};
} // Lutum

#endif //LUTUM_WINDOW_HPP
