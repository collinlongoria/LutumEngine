/*
* File: Renderer.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/1/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/Renderer.hpp"

#include <SDL3/SDL_gpu.h>

#include "Lutum/Debug/DebugUI.hpp"
#include "Lutum/Graphics/BufferUploader.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"
#include "Lutum/Graphics/ShaderCompiler.hpp"
#include "Lutum/Platform/Window.hpp"
#include "SDL3/SDL_log.h"

namespace Lutum {

struct FrameUniforms {
    Mat4 viewProj;
};

// TODO: move to asset files once there's a filesystem layer
// NOTE: SDL_shadercross requires TEXCOORDn semantics for vertex inputs.
// TEXCOORD0 -> attribute location 0, TEXCOORD1 -> location 1, etc.
// NOTE: SDL_GPU convention via shadercross: vertex uniform buffers live in space1.
// (Fragment uniforms would be space3.) register(b0, space1) -> uniform slot 0.
static const char* kVertexHLSL = R"(
cbuffer FrameUniforms : register(b0, space1) {
    float4x4 viewProj;
};

struct VSInput {
    float3 position : TEXCOORD0;
    float3 color    : TEXCOORD1;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput o;
    o.position = mul(viewProj, float4(input.position, 1.0));
    o.color = input.color;
    return o;
}
)";

static const char* kFragmentHLSL = R"(
struct PSInput {
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target0 {
    return float4(input.color, 1.0);
}
)";

struct Vertex {
    float position[3];
    float color[3];
};

Renderer::Renderer(GraphicsDevice &device)
    : m_device(&device)
{
}

bool Renderer::Initialize(const RenderTarget& sceneTarget) {
    m_shaderCompiler = std::make_unique<ShaderCompiler>();
    if (!m_shaderCompiler->Initialize())
        return false;

    std::optional<Shader> vert = m_shaderCompiler->LoadHLSL(*m_device, kVertexHLSL, ShaderStage::VERTEX);
    std::optional<Shader> frag = m_shaderCompiler->LoadHLSL(*m_device, kFragmentHLSL, ShaderStage::FRAGMENT);
    if (!vert || !frag)
        return false;

    GraphicsPipeline::CreateInfo pipelineInfo = {};
    pipelineInfo.vertexShader = &*vert;
    pipelineInfo.fragmentShader = &*frag;
    pipelineInfo.primitiveType = PrimitiveType::TRIANGLE_LIST;
    pipelineInfo.vertexLayout.stride = sizeof(Vertex);
    pipelineInfo.vertexLayout.attributes = {
            { 0, VertexFormat::FLOAT3, offsetof(Vertex, position) },
            { 1, VertexFormat::FLOAT3, offsetof(Vertex, color) },
        };
    pipelineInfo.depthState.testEnabled = true;
    pipelineInfo.depthState.writeEnabled = true;
    pipelineInfo.depthState.compareOp = CompareOp::LESS;

    pipelineInfo.colorFormat = sceneTarget.NativeColorFormat();
    pipelineInfo.depthFormat = sceneTarget.NativeDepthFormat();

    std::optional<GraphicsPipeline> pipeline = GraphicsPipeline::Create(*m_device, pipelineInfo);
    if (!pipeline)
        return false;

    m_placeholderPipeline = std::move(*pipeline);

    // Vertex buffer
    const Vertex vertices[] = {
        {{-0.6f, -0.4f, 0.0f}, {1, 0, 0}}, {{ 0.2f, -0.4f, 0.0f}, {1, 0, 0}}, {{-0.2f, 0.5f, 0.0f}, {1, 0, 0}},
        {{-0.2f, -0.4f, 0.5f}, {0, 0, 1}}, {{ 0.6f, -0.4f, 0.5f}, {0, 0, 1}}, {{ 0.2f, 0.5f, 0.5f}, {0, 0, 1}},
    };

    std::optional<Buffer> vbo = Buffer::Create(*m_device, BufferUsage::VERTEX, sizeof(vertices), "placeholder_vbo");
    if (!vbo)
        return false;
    m_placeholderVBO = std::move(*vbo);

    BufferUploader uploader(*m_device);
    if (!uploader.Begin())
        return false;
    if (!uploader.Upload(m_placeholderVBO, vertices, sizeof(vertices)))
        return false;
    if (!uploader.End())
        return false;

    m_initialized = true;
    return true;
}

bool Renderer::BeginFrame() {
    if (!m_initialized)
        return false;

    m_cmd = SDL_AcquireGPUCommandBuffer(m_device->NativeHandle());
    if (!m_cmd) {
        LUTUM_ERROR("SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
        return false;
    }

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(m_cmd, m_device->GetWindow().NativeHandle(), &m_swapchainTexture, nullptr, nullptr)) {
        LUTUM_ERROR("SDL_WaitAndAcquireGPUSwapchainTexture failed: {}", SDL_GetError());
        SDL_CancelGPUCommandBuffer(m_cmd);
        m_cmd = nullptr;
        return false;
    }

    if (!m_swapchainTexture) {
        // Swapchain unavailable (minimized). Must still submit.
        SDL_SubmitGPUCommandBuffer(m_cmd);
        m_cmd = nullptr;
        return false;
    }

    m_swapchainDrawn = false;
    return true;
}

void Renderer::RenderScene(Curia::Registry& registry, RenderTarget& target) {
    if (!m_cmd || !m_initialized)
        return;

    m_cameraQuery.Refresh(registry);
    bool hasCamera = false;
    Mat4 viewProj(1.0f);
    m_cameraQuery.Each([&](Transform& transform, CameraComponent& camera) {
        if (!hasCamera) {
            viewProj = CameraMath::ViewProjection(transform, camera);
            hasCamera = true;
        }
    });

    if (hasCamera) {
        FrameUniforms uniforms = {};
        uniforms.viewProj = viewProj;
        SDL_PushGPUVertexUniformData(m_cmd, 0, &uniforms, sizeof(uniforms));
    }

    SDL_GPUTexture* swapchain = target.IsOffscreen() ? nullptr : m_swapchainTexture;
    SDL_GPURenderPass* pass = target.BeginRenderPass(m_cmd, swapchain, Vec4(0.08f, 0.08f, 0.10f, 1.0f));
    if (!pass)
        return;

    if (hasCamera) {
        SDL_BindGPUGraphicsPipeline(pass, m_placeholderPipeline.NativeHandle());
        SDL_GPUBufferBinding vertexBinding = {m_placeholderVBO.NativeHandle(), 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);
        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    }

    SDL_EndGPURenderPass(pass);

    if (!target.IsOffscreen())
        m_swapchainDrawn = true;
}

void Renderer::RenderDebugUI() {
    if (!m_cmd || !Debug::UI::IsInitialized())
        return;

    Debug::UI::PrepareRender(m_cmd);

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = m_swapchainTexture;
    colorTarget.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f};
    colorTarget.load_op = m_swapchainDrawn ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(m_cmd, &colorTarget, 1, nullptr);
    Debug::UI::Render(m_cmd, pass);
    SDL_EndGPURenderPass(pass);
}

void Renderer::EndFrame() {
    if (!m_cmd)
        return;

    if (!SDL_SubmitGPUCommandBuffer(m_cmd)) {
        LUTUM_ERROR("SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError());
    }
    m_cmd = nullptr;
    m_swapchainTexture = nullptr;
}

void Renderer::Shutdown() {
    if (!m_initialized)
        return;

    // Make sure the GPU isn't still using the pipeline before releasing it.
    SDL_WaitForGPUIdle(m_device->NativeHandle());

    m_placeholderVBO = Buffer{};
    m_placeholderPipeline = GraphicsPipeline{};
    m_shaderCompiler.reset();

    m_initialized = false;
}

Renderer::~Renderer() {
    Shutdown();
}
} // Lutum