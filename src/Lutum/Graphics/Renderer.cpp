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

#include "ImageIO.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Debug/DebugUI.hpp"
#include "Lutum/Graphics/GpuUploader.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"
#include "Lutum/Graphics/ShaderCompiler.hpp"
#include "Lutum/Platform/Window.hpp"
#include "SDL3/SDL_log.h"

namespace Lutum {

struct FrameUniforms {
    Mat4 viewProj;
};

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

    auto vertSource = FileSystem::ReadText("/Engine/shaders/basic.vert.hlsl");
    auto fragSource = FileSystem::ReadText("/Engine/shaders/basic.frag.hlsl");
    if (!vertSource || !fragSource) {
        LUTUM_ERROR("Renderer: engine shaders missing");
        return false;
    }

    std::optional<Shader> vert = m_shaderCompiler->LoadHLSL(*m_device, vertSource->c_str(), ShaderStage::VERTEX);
    std::optional<Shader> frag = m_shaderCompiler->LoadHLSL(*m_device, fragSource->c_str(), ShaderStage::FRAGMENT);
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

    // Vertex & Index buffer
    const Vertex vertices[] = {
        // +Z front
        {{-0.5f,-0.5f, 0.5f},{0,1}}, {{ 0.5f,-0.5f, 0.5f},{1,1}}, {{ 0.5f, 0.5f, 0.5f},{1,0}}, {{-0.5f, 0.5f, 0.5f},{0,0}},
        // -Z back
        {{ 0.5f,-0.5f,-0.5f},{0,1}}, {{-0.5f,-0.5f,-0.5f},{1,1}}, {{-0.5f, 0.5f,-0.5f},{1,0}}, {{ 0.5f, 0.5f,-0.5f},{0,0}},
        // +X right
        {{ 0.5f,-0.5f, 0.5f},{0,1}}, {{ 0.5f,-0.5f,-0.5f},{1,1}}, {{ 0.5f, 0.5f,-0.5f},{1,0}}, {{ 0.5f, 0.5f, 0.5f},{0,0}},
        // -X left
        {{-0.5f,-0.5f,-0.5f},{0,1}}, {{-0.5f,-0.5f, 0.5f},{1,1}}, {{-0.5f, 0.5f, 0.5f},{1,0}}, {{-0.5f, 0.5f,-0.5f},{0,0}},
        // +Y top
        {{-0.5f, 0.5f, 0.5f},{0,1}}, {{ 0.5f, 0.5f, 0.5f},{1,1}}, {{ 0.5f, 0.5f,-0.5f},{1,0}}, {{-0.5f, 0.5f,-0.5f},{0,0}},
        // -Y bottom
        {{-0.5f,-0.5f,-0.5f},{0,1}}, {{ 0.5f,-0.5f,-0.5f},{1,1}}, {{ 0.5f,-0.5f, 0.5f},{1,0}}, {{-0.5f,-0.5f, 0.5f},{0,0}},
    };

    uint16_t indices[36];
    for (uint16_t face = 0; face < 6; ++face) {
        const uint16_t base = face * 4;
        const uint16_t faceIndices[6] = {
            base, uint16_t(base + 1), uint16_t(base + 2),
            base, uint16_t(base + 2), uint16_t(base + 3)
        };
        std::memcpy(indices + face * 6, faceIndices, sizeof(faceIndices));
    }

    auto image = ImageIO::Load("/Game/textures/test.png");
    if (!image) {
        LUTUM_ERROR("Renderer: failed to load test texture");
        return false;
    }

    Texture::CreateInfo texInfo = {};
    texInfo.width = image->width;
    texInfo.height = image->height;
    std::optional<Texture> texture = Texture::Create(*m_device, texInfo, "test_texture");
    if (!texture)
        return false;
    m_placeholderTexture = std::move(*texture);

    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.max_lod = 1000.0f;
    m_sampler = SDL_CreateGPUSampler(m_device->NativeHandle(), &samplerInfo);
    if (!m_sampler)
        return false;

    // buffers
    std::optional<Buffer> vbo = Buffer::Create(*m_device, BufferUsage::VERTEX, sizeof(vertices), "placeholder_vbo");
    if (!vbo) return false;
    m_placeholderVBO = std::move(*vbo);

    std::optional<Buffer> ibo = Buffer::Create(*m_device, BufferUsage::INDEX, sizeof(indices), "placeholder_ibo");
    if (!ibo) return false;
    m_placeholderIBO = std::move(*ibo);

    GpuUploader uploader(*m_device);
    if (!uploader.Begin()) return false;
    if (!uploader.Upload(m_placeholderVBO, vertices, sizeof(vertices))) return false;
    if (!uploader.Upload(m_placeholderIBO, indices, sizeof(indices))) return false;
    if (!uploader.Upload(m_placeholderTexture, image->pixels.data(),
                         static_cast<uint32_t>(image->pixels.size()))) return false;
    if (!uploader.End()) return false;

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

        SDL_GPUBufferBinding indexBinding = {m_placeholderIBO.NativeHandle(), 0};
        SDL_BindGPUIndexBuffer(pass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        SDL_GPUTextureSamplerBinding textureBinding = {m_placeholderTexture.NativeHandle(), m_sampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &textureBinding, 1);

        SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
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

    SDL_ReleaseGPUSampler(m_device->NativeHandle(), m_sampler);
    m_placeholderTexture = Texture{};
    m_placeholderIBO = Buffer{};
    m_placeholderVBO = Buffer{};
    m_placeholderPipeline = GraphicsPipeline{};
    m_shaderCompiler.reset();

    m_initialized = false;
}

Renderer::~Renderer() {
    Shutdown();
}
} // Lutum