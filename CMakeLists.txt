cmake_minimum_required(VERSION 3.28)
project(LutumEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# disable SDL tests for now
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# SDL_shadercross
set(SDLSHADERCROSS_VENDORED ON  CACHE BOOL "" FORCE) # build & bundle SPIRV-Cross statically
set(SDLSHADERCROSS_CLI      OFF CACHE BOOL "" FORCE) # you want the runtime lib, not the exe
set(SDLSHADERCROSS_SHARED   OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_STATIC   ON  CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_INSTALL  OFF CACHE BOOL "" FORCE)

add_subdirectory(external/SDL)
add_subdirectory(external/SDL_shadercross)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL3::SDL3
        SDL3_shadercross::SDL3_shadercross
)