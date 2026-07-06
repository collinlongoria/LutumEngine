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
#include <functional>

struct SDL_Window;
union SDL_Event;

namespace Lutum {

// Resource: describes the surface the scene camera renders to
// In the editor the viewport panel writes it; in a game runtime the window does
struct ViewportInfo {
    static constexpr const char* kCuriaName = "ViewportInfo";
    
    uint32_t width = 0;
    uint32_t height = 0;
    bool hovered = false;
    bool focused = false;
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
    void RequestClose() { m_shouldClose = true; }
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
    void Maximize();
    [[nodiscard]]
    bool IsMaximized() const;

    // Called for every SDL event before Window's own handling.
    void SetEventHook(std::function<void(const SDL_Event&)> hook) { m_eventHook = std::move(hook); }

private:
    SDL_Window *m_window = nullptr;

    std::function<void(const SDL_Event&)> m_eventHook;

    bool m_minimized = false;
    bool m_shouldClose = false;
};
} // Lutum

#endif //LUTUM_WINDOW_HPP
