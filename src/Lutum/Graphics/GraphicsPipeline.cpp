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

#include "Lutum/Core/Log.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"
#include "Lutum/Platform/Window.hpp"


namespace Lutum {

static SDL_GPUPrimitiveType ToSDLPrimitive(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::TRIANGLE_LIST:
            return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        case PrimitiveType::TRIANGLE_STRIP:
            return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
        case PrimitiveType::LINE_LIST:
            return SDL_GPU_PRIMITIVETYPE_LINELIST;
        case PrimitiveType::LINE_STRIP:
            return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
        case PrimitiveType::POINT_LIST:
            return SDL_GPU_PRIMITIVETYPE_POINTLIST;
    }

    SDL_assert(false);
    return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
}

static SDL_GPUVertexElementFormat ToSDLVertexFormat(VertexFormat format) {
    switch (format) {
        case VertexFormat::FLOAT:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
        case VertexFormat::FLOAT2:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        case VertexFormat::FLOAT3:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        case VertexFormat::FLOAT4:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        case VertexFormat::UBYTE4_NORM:
            return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        case VertexFormat::UINT:
            return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    }

    SDL_assert(false);
    return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
}

static SDL_GPUCompareOp ToSDLCompareOp(CompareOp op) {
    switch (op) {
        case CompareOp::NEVER:            return SDL_GPU_COMPAREOP_NEVER;
        case CompareOp::LESS:             return SDL_GPU_COMPAREOP_LESS;
        case CompareOp::EQUAL:            return SDL_GPU_COMPAREOP_EQUAL;
        case CompareOp::LESS_OR_EQUAL:    return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        case CompareOp::GREATER:          return SDL_GPU_COMPAREOP_GREATER;
        case CompareOp::NOT_EQUAL:        return SDL_GPU_COMPAREOP_NOT_EQUAL;
        case CompareOp::GREATER_OR_EQUAL: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
        case CompareOp::ALWAYS:           return SDL_GPU_COMPAREOP_ALWAYS;
    }

    SDL_assert(false);
    return SDL_GPU_COMPAREOP_LESS;
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
    colorTargetDesc.format = (info.colorFormat != 0) ? static_cast<SDL_GPUTextureFormat>(info.colorFormat) : SDL_GetGPUSwapchainTextureFormat(device.NativeHandle(), device.GetWindow().NativeHandle());

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = info.vertexShader->NativeHandle();
    pipelineInfo.fragment_shader = info.fragmentShader->NativeHandle();
    pipelineInfo.primitive_type = ToSDLPrimitive(info.primitiveType);
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;

    if (info.depthState.testEnabled || info.depthState.writeEnabled) {
        LUTUM_ASSERT(info.depthFormat != 0, "pipeline has depth state but no depth format; pass the target's NativeDepthFormat()");

        pipelineInfo.depth_stencil_state.enable_depth_test = info.depthState.testEnabled;
        pipelineInfo.depth_stencil_state.enable_depth_write = info.depthState.writeEnabled;
        pipelineInfo.depth_stencil_state.compare_op = ToSDLCompareOp(info.depthState.compareOp);

        pipelineInfo.target_info.has_depth_stencil_target = true;
        pipelineInfo.target_info.depth_stencil_format = static_cast<SDL_GPUTextureFormat>(info.depthFormat);
    }

    // Vertex input (single buffer, slot 0)
    SDL_GPUVertexBufferDescription bufferDesc = {};
    std::vector<SDL_GPUVertexAttribute> sdlAttributes;

    if (!info.vertexLayout.attributes.empty()) {
        if (info.vertexLayout.stride == 0) {
            SDL_Log("GraphicsPipeline::Create: vertex layout has attributes but stride is 0");
            return std::nullopt;
        }

        bufferDesc.slot = 0;
        bufferDesc.pitch = info.vertexLayout.stride;
        bufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        bufferDesc.instance_step_rate = 0;

        sdlAttributes.reserve(info.vertexLayout.attributes.size());
        for (const VertexAttribute& attr : info.vertexLayout.attributes) {
            SDL_GPUVertexAttribute sdlAttr = {};
            sdlAttr.location = attr.location;
            sdlAttr.buffer_slot = 0;
            sdlAttr.format = ToSDLVertexFormat(attr.format);
            sdlAttr.offset = attr.offset;
            sdlAttributes.push_back(sdlAttr);
        }

        pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &bufferDesc;
        pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineInfo.vertex_input_state.vertex_attributes = sdlAttributes.data();
        pipelineInfo.vertex_input_state.num_vertex_attributes =
            static_cast<Uint32>(sdlAttributes.size());
    }

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