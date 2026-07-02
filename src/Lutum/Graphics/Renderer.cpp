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

#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"
#include "Lutum/Graphics/ShaderCompiler.hpp"
#include "Lutum/Platform/Window.hpp"
#include "SDL3/SDL_log.h"

namespace Lutum {

// TODO: move to asset files once there's a filesystem layer
static const char* kVertexHLSL = R"(
struct VSOutput {
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID) {
    float2 positions[3] = {
        float2( 0.0,  0.5),
        float2( 0.5, -0.5),
        float2(-0.5, -0.5)
    };

    float3 colors[3] = {
        float3(1, 0, 0),
        float3(0, 1, 0),
        float3(0, 0, 1)
    };

    VSOutput o;
    o.position = float4(positions[vertexID], 0.0, 1.0);
    o.color = colors[vertexID];
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

    std::optional<GraphicsPipeline> pipeline = GraphicsPipeline::Create(*m_device, pipelineInfo);
    if (!pipeline)
        return false;

    m_trianglePipeline = std::move(*pipeline);

    m_initialized = true;
    return true;
}

void Renderer::RenderFrame() {
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

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchainTexture;
    colorTarget.clear_color = SDL_FColor{0.08f, 0.08f, 0.10f, 1.0f};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(pass, m_trianglePipeline.NativeHandle());

    // Important with SV_VertexID: first_vertex must stay 0
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

    m_trianglePipeline = GraphicsPipeline{};
    m_shaderCompiler.reset();

    m_initialized = false;
}

Renderer::~Renderer() {
    Shutdown();
}
} // Lutum