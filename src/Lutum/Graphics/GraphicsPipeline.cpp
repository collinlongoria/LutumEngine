/*
* File: GraphicsPipeline.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/GraphicsPipeline.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"
#include "Lutum/Platform/Window.hpp"


namespace Lutum {

static SDL_GPUPrimitiveType ToSDLPrimitive(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::TRIANGLE_LIST:  return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        case PrimitiveType::TRIANGLE_STRIP: return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
        case PrimitiveType::LINE_LIST:      return SDL_GPU_PRIMITIVETYPE_LINELIST;
        case PrimitiveType::LINE_STRIP:     return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
        case PrimitiveType::POINT_LIST:     return SDL_GPU_PRIMITIVETYPE_POINTLIST;
    }

    SDL_assert(false);
    return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
}

GraphicsPipeline::GraphicsPipeline(GraphicsDevice &device, SDL_GPUGraphicsPipeline *pipeline)
    : m_device(&device), m_pipeline(pipeline)
{
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&other) noexcept
    : m_device(other.m_device), m_pipeline(other.m_pipeline)
{
    other.m_device = nullptr;
    other.m_pipeline = nullptr;
}

GraphicsPipeline &GraphicsPipeline::operator=(GraphicsPipeline &&other) noexcept {
    if (this != &other) {
        Release();

        m_device = other.m_device;
        m_pipeline = other.m_pipeline;

        other.m_device = nullptr;
        other.m_pipeline = nullptr;
    }

    return *this;
}

std::optional<GraphicsPipeline> GraphicsPipeline::Create(GraphicsDevice &device, const CreateInfo &info) {
    if (!info.vertexShader || !info.vertexShader->IsValid() ||
        !info.fragmentShader || !info.fragmentShader->IsValid()) {
        SDL_Log("GraphicsPipeline::Create called with invalid shaders");
        return std::nullopt;
        }

    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(
        device.NativeHandle(),
        device.GetWindow().NativeHandle()
    );

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = info.vertexShader->NativeHandle();
    pipelineInfo.fragment_shader = info.fragmentShader->NativeHandle();
    pipelineInfo.primitive_type = ToSDLPrimitive(info.primitiveType);
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device.NativeHandle(), &pipelineInfo);
    if (!pipeline) {
        SDL_Log("SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        return std::nullopt;
    }

    return GraphicsPipeline{device, pipeline};
}

void GraphicsPipeline::Release() {
    if (m_pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(m_device->NativeHandle(), m_pipeline);
        m_pipeline = nullptr;
    }
}

GraphicsPipeline::~GraphicsPipeline() {
    Release();
}

} // Lutum