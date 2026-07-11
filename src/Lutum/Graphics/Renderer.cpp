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
#include "Lutum/Assets/AssetRegistry.hpp"
#include "Lutum/Assets/MeshAsset.hpp"
#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Debug/DebugUI.hpp"
#include "Lutum/Graphics/GpuUploader.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"
#include "Lutum/Graphics/ShaderCompiler.hpp"
#include "Lutum/Platform/Window.hpp"
#include "SDL3/SDL_log.h"

namespace Lutum {

struct ObjectUniforms {
    Mat4 viewProj;
    Mat4 model;
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
    pipelineInfo.vertexLayout.stride = sizeof(MeshVertex);
    pipelineInfo.vertexLayout.attributes = {
        { 0, VertexFormat::FLOAT3, offsetof(MeshVertex, position) },
        { 1, VertexFormat::FLOAT3, offsetof(MeshVertex, normal) },
        { 2, VertexFormat::FLOAT2, offsetof(MeshVertex, uv) },
    };
    pipelineInfo.depthState.testEnabled = true;
    pipelineInfo.depthState.writeEnabled = true;
    pipelineInfo.depthState.compareOp = CompareOp::LESS;

    pipelineInfo.colorFormat = sceneTarget.NativeColorFormat();
    pipelineInfo.depthFormat = sceneTarget.NativeDepthFormat();

    std::optional<GraphicsPipeline> pipeline = GraphicsPipeline::Create(*m_device, pipelineInfo);
    if (!pipeline)
        return false;
    m_meshPipeline = std::move(*pipeline);

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

    m_meshPool.Initialize(*m_device);
    m_texturePool.Initialize(*m_device);

    if (const auto* mat = Assets::FindByPath("/Engine/Materials/DefaultMaterial.lasset"))
        m_defaultMaterial = mat->id;
    if (const auto* tex = Assets::FindByPath("/Engine/Textures/DefaultTexture.lasset"))
        m_defaultTexture = tex->id;
    if (m_defaultMaterial.IsNull() || m_defaultTexture.IsNull())
        LUTUM_WARN("Renderer: engine content missing! run Tools > Generate Engine Content");

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

    m_meshQuery.Refresh(registry);

    m_cameraQuery.Refresh(registry);
    bool hasCamera = false;
    Mat4 viewProj(1.0f);
    m_cameraQuery.Each([&](Transform& transform, CameraComponent& camera) {
        if (!hasCamera) {
            viewProj = CameraMath::ViewProjection(transform, camera);
            hasCamera = true;
        }
    });

    SDL_GPUTexture* swapchain = target.IsOffscreen() ? nullptr : m_swapchainTexture;
    SDL_GPURenderPass* pass = target.BeginRenderPass(m_cmd, swapchain, Vec4(0.08f, 0.08f, 0.10f, 1.0f));
    if (!pass)
        return;

    if (hasCamera) {
        SDL_BindGPUGraphicsPipeline(pass, m_meshPipeline.NativeHandle());

        m_meshQuery.Each([&](Transform& transform, MeshRenderer& meshRenderer) {
            const GpuMesh* mesh = m_meshPool.Get(meshRenderer.mesh);
            if (!mesh)
                return; // null/missing mesh draws nothing
            const Texture* albedo = ResolveAlbedo(meshRenderer.material);
            if (!albedo)
                return;

            ObjectUniforms uniforms;
            uniforms.viewProj = viewProj;
            uniforms.model = glm::translate(Mat4(1.0f), transform.position)
                           * glm::mat4_cast(transform.rotation)
                           * glm::scale(Mat4(1.0f), transform.scale);
            SDL_PushGPUVertexUniformData(m_cmd, 0, &uniforms, sizeof(uniforms));

            SDL_GPUBufferBinding vertexBinding = {mesh->vbo.NativeHandle(), 0};
            SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);
            SDL_GPUBufferBinding indexBinding = {mesh->ibo.NativeHandle(), 0};
            SDL_BindGPUIndexBuffer(pass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT); // u32 now
            SDL_GPUTextureSamplerBinding textureBinding = {albedo->NativeHandle(), m_sampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &textureBinding, 1);

            SDL_DrawGPUIndexedPrimitives(pass, mesh->indexCount, 1, 0, 0, 0);
        });
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

    m_meshPool.Clear();
    m_texturePool.Clear();
    m_materialPool.Clear();

    SDL_ReleaseGPUSampler(m_device->NativeHandle(), m_sampler);
    m_meshPipeline = GraphicsPipeline{};
    m_shaderCompiler.reset();

    m_initialized = false;
}

const Texture* Renderer::ResolveAlbedo(AssetID materialId) {
    if (materialId.IsNull())
        materialId = m_defaultMaterial;
    const MaterialData* material = m_materialPool.Get(materialId);
    if (!material && materialId != m_defaultMaterial)
        material = m_materialPool.Get(m_defaultMaterial); // broken ref -> default

    const AssetID albedoId = (material && !material->albedo.IsNull())
        ? material->albedo : m_defaultTexture;
    const Texture* texture = m_texturePool.Get(albedoId);
    if (!texture && albedoId != m_defaultTexture)
        texture = m_texturePool.Get(m_defaultTexture);
    return texture;
}

Renderer::~Renderer() {
    Shutdown();
}
} // Lutum