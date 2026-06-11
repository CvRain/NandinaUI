//
// test_nan_font.cpp — NanFont 加载/度量/shaping 验证（更新至 P0 统一重构后 API）
//

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

import nandina.text.nan_font;
import nandina.foundation.color;

using namespace nandina::text;

// ═══════════════════════════════════════════════════════════════════════════
// 值类型 & 懒加载
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, DefaultConstructLazyLoad) {
    auto font = NanFont{}.size(14.0f);
    EXPECT_FALSE(font.is_loaded());

    auto layout = font.shape("A");
    EXPECT_FALSE(layout.empty());
    EXPECT_TRUE(font.is_loaded());
}

TEST(NanFontTest, CopySharesImpl) {
    auto font_a = NanFont{}.size(14.0f).color(nandina::NanColor::from(nandina::NanRgb{255, 0, 0}));
    font_a.shape("x");
    EXPECT_TRUE(font_a.is_loaded());

    auto font_b = font_a;
    EXPECT_TRUE(font_b.is_loaded());
    EXPECT_FLOAT_EQ(font_b.size(), 14.0f);

    font_b.size(24.0f);
    EXPECT_FLOAT_EQ(font_a.size(), 14.0f);
    EXPECT_FLOAT_EQ(font_b.size(), 24.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// 度量
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, MetricConsistency) {
    auto font = NanFont{}.size(14.0f);
    font.shape("A");

    const float lh  = font.line_height();
    const float asc = font.ascent();
    const float desc = font.descent();

    EXPECT_GT(lh, 0.0f);
    EXPECT_GT(asc, 0.0f);
    EXPECT_GE(desc, 0.0f);
    EXPECT_LT(asc, lh);
}

TEST(NanFontTest, DifferentSizes) {
    auto font_s = NanFont{}.size(10.0f);
    auto font_l = NanFont{}.size(24.0f);

    font_s.shape("A");
    font_l.shape("A");

    EXPECT_FLOAT_EQ(font_s.size(), 10.0f);
    EXPECT_FLOAT_EQ(font_l.size(), 24.0f);
    EXPECT_GT(font_l.line_height(), font_s.line_height());
}

// ═══════════════════════════════════════════════════════════════════════════
// Glyph 度量
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, GlyphAdvance) {
    auto font = NanFont{}.size(14.0f);

    EXPECT_GT(font.glyph_advance('a'), 0.0f);
    EXPECT_GT(font.glyph_advance('A'), 0.0f);
    EXPECT_GT(font.glyph_advance(' '), 0.0f);
    EXPECT_FLOAT_EQ(font.glyph_advance(0), 0.0f);
}

TEST(NanFontTest, EstimateTextWidth) {
    auto font = NanFont{}.size(14.0f);

    EXPECT_FLOAT_EQ(font.estimate_text_width(""), 0.0f);
    EXPECT_GT(font.estimate_text_width("A"), 0.0f);

    const float w1 = font.estimate_text_width("A");
    const float w2 = font.estimate_text_width("AA");
    EXPECT_GT(w2, w1);

    const float w = font.estimate_text_width("Hello, World!");
    EXPECT_GT(w, 40.0f);
    EXPECT_LT(w, 400.0f);
}

TEST(NanFontTest, EstimateTextWidthMatchesShapedWidth) {
    auto font = NanFont{}.size(14.0f);

    const auto layout = font.shape("afaad afdda");
    EXPECT_NEAR(font.estimate_text_width("afaad afdda"), layout.total_width, 0.01f);
}

// ═══════════════════════════════════════════════════════════════════════════
// Shaping
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, ShapeBasicText) {
    auto font = NanFont{}.size(14.0f);

    EXPECT_TRUE(font.shape("").empty());

    auto layout = font.shape("Hello!");
    EXPECT_FALSE(layout.empty());
    EXPECT_EQ(layout.lines.size(), 1);
    EXPECT_FALSE(layout.lines[0].glyphs.empty());
    EXPECT_GT(layout.total_width, 0.0f);
    EXPECT_GT(layout.total_height, 0.0f);
    EXPECT_EQ(layout.lines[0].glyphs.size(), 6);
}

TEST(NanFontTest, ShapeWithWrap) {
    auto font = NanFont{}.size(14.0f).overflow(TextOverflow::wrap);

    auto single = font.shape("A B C D E");
    EXPECT_EQ(single.lines.size(), 1);

    auto multi = font.shape("Hello World Foo Bar", 30.0f);
    EXPECT_GT(multi.lines.size(), 1);
    EXPECT_GT(multi.total_height, single.total_height);
}

