cmake_minimum_required(VERSION 4.0)
project(nandina_runtime LANGUAGES CXX)

# Runtime — UI 运行时基础
# 职责：生命周期管理、节点树、事件循环、调度边界。
# 依赖：nandina::foundation
# 对应 Issue：008–017（M1）

add_library(nandina_runtime INTERFACE)
add_library(nandina::runtime ALIAS nandina_runtime)

target_compile_features(nandina_runtime INTERFACE cxx_std_26)

target_link_libraries(nandina_runtime
    INTERFACE
        nandina::foundation
        SDL3::SDL3
)

# 源文件将在实现阶段添加，例如：
# target_sources(nandina_runtime
#     PUBLIC
#         FILE_SET CXX_MODULES FILES
#             src/application.cppm
#             src/window.cppm
#             src/event_loop.cppm
#             src/event.cppm
#             src/widget.cppm
# )
