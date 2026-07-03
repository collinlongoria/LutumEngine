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

bool Renderer::Initialize() {
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

    std::optional<GraphicsPipeline> pipeline = GraphicsPipeline::Create(*m_device, pipelineInfo);
    if (!pipeline)
        return false;

    m_trianglePipeline = std::move(*pipeline);

    // Vertex buffer
    const Vertex vertices[] = {
        {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };

    std::optional<Buffer> vbo = Buffer::Create(*m_device, BufferUsage::VERTEX, sizeof(vertices), "triangle_vbo");
    if (!vbo)
        return false;

    m_triangleVBO = std::move(*vbo);

    BufferUploader uploader(*m_device);
    if (!uploader.Begin())
        return false;
    if (!uploader.Upload(m_triangleVBO, vertices, sizeof(vertices)))
        return false;
    if (!uploader.End())
        return false;

    m_initialized = true;
    return true;
}

void Renderer::RenderFrame(const Camera& camera) {
    if (!m_initialized)
        return;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_device->NativeHandle());
    if (!cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    Uint32 swapchainWidth = 0;
    Uint32 swapchainHeight = 0;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmd,
            m_device->GetWindow().NativeHandle(),
            &swapchainTexture,
            &swapchainWidth,
            &swapchainHeight))
    {
        SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    if (swapchainTexture == nullptr) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    FrameUniforms uniforms = {};
    uniforms.viewProj = camera.ViewProjectionMatrix();
    SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchainTexture;
    colorTarget.clear_color = SDL_FColor{0.08f, 0.08f, 0.10f, 1.0f};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(pass, m_trianglePipeline.NativeHandle());

    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = m_triangleVBO.NativeHandle();
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

    SDL_EndGPURenderPass(pass);

    if (!SDL_SubmitGPUCommandBuffer(cmd)) {
        SDL_Log("SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
    }
}

void Renderer::Shutdown() {
    if (!m_initialized)
        return;

    // Make sure the GPU isn't still using the pipeline before releasing it.
    SDL_WaitForGPUIdle(m_device->NativeHandle());

    m_triangleVBO = Buffer{};
    m_trianglePipeline = GraphicsPipeline{};
    m_shaderCompiler.reset();

    m_initialized = false;
}

Renderer::~Renderer() {
    Shutdown();
}
} // Lutum