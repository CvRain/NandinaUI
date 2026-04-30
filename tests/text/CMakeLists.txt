# Text 模块测试
# 测试 NanFont 的加载、度量、shaping 和 ThorVG 绘制路径。

nandina_add_test(
    NAME    test_text_nan_font
    SOURCES
        test_nan_font.cpp
    DEPS
        nandina::text
)
