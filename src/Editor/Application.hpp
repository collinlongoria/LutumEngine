/*
* File: Application.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_APP_HPP
#define LUTUM_APP_HPP

#include <memory>

#include "Lutum/Core/Time.hpp"
#include "Lutum/Platform/Input.hpp"
#include "Lutum/Graphics/Camera.hpp"
#include "Lutum/Graphics/FlyCameraController.hpp"

namespace Lutum {

class GraphicsDevice;
class Window;
class PlatformContext;
class Renderer;

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize();
    void Run();
    void Shutdown();

private:
    std::unique_ptr<Lutum::PlatformContext> m_platform;
    std::unique_ptr<Lutum::Window> m_window;
    std::unique_ptr<Lutum::GraphicsDevice> m_graphicsDevice;
    std::unique_ptr<Lutum::Renderer> m_renderer;

    Time m_time;
    Input m_input;
    Camera m_camera;
    FlyCameraController m_cameraController;

    bool m_shutdown = false;
};
} // Lutum

#endif //LUTUM_APP_HPP
