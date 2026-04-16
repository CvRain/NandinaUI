cmake_minimum_required(VERSION 4.0)
project(nandina_foundation LANGUAGES CXX)

# Foundation — 基础几何类型与通用原语
# 职责：提供 Point / Size / Rect / Insets / Constraints 等基础数据类型与通用枚举。
#        不依赖任何上层模块，是整个框架的类型基础。
# 依赖：无
# 相关 Issue：005–007（Milestone M1）

add_library(nandina_foundation INTERFACE)
add_library(nandina::foundation ALIAS nandina_foundation)

target_compile_features(nandina_foundation INTERFACE cxx_std_26)

# 源文件将在实现阶段逐步添加，例如：
# target_sources(nandina_foundation
#     PUBLIC
#         FILE_SET CXX_MODULES FILES
#             src/point.cppm          # Point<T>（Issue 006）
#             src/size.cppm           # Size<T>（Issue 006）
#             src/rect.cppm           # Rect<T>（Issue 006）
#             src/insets.cppm         # Insets<T>（Issue 006）
#             src/constraints.cppm    # Constraints 与测量语义（Issue 007）
#             src/enums.cppm          # 通用枚举（Axis / Alignment / ...）（Issue 005）
# )
