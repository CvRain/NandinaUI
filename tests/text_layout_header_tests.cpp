// The canonical text protocol must work without any scene or widget include.

#include <nandina/text/line_break.hpp>
#include <nandina/text/text_layout_backend.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <type_traits>

using namespace nandina;

namespace
{
    class CanonicalBackend final: public text::ITextLayoutBackend {
    public:
        mutable std::string observed_text;
        mutable int calls = 0;

        auto layout(text::TextLayoutInput input) const -> text::TextLayoutResult override {
            observed_text = std::string(input.text);
            ++calls;
            return {.size = foundation::NanSize(37.0F, 14.0F), .font_size = input.style.font_size};
        }
    };

    class CanonicalRenderer final: public text::ITextLayoutRenderer {
    public:
        void draw(
            const text::TextLayoutResult&,
            render::DrawContext&,
            foundation::NanPoint,
            foundation::NanColor
        ) override {}
    };

    static_assert(!std::is_abstract_v<CanonicalBackend>);
    static_assert(!std::is_abstract_v<CanonicalRenderer>);
} // namespace

TEST_CASE(
    "canonical text header provides the shared ASCII fallback and caret queries",
    "[text][header]"
) {
    const text::TextPipeline pipeline;
    REQUIRE(pipeline.backend == &text::deterministic_text_layout_backend());
    REQUIRE(pipeline.renderer == nullptr);
    text::TextLayoutInput input {.text = "CAT"};
    input.style.font_size = 10.0F;
    input.style.overflow = text::TextOverflow::clip;
    const auto result = pipeline.backend->layout(input);
    REQUIRE(result.size.get_width() == Catch::Approx(16.8F));
    REQUIRE(result.size.get_height() == Catch::Approx(12.0F));
    REQUIRE(result.font_size == Catch::Approx(10.0F));
    REQUIRE(result.baseline == Catch::Approx(10.0F));
    REQUIRE_FALSE(result.overflowed);
    REQUIRE(result.lines.size() == 1);
    const auto& line = result.lines.front();
    REQUIRE(line.text_offset == 0);
    REQUIRE(line.text_length == 3);
    REQUIRE(line.visible_text == "CAT");
    REQUIRE(line.caret_stops.size() == 4);
    const auto after_c = line.caret_for_source(1, text::TextAffinity::upstream);
    REQUIRE(after_c.source_offset == 1);
    REQUIRE(after_c.x == Catch::Approx(5.6F));
    REQUIRE(after_c.affinity == text::TextAffinity::upstream);
    const auto near_a = line.caret_for_x(10.8F);
    REQUIRE(near_a.source_offset == 2);
    REQUIRE(near_a.x == Catch::Approx(11.2F));
    const auto near_end = result.caret_for_point(foundation::NanPoint(15.0F, 6.0F));
    REQUIRE(near_end.source_offset == 3);
    REQUIRE(near_end.x == Catch::Approx(16.8F));
}

TEST_CASE(
    "canonical backend and renderer override the public protocol signatures",
    "[text][header]"
) {
    CanonicalBackend backend;
    CanonicalRenderer renderer;
    const text::TextPipeline pipeline {.backend = &backend, .renderer = &renderer};
    text::TextLayoutInput input {.text = "custom"};
    input.style.font_size = 18.0F;
    const auto result = pipeline.backend->layout(input);
    REQUIRE(backend.calls == 1);
    REQUIRE(backend.observed_text == "custom");
    REQUIRE(result.size.get_width() == Catch::Approx(37.0F));
    REQUIRE(result.size.get_height() == Catch::Approx(14.0F));
    REQUIRE(result.font_size == Catch::Approx(18.0F));
    REQUIRE(pipeline.renderer == &renderer);
}

TEST_CASE("deterministic wrap advances in bounded time over a long run", "[text][header][budget]") {
    // 复杂度守卫：fallback 后端的换行循环曾经对每个 grapheme 都重新分段整段剩余文本
    // （O(n²)，且每次分配一个 vector）。5000 字的页面构建因此要数秒 —— 见
    // docs/references/text_shaping_cost.md。O(n) 实现只需毫秒级。
    std::string source;
    source.reserve(15000);
    for (int index = 0; index < 5000; ++index) {
        source += "汉";
    }

    const auto started = std::chrono::steady_clock::now();
    const auto result = text::deterministic_text_layout_backend().layout(
        text::TextLayoutInput {
            .text = source,
            .style =
                text::TextStyle {
                    .font_size = 16.0F,
                    .overflow = text::TextOverflow::wrap,
                    .max_lines = 100000,
                },
            .constraints = foundation::NanLayoutConstraints {
                .min_width = 1173.0F,
                .max_width = 1173.0F,
            },
        }
    );
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started)
            .count();

    REQUIRE_FALSE(result.lines.empty());
    REQUIRE(elapsed < 500.0); // O(n²) 在这个规模上要数秒
}

