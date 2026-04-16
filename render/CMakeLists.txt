cmake_minimum_required(VERSION 4.0)
project(nandina_render LANGUAGES CXX)

# Render — 渲染抽象层与后端适配
# 职责：RenderBackend 抽象接口、DrawCommand 中间层、渲染后端适配（如 ThorVG 软件后端）。
# 依赖：nandina::foundation, nandina::runtime
# 相关 Issue：043–045, 049

add_library(nandina_render INTERFACE)
add_library(nandina::render ALIAS nandina_render)

target_compile_features(nandina_render INTERFACE cxx_std_26)

target_link_libraries(nandina_render
    INTERFACE
        nandina::foundation
        nandina::runtime
        PkgConfig::THORVG
)

# 源文件将在实现阶段添加，预期结构：
#   render/
#     backend/          — RenderBackend 抽象接口
#     commands/         — DrawCommand 类型定义
#     adapters/thorvg/  — ThorVG 软件后端适配器（Issue 045）
