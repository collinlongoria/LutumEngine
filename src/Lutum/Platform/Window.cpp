/*
* File: Window.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Platform/Window.hpp"

#include <SDL3/SDL.h>

namespace Lutum {
Window::Window(const char *title, int width, int height) {
    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    }
}

Window::~Window() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void Window::PollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                m_shouldClose = true;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                m_shouldClose = true;
                break;

            case SDL_EVENT_WINDOW_MINIMIZED:
                m_minimized = true;
                break;

            case SDL_EVENT_WINDOW_RESTORED:
                m_minimized = false;
                break;

            default:
                break;
        }
    }
}

int Window::Height() const {
    int height;
    SDL_GetWindowSize(m_window, nullptr, &height);
    return height;
}

int Window::Width() const {
    int width;
    SDL_GetWindowSize(m_window, &width, nullptr);
    return width;
}

int Window::DrawableHeight() const {
    int height;
    SDL_GetWindowSizeInPixels(m_window, nullptr, &height);
    return height;
}

int Window::DrawableWidth() const {
    int width;
    SDL_GetWindowSizeInPixels(m_window, &width, nullptr);
    return width;
}
} // Lutum