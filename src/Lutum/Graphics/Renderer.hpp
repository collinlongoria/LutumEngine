/*
* File: Renderer.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/1/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_RENDERER_HPP
#define LUTUM_RENDERER_HPP
#include <memory>

#include "Lutum/Graphics/Buffer.hpp"
#include "Lutum/Graphics/GraphicsPipeline.hpp"

namespace Lutum {

class GraphicsDevice;
class ShaderCompiler;
class Shader;

class Renderer {
public:
    explicit Renderer(GraphicsDevice& device);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Initialize();
    void RenderFrame();
    void Shutdown();

private:
    GraphicsDevice* m_device = nullptr;

    bool m_initialized = false;

    std::unique_ptr<ShaderCompiler> m_shaderCompiler;
    GraphicsPipeline m_trianglePipeline;
    Buffer m_triangleVBO;
};
} // Lutum

#endif //LUTUM_RENDERER_HPP
