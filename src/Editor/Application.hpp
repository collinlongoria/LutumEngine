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

namespace Lutum {

class Window;
class PlatformContext;

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize();
    void Run();
    void Terminate();

private:
    std::unique_ptr<Lutum::PlatformContext> m_platform;
    std::unique_ptr<Lutum::Window> m_window;
};
} // Lutum

#endif //LUTUM_APP_HPP
