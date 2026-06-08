cmake_minimum_required(VERSION 4.0)

project(NandinaUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- 构建选项 ---
option(NANDINA_BUILD_TESTS "Build NandinaUI unit tests" ON)

# --- 项目级编译警告（适用于所有模块目标）---
add_compile_options(-Wall -Wextra -Wpedantic)

# MinGW/PE-COFF 无法像 ELF 那样合并 inline 函数中的函数局部静态变量，
# 导致 C++ 标准库头文件中的某些符号（如 std::_Sp_make_shared_tag::__tag、
# std::__format::__replace_rep）在模块 + 静态库场景下出现多重定义。
# --allow-multiple-definition 告知链接器取第一次出现的定义（语义上完全相同）。
# -static-libgcc -static-libstdc++ 将 GCC/C++ 运行时静态链接，
# 避免部署到其他机器时缺少 libstdc++-6.dll / libgcc_s_seh-1.dll。
if (MINGW)
    add_link_options(-Wl,--allow-multiple-definition -static)
    add_compile_options(-fmodules-ts)
endif ()

# ============================================================
# 第三方依赖
# ============================================================

# SDL3 — 窗口系统与平台事件（runtime 层）
# vcpkg port: sdl3   cmake target: SDL3::SDL3
find_package(SDL3 CONFIG REQUIRED)

# ThorVG — 矢量图形渲染后端（render 层）
# vcpkg port: thorvg  构建方式: Meson → 默认仅生成 pkg-config
# 在 vcpkg 预装产物存在时优先直接链接库文件；否则回退到 pkg-config
find_package(Threads REQUIRED)

add_library(nandina_thorvg INTERFACE)
add_library(nandina::thorvg ALIAS nandina_thorvg)

set(NANDINA_VCPKG_TRIPLET "${VCPKG_TARGET_TRIPLET}")
if (NOT NANDINA_VCPKG_TRIPLET)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
        set(NANDINA_VCPKG_TRIPLET "x64-linux")
    elseif (WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|amd64)$")
        if (MINGW)
            set(NANDINA_VCPKG_TRIPLET "x64-mingw-static")
        else ()
            set(NANDINA_VCPKG_TRIPLET "x64-windows")
        endif ()
    endif ()
endif ()

# ThorVG 库文件路径：MinGW triplet 使用 .a 静态库，MSVC triplet 使用 .lib
if (NANDINA_VCPKG_TRIPLET MATCHES "mingw")
    set(NANDINA_THORVG_LIB_NAME "libthorvg-1.a")
elseif (WIN32)
    set(NANDINA_THORVG_LIB_NAME "thorvg-1.lib")
else ()
    set(NANDINA_THORVG_LIB_NAME "libthorvg-1.a")
endif ()

set(NANDINA_THORVG_RELEASE_LIB
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/${NANDINA_VCPKG_TRIPLET}/lib/${NANDINA_THORVG_LIB_NAME}")
set(NANDINA_THORVG_DEBUG_LIB
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/${NANDINA_VCPKG_TRIPLET}/debug/lib/${NANDINA_THORVG_LIB_NAME}")

set(NANDINA_THORVG_INCLUDE_DIR
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/${NANDINA_VCPKG_TRIPLET}/include")

if (EXISTS "${NANDINA_THORVG_RELEASE_LIB}" AND EXISTS "${NANDINA_THORVG_INCLUDE_DIR}")
    target_include_directories(nandina_thorvg
            SYSTEM
            INTERFACE
            "${NANDINA_THORVG_INCLUDE_DIR}"
    )

    # ThorVG C++ header (thorvg.h) uses `#if 1` which always enables
    # __declspec(dllimport) on Windows.  Define TVG_BUILD so that the
    # declspec becomes dllexport (no __imp_ prefix), allowing static
    # linking to resolve symbols directly from the .a / .lib.
    target_compile_definitions(nandina_thorvg INTERFACE TVG_BUILD=1)

    target_link_libraries(nandina_thorvg
            INTERFACE
            optimized "${NANDINA_THORVG_RELEASE_LIB}"
    )
    if (EXISTS "${NANDINA_THORVG_DEBUG_LIB}")
        target_link_libraries(nandina_thorvg
                INTERFACE
                debug "${NANDINA_THORVG_DEBUG_LIB}"
        )
    endif ()

    # ThorVG static library needs pthread on both Linux and MinGW
    target_link_libraries(nandina_thorvg
            INTERFACE
            Threads::Threads
    )
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(THORVG REQUIRED IMPORTED_TARGET thorvg-1)
    target_link_libraries(nandina_thorvg
            INTERFACE
            PkgConfig::THORVG
    )
endif ()

# Yoga — 弹性布局引擎（layout 层）
# vcpkg port: yoga   cmake target: yoga::yogacore
find_package(yoga CONFIG REQUIRED)

# FreeType — 字体光栅化（text 层）
# vcpkg port: freetype  cmake target: Freetype::Freetype
find_package(Freetype REQUIRED)

# HarfBuzz — 文本 shaping（text 层）
# vcpkg port: harfbuzz  cmake target: harfbuzz::harfbuzz
find_package(harfbuzz CONFIG REQUIRED)

# Spdlog - 日志库
# vcpkg port: spdlog cmake target: spdlog::spdlog
find_package(spdlog CONFIG REQUIRED)

# GoogleTest — 单元测试框架（tests）
# vcpkg port: gtest   cmake targets: GTest::gtest  GTest::gtest_main
if (NANDINA_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
endif ()

# ============================================================
# 模块层（按依赖顺序：底层 → 高层）
# ============================================================
add_subdirectory(foundation)   # 基础几何类型与通用原语，无上层依赖
add_subdirectory(runtime)      # UI 运行时：生命周期、节点树、事件循环     [SDL3]
add_subdirectory(reactive)     # 响应式核心：State / Effect / Prop / StateList
add_subdirectory(render)       # 渲染抽象层与后端适配接口                  [ThorVG]
add_subdirectory(layout)       # 布局系统：Row / Column / Stack / Flex     [轻量 BasicLayoutBackend, Yoga 可选]
add_subdirectory(text)         # 文本能力：shaping、测量、绘制             [FreeType + HarfBuzz]
add_subdirectory(theme)        # 主题与 token：设计系统 schema 与 ThemeManager
add_subdirectory(widgets)      # 组件库：消费上述所有模块能力
add_subdirectory(app)          # 应用开发层：Page / Router / WindowShell
add_subdirectory(bindings)     # 多语言绑定与脚本宿主接口

# ============================================================
# 测试
# ============================================================
if (NANDINA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# ============================================================
# Showcase — 验证层，不作为框架核心依赖
# ============================================================
add_subdirectory(showcase)