cmake_minimum_required(VERSION 3.28)

project(LutumEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Options
# =========================

option(LUTUM_BUILD_SHADER_TOOLS_FROM_SOURCE "Build SDL_shadercross shader compiler dependencies from source" ON)
option(LUTUM_BUILD_TESTS   "Build LutumEngine tests"                 ON)
option(LUTUM_BUILD_EDITOR  "Build the Lutum editor executable"       ON)
option(LUTUM_ENABLE_RENDERER "Compile graphics/renderer support"     ON)
option(LUTUM_ENABLE_DEBUG_UI "Compile Dear ImGui debug UI support"   ON)

if(LUTUM_BUILD_EDITOR AND NOT LUTUM_ENABLE_RENDERER)
    message(STATUS "LUTUM_BUILD_EDITOR requires LUTUM_ENABLE_RENDERER. Disabling editor.")
    set(LUTUM_BUILD_EDITOR OFF)
endif()

if(LUTUM_ENABLE_DEBUG_UI AND NOT LUTUM_ENABLE_RENDERER)
    message(STATUS "LUTUM_ENABLE_DEBUG_UI requires LUTUM_ENABLE_RENDERER. Disabling debug UI.")
    set(LUTUM_ENABLE_DEBUG_UI OFF)
endif()

# Build-config definitions, linked by every module and propagated to consumers.
add_library(LutumOptions INTERFACE)
target_compile_definitions(LutumOptions INTERFACE
        LUTUM_ENABLE_RENDERER=$<BOOL:${LUTUM_ENABLE_RENDERER}>
        LUTUM_ENABLE_DEBUG_UI=$<BOOL:${LUTUM_ENABLE_DEBUG_UI}>
)

# =========================
# External dependencies
# =========================

# SDL
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL)

# GLM
add_subdirectory(external/glm)

# stb (header-only)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external/stb)

