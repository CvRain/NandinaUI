# tests/theme — Theme 模块单元测试
# 相关 Issue：037–042（Milestone M4）

nandina_add_test(
    NAME    test_theme_types
    SOURCES test_theme_types.cpp
    DEPS    nandina::theme
)

nandina_add_test(
    NAME    test_theme_manager
    SOURCES test_theme_manager.cpp
    DEPS    nandina::theme
)