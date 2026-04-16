cmake_minimum_required(VERSION 4.0)
project(nandina_text LANGUAGES CXX)

# Text — 文本能力
# 职责：字体加载、文本 shaping、测量与绘制接口。
# 依赖：nandina::foundation, nandina::render
# 相关 Issue：046–048（Milestone M5）

add_library(nandina_text INTERFACE)
add_library(nandina::text ALIAS nandina_text)

target_compile_features(nandina_text INTERFACE cxx_std_26)

target_link_libraries(nandina_text
    INTERFACE
        nandina::foundation
        nandina::render
        Freetype::Freetype
        harfbuzz::harfbuzz
)

# 文本引擎始终启用（freetype + harfbuzz 为 vcpkg 必需依赖）。
# 实现阶段请在源文件中使用 #define NANDINA_TEXT_ENGINE 1 或通过 cmake 定义传入。
#
# 源文件将在实现阶段逐步添加，例如：
# target_sources(nandina_text
#     PUBLIC
#         FILE_SET CXX_MODULES FILES
#             src/font.cppm           # 字体加载与描述符
#             src/text_shaper.cppm    # 文本 shaping 接口
#             src/text_measurer.cppm  # 文本测量（Issue 047）
#             src/text_painter.cppm   # 文本绘制（Issue 048）
# )
