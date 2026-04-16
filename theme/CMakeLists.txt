cmake_minimum_required(VERSION 4.0)
project(nandina_theme LANGUAGES CXX)

# Theme — 主题与设计系统
# 职责：token/theme schema、语义色映射、排版、spacing/radius 等 primitive token、ThemeManager。
# 依赖：nandina::foundation
# 相关 Issue：037–042（Milestone M4）

add_library(nandina_theme INTERFACE)
add_library(nandina::theme ALIAS nandina_theme)

target_compile_features(nandina_theme INTERFACE cxx_std_26)

target_link_libraries(nandina_theme
    INTERFACE
        nandina::foundation
)

# 源文件将在实现阶段添加（Issue 037–042）
