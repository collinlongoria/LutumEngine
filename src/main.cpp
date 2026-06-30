#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

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

static SDL_GPUShader* LoadHLSL(
    SDL_GPUDevice* device,
    const char* hlslSource,
    SDL_ShaderCross_ShaderStage stage)
{
    SDL_ShaderCross_HLSL_Info hlslInfo = {};
    hlslInfo.source = hlslSource;
    hlslInfo.entrypoint = "main";
    hlslInfo.shader_stage = stage;
    hlslInfo.props = 0;

    size_t spirvSize = 0;
    void* spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlslInfo, &spirvSize);
    if (!spirv) {
        SDL_Log("HLSL -> SPIR-V failed: %s", SDL_GetError());
        return nullptr;
    }

    SDL_ShaderCross_GraphicsShaderMetadata* meta =
        SDL_ShaderCross_ReflectGraphicsSPIRV(
            static_cast<const Uint8*>(spirv),
            spirvSize,
            0
        );

    if (!meta) {
        SDL_Log("SPIR-V reflection failed: %s", SDL_GetError());
        SDL_free(spirv);
        return nullptr;
    }

    SDL_ShaderCross_SPIRV_Info spirvInfo = {};
    spirvInfo.bytecode = static_cast<const Uint8*>(spirv);
    spirvInfo.bytecode_size = spirvSize;
    spirvInfo.entrypoint = "main";
    spirvInfo.shader_stage = stage;
    spirvInfo.props = 0;

    SDL_GPUShader* shader =
        SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
            device,
            &spirvInfo,
            &meta->resource_info,
            0
        );

    SDL_free(meta);
    SDL_free(spirv);

    if (!shader) {
        SDL_Log("SPIR-V -> SDL_GPUShader failed: %s", SDL_GetError());
        return nullptr;
    }

    return shader;
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_ShaderCross_Init()) {
        SDL_Log("SDL_ShaderCross_Init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Lutum Engine", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_ShaderCross_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_GPUDevice* gpu = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV,
        true,
        "vulkan"
    );

    if (!gpu) {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_ShaderCross_Quit();
        SDL_Quit();
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        SDL_DestroyGPUDevice(gpu);
        SDL_DestroyWindow(window);
        SDL_ShaderCross_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Log("GPU Driver: %s", SDL_GetGPUDeviceDriver(gpu));

    SDL_GPUShader* vert = LoadHLSL(gpu, kVertexHLSL, SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    SDL_GPUShader* frag = LoadHLSL(gpu, kFragmentHLSL, SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);

    if (!vert || !frag) {
        SDL_Log("Shader compilation failed.");
        if (vert) SDL_ReleaseGPUShader(gpu, vert);
        if (frag) SDL_ReleaseGPUShader(gpu, frag);
        SDL_ReleaseWindowFromGPUDevice(gpu, window);
        SDL_DestroyGPUDevice(gpu);
        SDL_DestroyWindow(window);
        SDL_ShaderCross_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(gpu, window);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vert;
    pipelineInfo.fragment_shader = frag;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;

    SDL_GPUGraphicsPipeline* pipeline =
        SDL_CreateGPUGraphicsPipeline(gpu, &pipelineInfo);

    if (!pipeline) {
        SDL_Log("SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        SDL_ReleaseGPUShader(gpu, vert);
        SDL_ReleaseGPUShader(gpu, frag);
        SDL_ReleaseWindowFromGPUDevice(gpu, window);
        SDL_DestroyGPUDevice(gpu);
        SDL_DestroyWindow(window);
        SDL_ShaderCross_Quit();
        SDL_Quit();
        return 1;
    }

    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
        if (!cmd) {
            SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
            break;
        }

        SDL_GPUTexture* swapchainTexture = nullptr;
        Uint32 swapchainWidth = 0;
        Uint32 swapchainHeight = 0;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                cmd,
                window,
                &swapchainTexture,
                &swapchainWidth,
                &swapchainHeight))
        {
            SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
            SDL_CancelGPUCommandBuffer(cmd);
            break;
        }

        if (swapchainTexture == nullptr) {
            SDL_SubmitGPUCommandBuffer(cmd);
            continue;
        }

        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTexture;
        colorTarget.clear_color = SDL_FColor{0.08f, 0.08f, 0.10f, 1.0f};
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* pass =
            SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);

        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        // Important with SV_VertexID: first_vertex must stay 0 for portability.
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(pass);

        if (!SDL_SubmitGPUCommandBuffer(cmd)) {
            SDL_Log("SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
            break;
        }
    }

    SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
    SDL_ReleaseGPUShader(gpu, vert);
    SDL_ReleaseGPUShader(gpu, frag);

    SDL_ReleaseWindowFromGPUDevice(gpu, window);
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);

    SDL_ShaderCross_Quit();
    SDL_Quit();

    return 0;
}