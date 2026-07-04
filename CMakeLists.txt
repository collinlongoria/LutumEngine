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

# SDL
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

add_subdirectory(external/SDL)

# SDL_shadercross
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

# GLM
add_subdirectory(external/glm)

# doctest and tests
if(LUTUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(external/doctest)
    add_subdirectory(tests)
endif()

# =========================
# Engine library
# =========================

add_library(LutumEngine STATIC
        src/Lutum/Platform/Window.cpp
        src/Lutum/Platform/Window.hpp
        src/Lutum/Platform/PlatformContext.cpp
        src/Lutum/Platform/PlatformContext.hpp
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
        src/Lutum/Core/Math.hpp
        src/Lutum/Core/Time.cpp
        src/Lutum/Core/Time.hpp
        src/Lutum/Platform/Input.cpp
        src/Lutum/Platform/Input.hpp
        src/Lutum/Core/Log.cpp
        src/Lutum/Core/Log.hpp
        src/Lutum/Core/Jobs.cpp
        src/Lutum/Core/Jobs.hpp
        src/Lutum/ECS/Entity.hpp
        src/Lutum/ECS/Component.hpp
        src/Lutum/ECS/Component.cpp
        src/Lutum/Core/Debug.hpp
        src/Lutum/Core/Debug.cpp
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
        src/Lutum/Graphics/RenderTarget.cpp
        src/Lutum/Graphics/RenderTarget.hpp
)

target_include_directories(LutumEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(LutumEngine PUBLIC
        SDL3::SDL3
        SDL3_shadercross::SDL3_shadercross
        glm::glm
)

# =========================
# Editor executable
# =========================

add_executable(LutumEditor
        src/Editor/main.cpp
        src/Editor/Application.cpp
        src/Editor/Application.hpp
        src/Lutum/Scene/Transform.hpp
        src/Lutum/Scene/Camera.hpp
        src/Lutum/Scene/CameraSystems.cpp
        src/Lutum/Scene/CameraSystems.hpp
)

target_include_directories(LutumEditor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(LutumEditor PRIVATE
        LutumEngine
)

# =========================
# Runtime shader compiler binaries
# =========================

if(WIN32)
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