cmake_minimum_required(VERSION 3.24)
project(nandina_sdl_web_probe LANGUAGES C CXX)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)

# SDL 源码不随仓库追踪（体积大、且需要固定具体版本）。默认期望它在 web/ 旁边，
# 用下面的命令取固定版本；也可以用 -DNANDINA_SDL_SOURCE_DIR=<path> 指向别处。
set(NANDINA_SDL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL"
    CACHE PATH "Path to the vendored SDL3 source tree used by the web probe")
if(NOT EXISTS "${NANDINA_SDL_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "SDL3 source not found at ${NANDINA_SDL_SOURCE_DIR}. Fetch the pinned version first "
        "(see probe/cross_platform/README.md), or pass -DNANDINA_SDL_SOURCE_DIR=<path>.")
endif()
add_subdirectory("${NANDINA_SDL_SOURCE_DIR}" SDL-build EXCLUDE_FROM_ALL)

add_executable(nandina_sdl_web_probe main.cpp)
target_compile_features(nandina_sdl_web_probe PRIVATE cxx_std_20)
target_compile_definitions(nandina_sdl_web_probe PRIVATE SDL_MAIN_USE_CALLBACKS)
target_link_libraries(nandina_sdl_web_probe PRIVATE SDL3::SDL3-static)
target_link_options(nandina_sdl_web_probe PRIVATE
    "-sMIN_WEBGL_VERSION=2"
    "-sMAX_WEBGL_VERSION=2"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sEXIT_RUNTIME=0"
)
set_target_properties(nandina_sdl_web_probe PROPERTIES SUFFIX ".html")
