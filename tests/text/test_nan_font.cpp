//
// Created by cvrain on 2026/5/6.
//
// test_nan_font.cpp — NanFont 加载与度量验证
//

#include <gtest/gtest.h>

import nandina.text.nan_font;

// ═══════════════════════════════════════════════════════════════════════════
// NanFont 工厂方法测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, LoadSystemDefault) {
    // 系统默认字体应该能找到并加载
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // 度量应该有合理的正值
    EXPECT_GT(font->ascent(), 0.0f);
    EXPECT_GE(font->descent(), 0.0f);
    EXPECT_GT(font->line_height(), 0.0f);
    EXPECT_GE(font->line_gap(), 0.0f);  // line_gap 可以为 0

    // 字号应匹配
    EXPECT_FLOAT_EQ(font->size_pt(), 14.0f);
    EXPECT_GT(font->em_size(), 0.0f);
}

TEST(NanFontTest, FindSystemFontPath) {
    const auto path = nandina::text::NanFont::find_system_font_path();
    EXPECT_FALSE(path.empty());
    // 路径应该指向一个存在的 .ttf 或 .otf 文件
    EXPECT_TRUE(path.ends_with(".ttf") || path.ends_with(".otf")
                || path.ends_with(".ttc"));
}

TEST(NanFontTest, LoadWithDifferentSizes) {
    auto font_small = nandina::text::NanFont::load_system_default(10.0f);
    auto font_large = nandina::text::NanFont::load_system_default(24.0f);

    ASSERT_NE(font_small, nullptr);
    ASSERT_NE(font_large, nullptr);

    // 大字号应该产生更大的度量
    EXPECT_GT(font_large->line_height(), font_small->line_height());
    EXPECT_GT(font_large->em_size(), font_small->em_size());

    // 字号应匹配
    EXPECT_FLOAT_EQ(font_small->size_pt(), 10.0f);
    EXPECT_FLOAT_EQ(font_large->size_pt(), 24.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// 字体度量一致性测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, MetricConsistency) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // line_height = ascent + descent + line_gap
    const float lh = font->line_height();
    const float asc = font->ascent();
    const float desc = font->descent();
    const float gap = font->line_gap();

    EXPECT_FLOAT_EQ(lh, asc + desc + gap);

    // ascent 应该小于 line_height
    EXPECT_LT(asc, lh);
}

// ═══════════════════════════════════════════════════════════════════════════
// Glyph 度量测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, GlyphAdvance) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // 基本拉丁字符应该有非零 advance
    const float adv_a = font->glyph_advance(static_cast<std::uint32_t>('a'));
    const float adv_A = font->glyph_advance(static_cast<std::uint32_t>('A'));
    const float adv_space = font->glyph_advance(static_cast<std::uint32_t>(' '));

    EXPECT_GT(adv_a, 0.0f);
    EXPECT_GT(adv_A, 0.0f);
    EXPECT_GT(adv_space, 0.0f);

    // C0 控制字符应该返回 0（不在字体中）
    const float adv_null = font->glyph_advance(0);
    EXPECT_EQ(adv_null, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// 文本宽度估算测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, EstimateTextWidth) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // 空字符串宽度为 0
    EXPECT_EQ(font->estimate_text_width(""), 0.0f);

    // 单字符宽度应大于 0
    EXPECT_GT(font->estimate_text_width("A"), 0.0f);

    // "AA" 应该比 "A" 宽
    const float w1 = font->estimate_text_width("A");
    const float w2 = font->estimate_text_width("AA");
    EXPECT_GT(w2, w1);

    // 长文本的宽度应合理（每字符 4~20px 对于 14pt 字体而言）
    const float w = font->estimate_text_width("Hello, World!");
    EXPECT_GT(w, 40.0f);
    EXPECT_LT(w, 400.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// HarfBuzz Shaping 测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, ShapeBasicText) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // 空文本 → 空 layout
    auto empty_layout = font->shape("", 0.0f, 0);
    EXPECT_TRUE(empty_layout.empty());

    // 普通文本 → 应有结果
    auto layout = font->shape("Hello!", 0.0f, 0);
    EXPECT_FALSE(layout.empty());
    EXPECT_EQ(layout.lines.size(), 1);
    EXPECT_FALSE(layout.lines[0].glyphs.empty());
    EXPECT_GT(layout.total_width, 0.0f);
    EXPECT_GT(layout.total_height, 0.0f);

    // glyph count 应与字符数一致（基本拉丁）
    EXPECT_EQ(layout.lines[0].glyphs.size(), 6);
}

TEST(NanFontTest, ShapeWithLineBreak) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // 无换行限制 → 所有文本在一行
    auto single_line = font->shape("A B C D E", 0.0f, 0);
    EXPECT_EQ(single_line.lines.size(), 1);

    // 非常窄的宽度 → 应该产生折行
    auto multi_line = font->shape("Hello World Foo Bar", 30.0f, 0);
    EXPECT_GT(multi_line.lines.size(), 1);
    // 折行后总高度应大于单行
    EXPECT_GT(multi_line.total_height, single_line.total_height);
}

TEST(NanFontTest, ShapeWithMaxLines) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    // 限制最大行数 = 2
    auto layout = font->shape(
        "This is a long string that should definitely wrap into"
        " many lines given a sufficiently narrow width",
        60.0f, 2);
    EXPECT_LE(layout.lines.size(), 2);
}