TEST_CASE("line break opportunities follow spaces, hyphens and wide characters", "[text][wrap]") {
    using text::break_opportunity_after;
    // 空格与制表符之后
    REQUIRE(break_opportunity_after(" "));
    REQUIRE(break_opportunity_after("\t"));
    // 连字符与破折号之后
    REQUIRE(break_opportunity_after("-"));
    REQUIRE(break_opportunity_after("\xE2\x80\x93")); // en dash U+2013
    // 宽字符（CJK / 假名 / 谚文 / 全角标点）之后
    REQUIRE(break_opportunity_after("汉"));
    REQUIRE(break_opportunity_after("あ"));
    REQUIRE(break_opportunity_after("한"));
    REQUIRE(break_opportunity_after("\xEF\xBC\x8C")); // fullwidth comma U+FF0C
    // 普通拉丁字母、标点、组合序列、emoji 之后都不断
    REQUIRE_FALSE(break_opportunity_after("a"));
    REQUIRE_FALSE(break_opportunity_after("."));
    REQUIRE_FALSE(break_opportunity_after("e\xCC\x81")); // e + combining acute
    REQUIRE_FALSE(break_opportunity_after("\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB"));
    REQUIRE_FALSE(break_opportunity_after(""));
}

TEST_CASE("last break opportunity picks the latest allowed break", "[text][wrap]") {
    using text::last_break_opportunity;
    const std::string source = "hello world foo";

    // "hello world" 内最后一个机会是空格之后（下标 6）。
    REQUIRE(last_break_opportunity(source, 0, 11) == 6);
    // 整个串内最后一个机会是第二个空格之后（下标 12）。
    REQUIRE(last_break_opportunity(source, 0, source.size()) == 12);
    // "hello" 里没有机会：返回 begin，调用方据此保留原切分点。
    REQUIRE(last_break_opportunity(source, 0, 5) == 0);
    // 连字符之后可以断。
    REQUIRE(last_break_opportunity("well-known", 0, 6) == 5);
    // CJK 每个字之后都可以断：范围里的最后一个字之后就是答案。
    REQUIRE(last_break_opportunity("中文测试", 0, 6) == 6);
    // 后面接拉丁字母时，最后一个机会停在第二个汉字之后（下标 6）。
    REQUIRE(last_break_opportunity("中文abc", 0, 9) == 6);
    // 非法范围不越界。
    REQUIRE(last_break_opportunity(source, 5, 5) == 5);
    REQUIRE(last_break_opportunity(source, 0, source.size() + 100) == 12);
}

TEST_CASE("deterministic wrap breaks at word boundaries", "[text][header][wrap]") {
    const text::ITextLayoutBackend& backend = text::deterministic_text_layout_backend();
    // font_size 10 -> 每字宽 5.6；宽 60 -> 每行 10 个字符。
    const auto wrap = [&backend](std::string_view source) {
        return backend.layout(
            text::TextLayoutInput {
                .text = source,
                .style =
                    text::TextStyle {
                        .font_size = 10.0F,
                        .overflow = text::TextOverflow::wrap,
                        .max_lines = 10,
                    },
                .constraints = foundation::NanLayoutConstraints {
                    .min_width = 60.0F,
                    .max_width = 60.0F,
                },
            }
        );
    };

    // "alpha beta" 正好 10 字符，但最后一个断行机会在空格之后：回退到 "alpha "。
    const auto wrapped = wrap("alpha beta gamma");
    REQUIRE(wrapped.lines.size() == 2);
    REQUIRE(wrapped.lines[0].visible_text == "alpha ");
    REQUIRE(wrapped.lines[1].visible_text == "beta gamma");

    // 找不到机会时仍然按字符断（超长单词必须能断，否则会溢出盒子）。
    const auto unbreakable = wrap("abcdefghijklmnop");
    REQUIRE(unbreakable.lines.size() == 2);
    REQUIRE(unbreakable.lines[0].visible_text == "abcdefghij");
    REQUIRE(unbreakable.lines[1].visible_text == "klmnop");

    // CJK 没有空格，但每个字之后都可断：按字符数切分即可。
    const auto cjk = wrap("这是一段没有空格的中文文本用来测试换行行为");
    REQUIRE(cjk.lines.size() >= 3);
    REQUIRE(cjk.lines[0].visible_text.size() == 30); // 10 个汉字 = 30 字节
}
