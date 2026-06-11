cmake_minimum_required(VERSION 4.0)
project(nandina_text LANGUAGES CXX)

# Text — 文本能力
# 职责：字体加载、文本 shaping、测量与绘制接口。
# 依赖：nandina::foundation, nandina::render
# 相关 Issue：046–048（Milestone M5）

add_library(nandina_text STATIC)
add_library(nandina::text ALIAS nandina_text)

target_compile_features(nandina_text PUBLIC cxx_std_26)

# 公共接口：模块文件（对外可见）
target_sources(nandina_text
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/nan_font.cppm          # NanFont 字体加载与度量接口
            src/nan_text.cppm          # 聚合模块 nandina.text
    PRIVATE
        src/nan_font.cpp              # NanFont 实现（隔离 FreeType + HarfBuzz）
)

target_link_libraries(nandina_text
    PUBLIC
        nandina::foundation
        nandina::thorvg
        Freetype::Freetype
        harfbuzz::harfbuzz
)

