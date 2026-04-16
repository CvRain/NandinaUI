cmake_minimum_required(VERSION 4.0)
project(nandina_widgets LANGUAGES CXX)

# Widgets — 组件库
# 职责：Surface / Pressable / Text / Label / Button / Panel / Card 等基础控件，
#        消费 runtime / reactive / layout / text / theme 能力，保持语义 API 一致性。
# 依赖：nandina::runtime, nandina::reactive, nandina::layout, nandina::text, nandina::theme
# 对应 Issue：050–062 (Milestone M6–M7)

add_library(nandina_widgets INTERFACE)
add_library(nandina::widgets ALIAS nandina_widgets)

target_compile_features(nandina_widgets INTERFACE cxx_std_26)

target_link_libraries(nandina_widgets
    INTERFACE
        nandina::runtime
        nandina::reactive
        nandina::layout
        nandina::text
        nandina::theme
)

# 源文件将在实现阶段添加，例如：
# target_sources(nandina_widgets
#     PUBLIC
#         FILE_SET CXX_MODULES FILES
#             src/surface.cppm
#             src/pressable.cppm
#             src/text.cppm
#             src/label.cppm
#             src/button.cppm
#             src/panel.cppm
#             src/card.cppm
# )