TEST(NanFontTest, ShapeUnicodeText) {
    auto font = nandina::text::NanFont::load_system_default(18.0f);
    ASSERT_NE(font, nullptr);

    // 中文字符也应能正确 shaping
    auto layout = font->shape("你好世界", 0.0f, 0);
    EXPECT_FALSE(layout.empty());
    EXPECT_EQ(layout.lines.size(), 1);
    // 4 个中文字符 → 至少 4 个 glyph（也可能多于 4 个）
    EXPECT_GE(layout.lines[0].glyphs.size(), 4);
    EXPECT_GT(layout.total_width, 0.0f);

    // 混合中英文
    auto mixed = font->shape("你好 World", 0.0f, 0);
    EXPECT_FALSE(mixed.empty());
    EXPECT_GT(mixed.total_width, 0.0f);
}

TEST(NanFontTest, PreferredSize) {
    auto font = nandina::text::NanFont::load_system_default(14.0f);
    ASSERT_NE(font, nullptr);

    auto layout = font->shape("Hello!", 0.0f, 0);
    auto pref_size = layout.preferred_size();

    EXPECT_FLOAT_EQ(pref_size.width(), layout.total_width);
    EXPECT_FLOAT_EQ(pref_size.height(), layout.total_height);
    EXPECT_GT(pref_size.width(), 0.0f);
    EXPECT_GT(pref_size.height(), 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// 异常路径测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, InvalidFontPath) {
    EXPECT_THROW(
        nandina::text::NanFont::load_from_path("/nonexistent/font.ttf", 14.0f),
        std::runtime_error
    );
}

TEST(NanFontTest, InvalidFontSize) {
    EXPECT_THROW(
        nandina::text::NanFont::load_system_default(0.0f),
        std::runtime_error
    );
    EXPECT_THROW(
        nandina::text::NanFont::load_system_default(-5.0f),
        std::runtime_error
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// 多字体实例测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, MultipleInstances) {
    // 多个 NanFont 实例共享全局 FreeType 句柄，不应互相干扰
    auto f1 = nandina::text::NanFont::load_system_default(12.0f);
    auto f2 = nandina::text::NanFont::load_system_default(16.0f);
    auto f3 = nandina::text::NanFont::load_system_default(20.0f);

    ASSERT_NE(f1, nullptr);
    ASSERT_NE(f2, nullptr);
    ASSERT_NE(f3, nullptr);

    EXPECT_FLOAT_EQ(f1->size_pt(), 12.0f);
    EXPECT_FLOAT_EQ(f2->size_pt(), 16.0f);
    EXPECT_FLOAT_EQ(f3->size_pt(), 20.0f);

    // 字号越大行高越大
    EXPECT_LT(f1->line_height(), f2->line_height());
    EXPECT_LT(f2->line_height(), f3->line_height());
}
