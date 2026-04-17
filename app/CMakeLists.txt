cmake_minimum_required(VERSION 4.0)
project(nandina_app LANGUAGES CXX)

# App — 应用开发层（authoring layer）
# 职责：Page / Router / PageHost / WindowShell / AppShell 等应用层抽象，
#       承载页面组织、路由与窗口控制协同。
# 依赖（当前）：SDL3::SDL3, PkgConfig::THORVG（nan_application.ixx 直接使用）
# 依赖（目标）：nandina::widgets, nandina::runtime（待各模块实现后逐步接入）

add_library(nandina_app STATIC)
add_library(nandina::app ALIAS nandina_app)

# PUBLIC：将 C++26 要求传播给所有消费此库的目标（如 showcase）
target_compile_features(nandina_app PUBLIC cxx_std_26)

# PUBLIC FILE_SET：模块接口对外可见，消费者才能 import nandina.app.application
target_sources(nandina_app
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/nan_application.ixx
)

target_link_libraries(nandina_app
    PUBLIC
        nandina::foundation
        SDL3::SDL3
        PkgConfig::THORVG
)
