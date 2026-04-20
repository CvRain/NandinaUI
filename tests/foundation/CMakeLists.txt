# tests/foundation — Foundation 模块单元测试
# 当前覆盖：nandina.log（日志服务）
# 相关 Issue：005–007（Milestone M1）

nandina_add_test(
    NAME    test_foundation_log
    SOURCES test_log.cpp
    DEPS    nandina::foundation
)

nandina_add_test(
    NAME    test_foundation_color
    SOURCES test_color.cpp
    DEPS    nandina::foundation
)
