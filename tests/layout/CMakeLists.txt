cmake_minimum_required(VERSION 4.0)
project(nandina_layout_tests LANGUAGES CXX)

# layout 测试需要链接 layout 库（其中包含 nandina::foundation 和 nandina::runtime）
nandina_add_test(
    NAME    test_layout_core
    SOURCES test_layout_core.cpp
    DEPS    nandina::layout
)