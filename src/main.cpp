/*
* File: main.cpp
* Project: LutumEngine
* Author: ${AUTHOR}
* Created on: 6/30/2026
*
* Copyright (c) 2025 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

static const char* kVertexHLSL = R"(
struct VSOutput {
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
};
VSOutput main(uint vertexID : SV_VertexID) {
    float2 positions[3] = { float2(0.0,0.5), float2(0.5,-0.5), float2(-0.5,-0.5) };
    float3 colors[3]    = { float3(1,0,0), float3(0,1,0), float3(0,0,1) };
    VSOutput o;
    o.position = float4(positions[vertexID], 0.0, 1.0);
    o.color    = colors[vertexID];
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

SDL_GPUShader* LoadHLSL(SDL_GPUDevice* device,
                        const char* hlslSource,
                        SDL_ShaderCross_ShaderStage stage) {
    // 1) HLSL -> SPIR-V
    SDL_ShaderCross_HLSL_Info hlslInfo = {};
    hlslInfo.source       = hlslSource;
    hlslInfo.entrypoint   = "main";
    hlslInfo.shader_stage = stage;

    size_t spirvSize = 0;
    void* spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlslInfo, &spirvSize);
    if (!spirv) {
        SDL_Log("HLSL->SPIRV failed: %s", SDL_GetError());
        return nullptr;
    }

    // 2) reflect, then create the shader from the resource info
    SDL_ShaderCross_SPIRV_Info spirvInfo = {};
    spirvInfo.bytecode      = static_cast<Uint8*>(spirv);
    spirvInfo.bytecode_size = spirvSize;
    spirvInfo.entrypoint    = "main";
    spirvInfo.shader_stage  = stage;

    SDL_ShaderCross_GraphicsShaderMetadata* meta =
        SDL_ShaderCross_ReflectGraphicsSPIRV(static_cast<Uint8*>(spirv), spirvSize, 0);

    SDL_GPUShader* shader =
        SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &spirvInfo, &meta->resource_info, 0);

    SDL_free(meta);
    SDL_free(spirv);

    if (!shader) {
        SDL_Log("SPIRV->GPUShader failed: %s", SDL_GetError());
        return nullptr;
    }
    return shader;
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_ShaderCross_Init()) {
        SDL_Log("SDL_ShaderCross_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 GPU", 1280, 720, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GPUDevice* gpu = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL,
        true,
        nullptr);
    if (!gpu) {
        SDL_Log("SDL_CreateGPU failed: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Log("GPU Driver: %s", SDL_GetGPUDeviceDriver(gpu));

    SDL_GPUShader* vert = LoadHLSL(gpu, kVertexHLSL, SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    SDL_GPUShader* frag = LoadHLSL(gpu, kFragmentHLSL, SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);

    if (vert && frag) {
        SDL_Log("Both shaders compiled and reflected successfully.");
    } else {
        SDL_Log("Shader compilation failed — check the log above.");
    }

    SDL_ReleaseWindowFromGPUDevice(gpu, window);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
    SDL_ShaderCross_Quit();
    SDL_Quit();
    return 0;
}