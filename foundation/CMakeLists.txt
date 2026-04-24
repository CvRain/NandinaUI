cmake_minimum_required(VERSION 4.0)
project(nandina_foundation LANGUAGES CXX)

# Foundation — 基础几何类型、通用原语与日志基础设施
# 职责：提供 Point / Size / Rect / Insets / Constraints 等基础数据类型、
#        通用枚举，以及贯穿全框架的 nandina::log 日志服务。
#        不依赖任何上层 nandina 模块，是整个框架的类型与工具基础。
# 第三方依赖：spdlog（日志后端，由根 CMakeLists.txt 查找）
# 相关 Issue：005–007（Milestone M1）

add_library(nandina_foundation STATIC)
add_library(nandina::foundation ALIAS nandina_foundation)

set(PROJECT_MODULE_FILES
        src/log.cppm
        src/nan_types.cppm
        src/nan_color_schema.cppm
        src/nan_color_converter.cppm
        src/nan_color.cppm
        src/nan_point.cppm
        src/nan_size.cppm
        src/nan_rect.cppm
        src/nan_insets.cppm
)

target_compile_features(nandina_foundation PUBLIC cxx_std_26)

target_sources(nandina_foundation
        PUBLIC
        FILE_SET CXX_MODULES FILES ${PROJECT_MODULE_FILES}
        PRIVATE
        src/log.cpp             # nandina.log 模块实现单元（spdlog 隔离于此）
        # 几何类型将在实现阶段逐步添加，例如：
        #     src/enums.cppm          # 通用枚举（Axis / Alignment / ...）（Issue 005）
        #     src/point.cppm          # Point<T>（Issue 006）
        #     src/size.cppm           # Size<T>（Issue 006）
        #     src/rect.cppm           # Rect<T>（Issue 006）
        #     src/insets.cppm         # Insets<T>（Issue 006）
        #     src/constraints.cppm    # Constraints 与测量语义（Issue 007）
)

target_link_libraries(nandina_foundation
        PUBLIC
        spdlog::spdlog
)
