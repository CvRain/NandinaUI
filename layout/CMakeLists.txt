cmake_minimum_required(VERSION 4.0)
project(nandina_layout LANGUAGES CXX)

# Layout — 布局系统
# 职责：Row / Column / Stack 布局容器、LayoutNode 测量协议、Spacer / SizedBox、轻量 Flex 能力。
# 依赖：nandina::foundation, nandina::runtime
# 对应 Issue：029–036

add_library(nandina_layout STATIC)
add_library(nandina::layout ALIAS nandina_layout)

target_compile_features(nandina_layout PUBLIC cxx_std_26)

target_sources(nandina_layout
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/layout_core.cppm
            src/flex_widgets.cppm
            src/layout_container.cppm
            src/nan_layout.cppm
)

target_link_libraries(nandina_layout
    PUBLIC
        nandina::foundation
        nandina::runtime
)