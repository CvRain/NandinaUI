cmake_minimum_required(VERSION 4.0)
project(nandina_theme LANGUAGES CXX)

# Theme — 主题与设计系统
# 职责：token/theme schema、语义色映射、排版、spacing/radius 等 primitive token、ThemeManager。
# 依赖：nandina::foundation
# 相关 Issue：037–042（Milestone M4）

add_library(nandina_theme)
add_library(nandina::theme ALIAS nandina_theme)

target_compile_features(nandina_theme PUBLIC cxx_std_26)

target_link_libraries(nandina_theme
    PUBLIC
        nandina::foundation
)

target_sources(nandina_theme
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/nan_theme_root.cppm        # 聚合模块 nandina.theme
            src/nan_theme_types.cppm        # NanColorRole / NanColorScheme / NanPrimitiveToken 枚举与类型
            src/nan_primitive_tokens.cppm   # NanPrimitiveTokens 结构体
            src/nan_color_palette.cppm      # NanRoleColorPair / NanColorPalette
            src/nan_theme.cppm              # NanTheme / ThemeManager
)
