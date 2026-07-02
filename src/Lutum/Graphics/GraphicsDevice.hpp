/*
* File: GraphicsDevice.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_GRAPHICSDEVICE_HPP
#define LUTUM_GRAPHICSDEVICE_HPP

struct SDL_GPUDevice;

namespace Lutum {

class Window;

class GraphicsDevice {
public:
    explicit GraphicsDevice(Window& window);
    ~GraphicsDevice();

    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

    bool Initialize();
    void Shutdown();

    [[nodiscard]]
    SDL_GPUDevice* NativeHandle() const { return m_device; }
    [[nodiscard]]
    Window& GetWindow() const { return *m_window; }

private:
    Window* m_window = nullptr;
    SDL_GPUDevice* m_device = nullptr;
    bool m_windowClaimed = false;
};
} // Lutum

#endif //LUTUM_GRAPHICSDEVICE_HPP
