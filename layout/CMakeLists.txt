cmake_minimum_required(VERSION 4.0)
project(nandina_layout LANGUAGES CXX)

# Layout — 布局系统
# 职责：Row / Column / Stack 布局容器、LayoutNode 测量协议、Spacer / SizedBox、轻量 Flex 能力。
# 依赖：nandina::foundation, nandina::runtime
# 对应 Issue：029–036

add_library(nandina_layout INTERFACE)
add_library(nandina::layout ALIAS nandina_layout)

target_compile_features(nandina_layout INTERFACE cxx_std_26)

target_link_libraries(nandina_layout
    INTERFACE
        nandina::foundation
        nandina::runtime
        yoga::yogacore
)

# 源文件将在实现阶段逐步添加，例如：
# target_sources(nandina_layout
#     PUBLIC
#         FILE_SET CXX_MODULES FILES
#             src/layout_node.cppm
#             src/row.cppm
#             src/column.cppm
#             src/stack.cppm
#             src/spacer.cppm
#             src/flex.cppm
# )
