/*
* File: ShaderCompiler.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/ShaderCompiler.hpp"

#include <SDL3_shadercross/SDL_shadercross.h>

#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Shader.hpp"


namespace Lutum {
ShaderCompiler::ShaderCompiler() = default;

bool ShaderCompiler::Initialize() {
    if (!SDL_ShaderCross_Init()) {
        SDL_Log("SDL_ShaderCross_Init failed: %s", SDL_GetError());
        return false;
    }
    m_initialized = true;
    return true;
}

static SDL_ShaderCross_ShaderStage ToSDLStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::VERTEX:
            return SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
        case ShaderStage::FRAGMENT:
            return SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
        case ShaderStage::COMPUTE:
            return SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
    }

    SDL_assert(false);
    return SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
}

std::optional<Shader> ShaderCompiler::LoadHLSL(GraphicsDevice &device, const char *hlslSource, ShaderStage stage) {
    // HLSL info
    SDL_ShaderCross_HLSL_Info shaderInfo = {};
    shaderInfo.source = hlslSource;
    shaderInfo.entrypoint = "main";
    shaderInfo.shader_stage = ToSDLStage(stage);
    shaderInfo.props = 0;

    // Compile to SPIR-V
    size_t spirvSize = 0;
    void* spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&shaderInfo, &spirvSize);
    if (!spirv) {
        SDL_Log("HLSL -> SPIR-V failed: %s", SDL_GetError());
        return std::nullopt;
    }

    // Metadata
    SDL_ShaderCross_GraphicsShaderMetadata* meta =
        SDL_ShaderCross_ReflectGraphicsSPIRV(static_cast<const Uint8*>(spirv), spirvSize, 0);
    if (!meta) {
        SDL_Log("SPIR-V reflection failed: %s", SDL_GetError());
        SDL_free(spirv);
        return std::nullopt;
    }

    // Create SDL_GPUShader
    SDL_ShaderCross_SPIRV_Info spirvInfo = {};
    spirvInfo.bytecode = static_cast<const Uint8*>(spirv);
    spirvInfo.bytecode_size = spirvSize;
    spirvInfo.entrypoint = "main";
    spirvInfo.shader_stage = ToSDLStage(stage);
    spirvInfo.props = 0;

    SDL_GPUShader* shader =
        SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
            device.NativeHandle(),
            &spirvInfo,
            &meta->resource_info,
            0
        );

    SDL_free(spirv);
    SDL_free(meta);

    if (!shader) {
        SDL_Log("SPIR-V -> SDL_GPUShader failed: %s", SDL_GetError());
        return std::nullopt;
    }

    return Shader{device, shader};
}

void ShaderCompiler::Shutdown() {
    if (m_initialized) {
        SDL_ShaderCross_Quit();
        m_initialized = false;
    }
}

ShaderCompiler::~ShaderCompiler() {
    Shutdown();
}
} // Lutum