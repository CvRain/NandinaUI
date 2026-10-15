cmake_minimum_required(VERSION 3.24)
project(nandina_sdl_gpu_probe LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3>=3.4.0)
find_program(GLSLC_EXECUTABLE glslc REQUIRED)
find_program(SPIRV_VAL_EXECUTABLE spirv-val REQUIRED)

execute_process(
    COMMAND "${GLSLC_EXECUTABLE}" --version
    OUTPUT_VARIABLE GLSLC_VERSION_OUTPUT
    ERROR_VARIABLE GLSLC_VERSION_ERROR
    RESULT_VARIABLE GLSLC_VERSION_RESULT
)
if(NOT GLSLC_VERSION_RESULT EQUAL 0 OR NOT GLSLC_VERSION_OUTPUT MATCHES "2026\\.4")
    message(FATAL_ERROR
        "Expected pinned glslc 2026.4; got: ${GLSLC_VERSION_OUTPUT}${GLSLC_VERSION_ERROR}")
endif()

execute_process(
    COMMAND "${SPIRV_VAL_EXECUTABLE}" --version
    OUTPUT_VARIABLE SPIRV_VAL_VERSION_OUTPUT
    ERROR_VARIABLE SPIRV_VAL_VERSION_ERROR
    RESULT_VARIABLE SPIRV_VAL_VERSION_RESULT
)
if(NOT SPIRV_VAL_VERSION_RESULT EQUAL 0 OR NOT SPIRV_VAL_VERSION_OUTPUT MATCHES "v2026\.4")
    message(FATAL_ERROR
        "Expected pinned SPIRV-Tools v2026.4; got: "
        "${SPIRV_VAL_VERSION_OUTPUT}${SPIRV_VAL_VERSION_ERROR}")
endif()

set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sdf.frag.hlsl")
set(SHADER_BINARY "${CMAKE_CURRENT_BINARY_DIR}/generated/sdf.frag.spv")
set(SHADER_VALIDATED "${CMAKE_CURRENT_BINARY_DIR}/generated/sdf.frag.spv.validated")
set(SHADER_RUNTIME "${CMAKE_CURRENT_BINARY_DIR}/assets/sdf.frag.spv")
add_custom_command(
    OUTPUT "${SHADER_BINARY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${GLSLC_EXECUTABLE}" -x hlsl -fshader-stage=frag
            --target-env=vulkan1.0 "${SHADER_SOURCE}" -o "${SHADER_BINARY}"
    DEPENDS "${SHADER_SOURCE}"
    VERBATIM
    COMMENT "Compiling ${SHADER_SOURCE} for Vulkan SPIR-V with glslc 2026.4"
)
# 校验必须**在构建图里**：README 曾经声称跑过 spirv-val，但 CMake 与 build.ninja
# 里都没有这一步。现在把产物与校验都变成可追踪的构建目标，源 shader 变化会重新生成
# 并重新校验；校验失败会让构建失败，而不是等到运行时才发现。
add_custom_command(
    OUTPUT "${SHADER_VALIDATED}"
    COMMAND "${SPIRV_VAL_EXECUTABLE}" --target-env vulkan1.0 "${SHADER_BINARY}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${SHADER_VALIDATED}"
    DEPENDS "${SHADER_BINARY}"
    VERBATIM
    COMMENT "Validating ${SHADER_BINARY} with spirv-val (SPIRV-Tools v2026.4)"
)
add_custom_target(probe_shaders DEPENDS "${SHADER_VALIDATED}")

add_executable(nandina_sdl_gpu_probe main.cpp)
target_compile_features(nandina_sdl_gpu_probe PRIVATE cxx_std_20)
target_link_libraries(nandina_sdl_gpu_probe PRIVATE PkgConfig::SDL3)
add_dependencies(nandina_sdl_gpu_probe probe_shaders)
add_custom_command(TARGET nandina_sdl_gpu_probe POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SHADER_BINARY}" "${SHADER_RUNTIME}"
    VERBATIM
)
install(TARGETS nandina_sdl_gpu_probe RUNTIME DESTINATION bin)
install(FILES "${SHADER_BINARY}" DESTINATION bin/assets)
