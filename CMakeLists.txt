cmake_minimum_required(VERSION 3.28)

project(LutumEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LUTUM_BUILD_SHADER_TOOLS_FROM_SOURCE
        "Build SDL_shadercross shader compiler dependencies from source"
        ON
)

option(LUTUM_BUILD_TESTS
        "Build LutumEngine tests"
        ON
)

option(LUTUM_BUILD_EDITOR
        "Build the Lutum editor executable"
        ON
)

option(LUTUM_ENABLE_RENDERER
        "Compile graphics/renderer support"
        ON
)

option(LUTUM_ENABLE_DEBUG_UI
        "Compile Dear ImGui debug UI support"
        ON
)

if(LUTUM_BUILD_EDITOR AND NOT LUTUM_ENABLE_RENDERER)
    message(STATUS "LUTUM_BUILD_EDITOR=ON requires LUTUM_ENABLE_RENDERER=ON. Disabling editor for this configuration.")
    set(LUTUM_BUILD_EDITOR OFF)
endif()

if(LUTUM_ENABLE_DEBUG_UI AND NOT LUTUM_ENABLE_RENDERER)
    message(STATUS "LUTUM_ENABLE_DEBUG_UI=ON requires LUTUM_ENABLE_RENDERER=ON. Disabling debug UI for this configuration.")
    set(LUTUM_ENABLE_DEBUG_UI OFF)
endif()

# =========================
# External dependencies
# =========================

# SDL
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

add_subdirectory(external/SDL)

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

# GLM
add_subdirectory(external/glm)



# Dear ImGui Docking
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

    target_link_libraries(imgui PUBLIC
            SDL3::SDL3
    )
endif()

# =========================
# Lutum core object library
# =========================

add_library(LutumCore OBJECT
        src/Lutum/Core/Math.hpp
        src/Lutum/Core/Time.cpp
        src/Lutum/Core/Time.hpp
        src/Lutum/Core/Log.cpp
        src/Lutum/Core/Log.hpp
        src/Lutum/Core/Jobs.cpp
        src/Lutum/Core/Jobs.hpp

        src/Lutum/ECS/Entity.hpp
        src/Lutum/ECS/Component.hpp
        src/Lutum/ECS/Component.cpp
        src/Lutum/ECS/Slab.hpp
        src/Lutum/ECS/Archetype.hpp
        src/Lutum/ECS/Archetype.cpp
        src/Lutum/ECS/Registry.hpp
        src/Lutum/ECS/Registry.cpp
        src/Lutum/ECS/Query.hpp
        src/Lutum/ECS/CommandBuffer.hpp
        src/Lutum/ECS/Resource.hpp
        src/Lutum/ECS/Scheduler.hpp
        src/Lutum/ECS/Scheduler.cpp
)

target_include_directories(LutumCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(LutumCore PUBLIC
        glm::glm
)

# =========================
# Lutum platform object library
# =========================

add_library(LutumPlatform OBJECT
        src/Lutum/Platform/Window.cpp
        src/Lutum/Platform/Window.hpp
        src/Lutum/Platform/PlatformContext.cpp
        src/Lutum/Platform/PlatformContext.hpp
        src/Lutum/Platform/Input.cpp
        src/Lutum/Platform/Input.hpp
)

target_include_directories(LutumPlatform PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(LutumPlatform PUBLIC
        SDL3::SDL3
)

# =========================
# Lutum renderer object library
# =========================

if(LUTUM_ENABLE_RENDERER)
    add_library(LutumRenderer OBJECT
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
            src/Lutum/Graphics/BufferUploader.cpp
            src/Lutum/Graphics/BufferUploader.hpp
            src/Lutum/Graphics/RenderTarget.cpp
            src/Lutum/Graphics/RenderTarget.hpp
    )

    target_include_directories(LutumRenderer PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(LutumRenderer PUBLIC
            SDL3::SDL3
            SDL3_shadercross::SDL3_shadercross
            glm::glm
    )

    target_compile_definitions(LutumRenderer PUBLIC
            LUTUM_ENABLE_RENDERER=1
    )
endif()

# =========================
# Lutum debug object library
# =========================

add_library(LutumDebug OBJECT
        src/Lutum/Debug/DebugTools.hpp
        src/Lutum/Debug/DebugTools.cpp
)

target_include_directories(LutumDebug PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(LUTUM_ENABLE_DEBUG_UI)
    target_sources(LutumDebug PRIVATE
            src/Lutum/Debug/DebugUI.cpp
            src/Lutum/Debug/DebugUI.hpp
    )

    target_link_libraries(LutumDebug PUBLIC
            imgui
            SDL3::SDL3
    )

    target_compile_definitions(LutumDebug PUBLIC
            LUTUM_ENABLE_DEBUG_UI=1
    )
else()
    target_compile_definitions(LutumDebug PUBLIC
            LUTUM_ENABLE_DEBUG_UI=0
    )
endif()

# =========================
# Final engine library
# =========================

add_library(LutumEngine STATIC
        $<TARGET_OBJECTS:LutumCore>
        $<TARGET_OBJECTS:LutumPlatform>
        $<TARGET_OBJECTS:LutumDebug>
)

target_include_directories(LutumEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(LutumEngine PUBLIC
        SDL3::SDL3
        glm::glm
)

if(LUTUM_ENABLE_RENDERER)
    target_sources(LutumEngine PRIVATE
            $<TARGET_OBJECTS:LutumRenderer>
    )

    target_link_libraries(LutumEngine PUBLIC
            SDL3_shadercross::SDL3_shadercross
    )

    target_compile_definitions(LutumEngine PUBLIC
            LUTUM_ENABLE_RENDERER=1
    )
else()
    target_compile_definitions(LutumEngine PUBLIC
            LUTUM_ENABLE_RENDERER=0
    )
endif()

if(LUTUM_ENABLE_DEBUG_UI)
    target_link_libraries(LutumEngine PUBLIC
            imgui
    )
endif()

# =========================
# Editor executable
# =========================

if(LUTUM_BUILD_EDITOR)
    add_executable(LutumEditor
            src/Editor/main.cpp
            src/Editor/Application.cpp
            src/Editor/Application.hpp
            src/Lutum/Scene/Transform.hpp
            src/Lutum/Scene/Camera.hpp
            src/Lutum/Scene/CameraSystems.cpp
            src/Lutum/Scene/CameraSystems.hpp
            src/Editor/EditorUI.cpp
            src/Editor/EditorUI.hpp
    )

    target_include_directories(LutumEditor PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(LutumEditor PRIVATE
            LutumEngine
    )
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
        set(LUTUM_SHADER_RUNTIME_DIR
                "${CMAKE_SOURCE_DIR}/tools/shader/win64"
                CACHE PATH "Directory containing shader compiler runtime binaries"
        )

        if(NOT EXISTS "${LUTUM_SHADER_RUNTIME_DIR}/dxcompiler.dll")
            message(FATAL_ERROR
                    "Missing dxcompiler.dll at: ${LUTUM_SHADER_RUNTIME_DIR}/dxcompiler.dll"
            )
        endif()

        if(NOT EXISTS "${LUTUM_SHADER_RUNTIME_DIR}/dxil.dll")
            message(FATAL_ERROR
                    "Missing dxil.dll at: ${LUTUM_SHADER_RUNTIME_DIR}/dxil.dll"
            )
        endif()

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