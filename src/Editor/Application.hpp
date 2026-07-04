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

#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Scheduler.hpp"
#include "Editor/EditorUI.hpp"
#include "Lutum/Graphics/RenderTarget.hpp"

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

    Curia::Registry m_registry;
    Curia::Scheduler m_scheduler{m_registry};

    RenderTarget m_sceneTarget;
    EditorUI m_editorUI;

    bool m_shutdown = false;
};
} // Lutum

#endif //LUTUM_APP_HPP
