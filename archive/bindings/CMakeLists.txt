cmake_minimum_required(VERSION 4.0)
project(nandina_bindings LANGUAGES CXX)

# Bindings — 多语言绑定与脚本宿主接口
# 职责：向脚本层（TypeScript / Lua 等）暴露框架能力的 C++ 接口层。
# 依赖：nandina::app
# 源文件将在实现阶段添加（M5 脚本层扩展方向）

add_library(nandina_bindings INTERFACE)
add_library(nandina::bindings ALIAS nandina_bindings)

target_compile_features(nandina_bindings INTERFACE cxx_std_26)

target_link_libraries(nandina_bindings
    INTERFACE
        nandina::app
)