# SDL_shadercross
if(LUTUM_ENABLE_RENDERER)
    set(SDLSHADERCROSS_CLI     OFF CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_SHARED  OFF CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_STATIC  ON  CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_DXC     ON  CACHE BOOL "" FORCE)

    if(LUTUM_BUILD_SHADER_TOOLS_FROM_SOURCE)
        set(SDLSHADERCROSS_VENDORED ON CACHE BOOL "" FORCE)
        set(ENABLE_SPIRV_CODEGEN ON CACHE BOOL "" FORCE)
    else()
        set(SDLSHADERCROSS_VENDORED OFF CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(external/SDL_shadercross)
endif()

# Dear ImGui (docking)
if(LUTUM_ENABLE_DEBUG_UI)
    add_library(imgui STATIC
            external/imgui/imgui.cpp
            external/imgui/imgui_demo.cpp
            external/imgui/imgui_draw.cpp
            external/imgui/imgui_tables.cpp
            external/imgui/imgui_widgets.cpp
            external/imgui/backends/imgui_impl_sdl3.cpp
            external/imgui/backends/imgui_impl_sdlgpu3.cpp
    )
    target_include_directories(imgui PUBLIC
            external/imgui
            external/imgui/backends
    )
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
endif()

# =========================
# LutumCore: Core + ECS
# =========================

add_library(LutumCore STATIC
        src/Lutum/Core/Math.hpp
        src/Lutum/Core/Time.cpp
        src/Lutum/Core/Time.hpp
        src/Lutum/Core/Log.cpp
        src/Lutum/Core/Log.hpp
        src/Lutum/Core/Jobs.cpp
        src/Lutum/Core/Jobs.hpp

        src/Lutum/Debug/DebugTools.cpp
        src/Lutum/Debug/DebugTools.hpp

        src/Lutum/ECS/Entity.hpp
        src/Lutum/ECS/Component.cpp
        src/Lutum/ECS/Component.hpp
        src/Lutum/ECS/Slab.hpp
        src/Lutum/ECS/Archetype.cpp
        src/Lutum/ECS/Archetype.hpp
        src/Lutum/ECS/Registry.cpp
        src/Lutum/ECS/Registry.hpp
        src/Lutum/ECS/Query.hpp
        src/Lutum/ECS/CommandBuffer.hpp
        src/Lutum/ECS/Resource.hpp
        src/Lutum/ECS/Scheduler.cpp
        src/Lutum/ECS/Scheduler.hpp
        src/Lutum/Core/FileSystem.cpp
        src/Lutum/Core/FileSystem.hpp
        src/Lutum/ECS/Resource.cpp
        src/Lutum/ECS/Snapshot.cpp
        src/Lutum/ECS/Reflect.hpp
        src/Lutum/Core/Hash.hpp
        src/Lutum/Assets/AssetID.hpp
        src/Lutum/Assets/AssetHeader.hpp
        src/Lutum/Assets/AssetRegistry.hpp
        src/Lutum/Assets/AssetRegistry.cpp
        src/Lutum/Assets/PayloadIO.hpp
        src/Lutum/Assets/TextureAsset.hpp
        src/Lutum/Assets/TextureAsset.cpp
        src/Lutum/Assets/MeshAsset.hpp
        src/Lutum/Assets/MeshAsset.cpp
        src/Lutum/Assets/MaterialAsset.hpp
        src/Lutum/Assets/MaterialAsset.cpp
)
target_include_directories(LutumCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(LutumCore PUBLIC
        LutumOptions
        glm::glm
        SDL3::SDL3
)

# =========================
# LutumPlatform: window, input, platform context
# =========================

add_library(LutumPlatform STATIC
        src/Lutum/Platform/Window.cpp
        src/Lutum/Platform/Window.hpp
        src/Lutum/Platform/PlatformContext.cpp
        src/Lutum/Platform/PlatformContext.hpp
        src/Lutum/Platform/Input.cpp
        src/Lutum/Platform/Input.hpp
)
target_link_libraries(LutumPlatform PUBLIC
        LutumCore
)

# =========================
# LutumGraphics: renderer + debug UI
# =========================

if(LUTUM_ENABLE_RENDERER)
    add_library(LutumGraphics STATIC
            src/Lutum/Graphics/GraphicsDevice.cpp
            src/Lutum/Graphics/GraphicsDevice.hpp
            src/Lutum/Graphics/Renderer.cpp
            src/Lutum/Graphics/Renderer.hpp
            src/Lutum/Graphics/ShaderCompiler.cpp
            src/Lutum/Graphics/ShaderCompiler.hpp
            src/Lutum/Graphics/Shader.cpp
            src/Lutum/Graphics/Shader.hpp
            src/Lutum/Graphics/GraphicsPipeline.cpp
            src/Lutum/Graphics/GraphicsPipeline.hpp
            src/Lutum/Graphics/Buffer.cpp
            src/Lutum/Graphics/Buffer.hpp
            src/Lutum/Graphics/GpuUploader.cpp
            src/Lutum/Graphics/GpuUploader.hpp
            src/Lutum/Graphics/RenderTarget.cpp
            src/Lutum/Graphics/RenderTarget.hpp
            src/Lutum/Graphics/ImageIO.cpp
            src/Lutum/Graphics/ImageIO.hpp
            src/Lutum/Graphics/Texture.cpp
            src/Lutum/Graphics/Texture.hpp
    )

    if(LUTUM_ENABLE_DEBUG_UI)
        target_sources(LutumGraphics PRIVATE
                src/Lutum/Debug/DebugUI.cpp
                src/Lutum/Debug/DebugUI.hpp
        )
        target_link_libraries(LutumGraphics PUBLIC imgui)
    endif()

    target_link_libraries(LutumGraphics
            PUBLIC
            LutumCore
            LutumPlatform   # Window
            SDL3_shadercross::SDL3_shadercross
            PRIVATE
            stb
    )
endif()

# =========================
# LutumScene: engine-side scene layer
# =========================

add_library(LutumScene STATIC
        src/Lutum/Scene/Transform.hpp
        src/Lutum/Scene/Camera.hpp
        src/Lutum/Scene/CameraSystems.cpp
        src/Lutum/Scene/CameraSystems.hpp
        src/Lutum/Scene/Name.hpp
)
target_link_libraries(LutumScene PUBLIC
        LutumCore
        LutumPlatform
)

# =========================
# LutumEngine
# =========================

add_library(LutumEngine INTERFACE)
target_link_libraries(LutumEngine INTERFACE
        LutumCore
        LutumPlatform
        LutumScene
)
if(LUTUM_ENABLE_RENDERER)
    target_link_libraries(LutumEngine INTERFACE LutumGraphics)
endif()

# =========================
# Editor executable
# =========================

if(LUTUM_BUILD_EDITOR)
    add_executable(LutumEditor
            src/Editor/main.cpp
            src/Editor/Application.cpp
            src/Editor/Application.hpp
            src/Editor/EditorUI.cpp
            src/Editor/EditorUI.hpp
            src/Editor/EditorContext.hpp
            src/Editor/Panels/ViewportPanel.hpp
            src/Editor/Panels/ViewportPanel.cpp
            src/Editor/Panels/StatsPanel.hpp
            src/Editor/Panels/StatsPanel.cpp
            src/Editor/Panels/SystemsPanel.hpp
            src/Editor/Panels/SystemsPanel.cpp
            src/Editor/Panels/EntitiesPanel.hpp
            src/Editor/Panels/EntitiesPanel.cpp
            src/Editor/Panels/InspectorPanel.hpp
            src/Editor/Panels/InspectorPanel.cpp
            src/Editor/Panels/ArchetypesPanel.hpp
            src/Editor/Panels/ArchetypesPanel.cpp
            src/Editor/Panels/ResourcesPanel.hpp
            src/Editor/Panels/ResourcesPanel.cpp
            src/Editor/EditorSettings.hpp
            src/Editor/EditorSettings.cpp
            src/Editor/Panels/LogPanel.hpp
            src/Editor/Panels/LogPanel.cpp
            src/Editor/EngineContentGen.hpp
            src/Editor/EngineContentGen.cpp
    )
    target_link_libraries(LutumEditor PRIVATE LutumEngine stb)
endif()

# =========================
# Tests
# =========================

if(LUTUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/doctest)
    add_subdirectory(tests)
endif()

# =========================
# Runtime shader compiler binaries
# =========================

if(WIN32 AND LUTUM_BUILD_EDITOR AND LUTUM_ENABLE_RENDERER)
    if(LUTUM_BUILD_SHADER_TOOLS_FROM_SOURCE)
        add_custom_command(TARGET LutumEditor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:DirectXShaderCompiler::dxcompiler>"
                "$<TARGET_FILE_DIR:LutumEditor>/dxcompiler.dll"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:DirectXShaderCompiler::dxil>"
                "$<TARGET_FILE_DIR:LutumEditor>/dxil.dll"
        )
    else()
        set(LUTUM_SHADER_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/tools/shader/win64"
                CACHE PATH "Directory containing shader compiler runtime binaries")

        foreach(dll dxcompiler.dll dxil.dll)
            if(NOT EXISTS "${LUTUM_SHADER_RUNTIME_DIR}/${dll}")
                message(FATAL_ERROR "Missing ${dll} at: ${LUTUM_SHADER_RUNTIME_DIR}/${dll}")
            endif()
        endforeach()

        add_custom_command(TARGET LutumEditor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LUTUM_SHADER_RUNTIME_DIR}/dxcompiler.dll"
                "$<TARGET_FILE_DIR:LutumEditor>/dxcompiler.dll"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LUTUM_SHADER_RUNTIME_DIR}/dxil.dll"
                "$<TARGET_FILE_DIR:LutumEditor>/dxil.dll"
        )
    endif()
endif()