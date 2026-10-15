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

# IRenderDevice 的接口是 header-only 的，所以探测只依赖 Nandina 的**头文件**，不链接
# libnandina：这样「设备能不能实现这个契约」与「Nandina 的完整运行时」是两件独立的事。
set(NANDINA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
    CACHE PATH "Path to the NandinaUI source root (headers only)")
if(NOT EXISTS "${NANDINA_SOURCE_DIR}/nandina/render/render_device.hpp")
    message(FATAL_ERROR
        "NandinaUI headers not found under ${NANDINA_SOURCE_DIR}; pass "
        "-DNANDINA_SOURCE_DIR=<path-to-NandinaUI>.")
endif()

# 接口本身是 header-only，但它用到的 foundation **值类型**（NanRect/NanPoint/NanSize/NanColor）
# 有 out-of-line 实现，所以探测必须编进这几个 TU（或改去链接 libnandina）。这里选择前者：
# 依赖面小、且不需要把 raylib 拖进来。这条事实本身是 P4「host 行为契约」的输入 ——
# 设备实现无法是一个纯头文件的插件。
set(NANDINA_FOUNDATION_SOURCES
    "${NANDINA_SOURCE_DIR}/nandina/foundation/geometry.cpp"
    "${NANDINA_SOURCE_DIR}/nandina/foundation/nandina_color.cpp"
    "${NANDINA_SOURCE_DIR}/nandina/foundation/nandina_color_space.cpp"
)
foreach(source IN LISTS NANDINA_FOUNDATION_SOURCES)
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR "Missing NandinaUI source: ${source}")
    endif()
endforeach()

add_executable(nandina_sdl_gpu_probe
    main.cpp
    sdl_render_device.cpp
    ${NANDINA_FOUNDATION_SOURCES}
)
target_compile_features(nandina_sdl_gpu_probe PRIVATE cxx_std_20)
target_include_directories(nandina_sdl_gpu_probe PRIVATE "${NANDINA_SOURCE_DIR}")
target_link_libraries(nandina_sdl_gpu_probe PRIVATE PkgConfig::SDL3)
add_dependencies(nandina_sdl_gpu_probe probe_shaders)
add_custom_command(TARGET nandina_sdl_gpu_probe POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SHADER_BINARY}" "${SHADER_RUNTIME}"
    VERBATIM
)
install(TARGETS nandina_sdl_gpu_probe RUNTIME DESTINATION bin)
install(FILES "${SHADER_BINARY}" DESTINATION bin/assets)
