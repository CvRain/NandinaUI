cmake_minimum_required(VERSION 4.0)
project(nandina_widgets LANGUAGES CXX)

# Widgets — 组件库
# 职责：Surface / Pressable / Text / Label / Button / Panel / Card 等基础控件，
#        消费 runtime / reactive / layout / text / theme 能力，保持语义 API 一致性。
# 依赖：nandina::runtime, nandina::reactive, nandina::layout, nandina::text, nandina::theme
# 对应 Issue：050–062 (Milestone M6–M7)

add_library(nandina_widgets)
add_library(nandina::widgets ALIAS nandina_widgets)

target_compile_features(nandina_widgets PUBLIC cxx_std_26)

target_link_libraries(nandina_widgets
    PUBLIC
        nandina::runtime
        nandina::reactive
        nandina::layout
        nandina::text
        nandina::theme
        nandina::thorvg
)

target_sources(nandina_widgets
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/nan_widgets.cppm     # 聚合模块 nandina.widgets
            src/nan_surface.cppm     # Surface 容器组件
            src/nan_pressable.cppm   # Pressable 交互状态机
            src/nan_label.cppm       # Label 文本显示组件
            src/nan_button.cppm      # Button 按钮组件（仅接口）
    PRIVATE
        src/nan_button.cpp           # Button 组件实现（含 Pressable/Label 完整类型）
)
