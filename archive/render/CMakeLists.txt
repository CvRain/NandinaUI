cmake_minimum_required(VERSION 4.0)
project(nandina_render LANGUAGES CXX)

# Render — 渲染抽象层与后端适配
# 职责：RenderBackend 抽象接口、DrawCommand 中间层、渲染后端适配（如 ThorVG 软件后端）。
# 依赖：nandina::foundation, nandina::runtime
# 相关 Issue：043–045, 049

add_library(nandina_render)
add_library(nandina::render ALIAS nandina_render)

target_compile_features(nandina_render PUBLIC cxx_std_26)

target_link_libraries(nandina_render
    PUBLIC
        nandina::foundation
        nandina::thorvg
)

target_sources(nandina_render
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/nan_render.cppm
            src/scene.cppm
            src/backend.cppm
            src/thorvg_adapter.cppm
            src/thorvg_backend.cppm
            src/nan_draw_clip.cppm
    PRIVATE
        src/nan_draw_clip.cpp
)
