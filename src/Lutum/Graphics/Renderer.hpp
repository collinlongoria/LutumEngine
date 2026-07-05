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

#include "Texture.hpp"
#include "Lutum/Graphics/Buffer.hpp"
#include "Lutum/Graphics/GraphicsPipeline.hpp"
#include "Lutum/ECS/Query.hpp"
#include "Lutum/Scene/Camera.hpp"
#include "Lutum/Scene/Transform.hpp"
#include "Lutum/Graphics/RenderTarget.hpp"

struct SDL_GPUSampler;

namespace Lutum {

namespace Curia { class Registry; }
class GraphicsDevice;
class ShaderCompiler;
class Shader;

class Renderer {
public:
    explicit Renderer(GraphicsDevice& device);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Initialize(const RenderTarget& sceneTarget);

    // Frame flow: BeginFrame -> RenderScene (0+ times) -> RenderDebugUI (optional) -> EndFrame
    // BeginFrame false = skip frame entirely
    bool BeginFrame();
    void RenderScene(Curia::Registry& registry, RenderTarget& target);
    void RenderDebugUI();
    void EndFrame();

    void Shutdown();

private:
    GraphicsDevice* m_device = nullptr;

    bool m_initialized = false;

    std::unique_ptr<ShaderCompiler> m_shaderCompiler;

    SDL_GPUCommandBuffer* m_cmd = nullptr;
    SDL_GPUTexture* m_swapchainTexture = nullptr;
    bool m_swapchainDrawn = false; // decides LOAD vs CLEAR for the UI pass

    GraphicsPipeline m_placeholderPipeline;
    Buffer m_placeholderVBO;
    RenderTarget m_sceneTarget;
    Texture m_placeholderTexture;
    Buffer m_placeholderIBO;
    SDL_GPUSampler* m_sampler = nullptr;

    Curia::Query<Transform, CameraComponent, Curia::With<ActiveCamera>> m_cameraQuery;
};
} // Lutum

#endif //LUTUM_RENDERER_HPP
