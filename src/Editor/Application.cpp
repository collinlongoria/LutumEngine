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

#include "Lutum/Core/Log.hpp"
#include "Lutum/Platform/PlatformContext.hpp"
#include "Lutum/Platform/Window.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Renderer.hpp"

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

    m_graphicsDevice = std::make_unique<Lutum::GraphicsDevice>(*m_window);
    if (!m_graphicsDevice)
        return false;
    if (!m_graphicsDevice->Initialize())
        return false;

    m_renderer = std::make_unique<Lutum::Renderer>(*m_graphicsDevice);
    if (!m_renderer->Initialize())
        return false;

    m_camera.SetPosition(Vec3(0.0f, 0.0f, 2.0f));
    m_camera.SetPerspective(
        glm::radians(60.0f),
        static_cast<float>(m_window->DrawableWidth()) / static_cast<float>(m_window->DrawableHeight()),
        0.1f,
        1000.0f
    );
    m_input.SetRelativeMouseMode(*m_window, true);

    Lutum::Log::Initialize();

    return true;
}

void Application::Run() {
    while (!m_window->ShouldClose()) {
        m_time.Tick();
        m_window->PollEvents();
        m_input.Update();

        // Esc releases the cursor, click recaptures
        if (m_input.WasKeyPressed(Key::ESCAPE))
            m_input.SetRelativeMouseMode(*m_window, false);
        if (!m_input.IsRelativeMouseMode() && m_input.WasMouseButtonPressed(MouseButton::LEFT))
            m_input.SetRelativeMouseMode(*m_window, true);

        m_cameraController.Update(m_camera, m_input, m_time.DeltaSeconds());

        m_camera.SetAspect(
            static_cast<float>(m_window->DrawableWidth()) /
            static_cast<float>(m_window->DrawableHeight())
        );

        m_renderer->RenderFrame(m_camera);
    }
}

void Application::Terminate() {
    m_renderer.reset();
    m_graphicsDevice.reset();
    m_window.reset();

    Lutum::Log::Shutdown();
}
} // Lutum