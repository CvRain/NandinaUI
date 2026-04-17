cmake_minimum_required(VERSION 4.0)
project(nandina_tests LANGUAGES CXX)

include(CTest)
include(GoogleTest)

# ---------------------------------------------------------------------------
# GoogleTest 由 vcpkg 提供，已在根 CMakeLists.txt 中通过
#   find_package(GTest CONFIG REQUIRED)
# 完成查找。此处直接使用 GTest::gtest / GTest::gtest_main 目标。
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 辅助函数：为一个模块注册测试可执行文件
#
# 用法：
#   nandina_add_test(
#       NAME    test_reactive_state
#       SOURCES reactive/test_state.cpp
#       DEPS    nandina::reactive
#   )
# ---------------------------------------------------------------------------
function(nandina_add_test)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;DEPS" ${ARGN})

    add_executable(${ARG_NAME} ${ARG_SOURCES})

    target_link_libraries(${ARG_NAME}
        PRIVATE
            GTest::gtest
            GTest::gtest_main
            ${ARG_DEPS}
    )

    target_compile_features(${ARG_NAME} PRIVATE cxx_std_26)

    gtest_discover_tests(${ARG_NAME})
endfunction()

# ---------------------------------------------------------------------------
# 各模块测试子目录
# 随各模块实现进度逐步解注释并落地。
# ---------------------------------------------------------------------------

add_subdirectory(foundation)     # nandina.log 日志服务 + Issue 005–007 — Point / Size / Rect / Constraints
# add_subdirectory(reactive)     # Issue 028    — State / Effect / Prop / batch
# add_subdirectory(runtime)      # Issue 076    — Application / Window / Widget / Event
# add_subdirectory(layout)       # Issue 036    — Row / Column / Stack / Flex
# add_subdirectory(theme)        # Issue 077    — token / ThemeManager
# add_subdirectory(widgets)      # Issue 078    — Label / Button / Panel