TEST(NanFontTest, WrapPoliciesProduceBoundedLinesForMixedText) {
    auto word_font = NanFont{}.size(14.0f).overflow(TextOverflow::wrap).wrap_policy(TextWrapPolicy::word);
    auto break_font = NanFont{}.size(14.0f).overflow(TextOverflow::wrap).wrap_policy(TextWrapPolicy::break_word);

    constexpr float max_width = 36.0f;
    const auto word_layout = word_font.shape("alpha beta gamma", max_width);
    const auto break_layout = break_font.shape("alpha beta gamma", max_width);

    EXPECT_GT(word_layout.lines.size(), 1u);
    EXPECT_GT(break_layout.lines.size(), 1u);
    EXPECT_LE(word_layout.total_width, max_width + 1.0f);
    EXPECT_LE(break_layout.total_width, max_width + 1.0f);
}

TEST(NanFontTest, WrapProgressesUnderExtremelyNarrowWidth) {
    auto font = NanFont{}.size(14.0f).overflow(TextOverflow::wrap).wrap_policy(TextWrapPolicy::break_word);

    const auto layout = font.shape("abc", 1.0f);

    EXPECT_FALSE(layout.empty());
    EXPECT_EQ(layout.lines.size(), 3u);
    EXPECT_GT(layout.total_width, 0.0f);
    EXPECT_GT(layout.total_height, font.line_height() * 2.0f);
}

TEST(NanFontTest, ShapeWithMaxLines) {
    auto font = NanFont{}.size(14.0f).max_lines(2);

    auto layout = font.shape(
        "This is a long string that should definitely wrap into"
        " many lines given a sufficiently narrow width",
        60.0f);
    EXPECT_LE(layout.lines.size(), 2);
}

TEST(NanFontTest, ShapeUnicodeText) {
    auto font = NanFont{}.size(18.0f);

    auto layout = font.shape("你好世界");
    EXPECT_FALSE(layout.empty());
    EXPECT_EQ(layout.lines.size(), 1);
    EXPECT_GE(layout.lines[0].glyphs.size(), 4);
    EXPECT_GT(layout.total_width, 0.0f);

    auto mixed = font.shape("你好 World");
    EXPECT_FALSE(mixed.empty());
    EXPECT_GT(mixed.total_width, 0.0f);
}

TEST(NanFontTest, ShapeWithNewline) {
    auto font = NanFont{}.size(14.0f);

    auto layout = font.shape("Line 1\nLine 2\nLine 3");
    EXPECT_EQ(layout.lines.size(), 3);
    EXPECT_GT(layout.total_height, font.line_height() * 2.0f);
}

TEST(NanFontTest, SingleLineMode) {
    auto font = NanFont{}.size(14.0f).single_line(true);

    EXPECT_EQ(font.shape("A\nB\nC").lines.size(), 1);
}

TEST(NanFontTest, EllipsisOverflow) {
    auto font = NanFont{}.size(14.0f).overflow(TextOverflow::ellipsis);

    auto layout = font.shape("Hello World", 30.0f);
    EXPECT_FALSE(layout.empty());
    EXPECT_LE(layout.total_width, 30.0f + 1.0f);
}

TEST(NanFontTest, PreferredSize) {
    auto font = NanFont{}.size(14.0f);

    auto layout    = font.shape("Hello!");
    auto pref_size = layout.preferred_size();

    EXPECT_FLOAT_EQ(pref_size.width(), layout.total_width);
    EXPECT_FLOAT_EQ(pref_size.height(), layout.total_height);
    EXPECT_GT(pref_size.width(), 0.0f);
    EXPECT_GT(pref_size.height(), 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// CJK
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, PrefersCjkCapableFont) {
    constexpr std::string_view cjk_candidates[] = {
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/TTF/LXGWWenKai-Regular.ttf",
        "/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc",
        "/usr/share/fonts/sarasa-gothic/Sarasa-Regular.ttc",
    };

    bool has_cjk = std::any_of(std::begin(cjk_candidates), std::end(cjk_candidates),
        [](std::string_view path) {
            std::error_code ec;
            return std::filesystem::exists(path, ec) && !ec;
        });

    if (!has_cjk) {
        GTEST_SKIP() << "No CJK font found.";
    }

    auto font = NanFont{}.size(18.0f);
    EXPECT_GT(font.glyph_advance(0x4F60u), 0.0f);  // U+4F60 = 你
}

// ═══════════════════════════════════════════════════════════════════════════
// 异常路径
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontTest, InvalidFontPath) {
    EXPECT_THROW(
        static_cast<void>(NanFont::load_from_path("/nonexistent/font.ttf", 14.0f)),
        std::runtime_error
    );
}
