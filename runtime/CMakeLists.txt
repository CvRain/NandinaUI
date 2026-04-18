cmake_minimum_required(VERSION 4.0)
project(nandina_runtime LANGUAGES CXX)

# Runtime — UI 运行时基础
# 职责：生命周期管理、节点树、事件循环、调度边界。
# 依赖：nandina::foundation, SDL3::SDL3, PkgConfig::THORVG
# 对应 Issue：008–017（M1）

add_library(nandina_runtime STATIC)
add_library(nandina::runtime ALIAS nandina_runtime)

# PUBLIC：将 C++26 要求传播给所有消费此库的目标
target_compile_features(nandina_runtime PUBLIC cxx_std_26)

target_sources(nandina_runtime
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/nan_window.cppm         # nandina.runtime.nan_window（Issue 009/010/017）
    # 后续实现阶段逐步添加：
    #     src/application.cppm          # Application 生命周期（Issue 008）
    #     src/event_loop.cppm           # 事件循环调度（Issue 010）
    #     src/event.cppm                # 统一 Event 类型体系（Issue 011）
    #     src/widget.cppm               # Widget 树基础结构（Issue 012–015）
    PRIVATE
        src/nan_window.cpp              # SDL + ThorVG 实现单元（隔离平台类型）
)

target_link_libraries(nandina_runtime
    PUBLIC
        nandina::foundation             # nandina.log 及后续几何基础类型
        SDL3::SDL3                      # 窗口系统与平台事件
        PkgConfig::THORVG               # 矢量图形渲染后端
)
