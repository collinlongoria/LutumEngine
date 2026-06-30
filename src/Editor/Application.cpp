/*
* File: Application.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Application.hpp"

#include "Lutum/Platform/PlatformContext.hpp"
#include "Lutum/Platform/Window.hpp"

namespace Lutum {
Application::Application() = default;
Application::~Application() = default;

bool Application::Initialize() {
    m_platform = std::make_unique<Lutum::PlatformContext>();
    if (!m_platform->IsValid())
        return false;

    m_window = std::make_unique<Lutum::Window>("Lutum", 1280, 720);
    if (!m_window)
        return false;

    return true;
}

void Application::Run() {
    while (!m_window->ShouldClose()) {
        m_window->PollEvents();
    }
}

void Application::Terminate() {

}
} // Lutum