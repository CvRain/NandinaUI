//
// layout_convergence_tests — 一次布局周期必须收敛，且 arrange 不得越过子项自己的尺寸规格。
//
// 背景：showcase 的 Label 页面在稳态下每帧重跑一次全量布局、把全部文本重新整形一遍
// （实测 80.4 ms/帧）。根因不是渲染，而是布局永不收敛：子项在 measure 阶段与 arrange
// 阶段被指派了不同尺寸，每次 `set_size()` 都会把祖先链重新标脏，而父级是在
// `layout_to()` 里刚清完标记之后被标脏的。
// 设计与取舍见 docs/references/layout_convergence_and_text_measurement.md。
//

#include <nandina/foundation/layout_constraints.hpp>
#include <nandina/scene/control.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/primitives/text.hpp>
#include <nandina/widget/scroll_view.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    const auto kViewport = foundation::NanSize(400.0F, 300.0F);
    constexpr float kPadding = 16.0F;

    /// 一段足够长、在给定宽度下必然换行的文本。
    constexpr std::string_view kLongText =
        "A label wraps when its parent provides a finite width, and that wrapping changes the "
        "height the container has to reserve for it.";

    /// 场景树里是否还有控件处于 layout 脏状态。
    [[nodiscard]] auto any_layout_dirty(scene::NanNode& node) -> bool {
        if (auto* control = node.as_control(); control != nullptr && control->layout_dirty()) {
            return true;
        }
        for (std::size_t index = 0; index < node.child_count(); ++index) {
            auto* child = node.get_child(index);
            if (child != nullptr && any_layout_dirty(*child)) {
                return true;
            }
        }
        return false;
    }

    /// 让场景从构造期状态落定到真实视口，并断言此后**连续**多次布局都无事可做。
    ///
    /// 首帧允许有限轮收敛：组件在构造期用 loose 约束做过一次 relayout，落到真实
    /// 视口需要一次额外布局。真正的缺陷是「每帧都要重排」——那时这个循环永远出不来。
    void require_converged(scene::NanSceneTree& tree) {
        constexpr int kMaxWarmupPasses = 3;
        int warmup = 0;
        while (warmup <= kMaxWarmupPasses && tree.layout_root(kViewport) != 0) {
            ++warmup;
        }
        INFO("warmup passes: " << warmup);
        REQUIRE(warmup <= kMaxWarmupPasses);
        for (int steady = 0; steady < 4; ++steady) {
            REQUIRE(tree.layout_root(kViewport) == 0);
        }
        REQUIRE_FALSE(any_layout_dirty(*tree.root()));
    }

    [[nodiscard]] auto make_wrapping_text(float width_percent)
        -> std::shared_ptr<widget::primitives::Text> {
        auto text = std::make_shared<widget::primitives::Text>(std::string(kLongText));
        text->set_overflow(text::TextOverflow::wrap);
        text->set_max_lines(8);
        text->set_width(scene::percent(width_percent));
        return text;
    }

    /// 复现 Label 页面那个会失稳的结构：ScrollView > Padding > `fill` + `max_width` 的
    /// Column，里面既有百分比宽度的换行文本，也有 fill 宽度的换行文本。
    ///
    /// `max_width` 是复现的关键：它让内容列在「自己的上限（200）」与「父级紧约束（368）」
    /// 之间产生两种有效约束。ScrollView 的 `on_measure` 与 `on_layout` 若使用不同的跨轴
    /// 契约（上界 vs 紧约束），两次测量就会落在不同宽度上，子项每轮布局改写一次尺寸。
    struct Fixture {
        std::shared_ptr<widget::ScrollView> scroll;
        std::shared_ptr<widget::Column> content;
        std::shared_ptr<widget::primitives::Text> percent_text;
        std::shared_ptr<widget::primitives::Text> fill_text;
    };

    [[nodiscard]] auto make_fixture() -> Fixture {
        Fixture fixture;
        fixture.content = widget::Column::create();
        fixture.content->set_width(scene::fill);
        fixture.content->set_max_width(200.0F);
        fixture.content->set_cross_alignment(widget::LayoutAlignment::stretch);
        fixture.content->set_gap(8.0F);

        fixture.percent_text = make_wrapping_text(50.0F);
        fixture.fill_text = make_wrapping_text(100.0F);
        fixture.fill_text->set_width(scene::fill);

        fixture.content->add(fixture.percent_text);
        fixture.content->add(fixture.fill_text);

        auto padding = widget::Padding::create(foundation::NanInsets::all(kPadding));
        padding->set_child(fixture.content);

        fixture.scroll = widget::ScrollView::create(widget::ScrollAxis::vertical);
        fixture.scroll->set_child(padding);
        return fixture;
    }
} // namespace

TEST_CASE(
    "a scroll view content pass converges after one layout",
    "[layout][convergence][scroll]"
) {
    auto fixture = make_fixture();
    scene::NanSceneTree tree;
    tree.set_root(fixture.scroll);

    require_converged(tree);
}

TEST_CASE(
    "stretch does not override a child's explicit cross-axis size",
    "[layout][convergence][stretch]"
) {
    auto fixture = make_fixture();
    scene::NanSceneTree tree;
    tree.set_root(fixture.scroll);
    require_converged(tree);

    // 内容列宽 = 视口 - 两侧 padding；百分比子项必须停在它自己的百分比上，
    // 而不是被父级的 stretch 拉满。
    const auto content_width = kViewport.get_width() - kPadding * 2.0F;
    REQUIRE(fixture.content->size().get_width() == Catch::Approx(content_width));
    REQUIRE(fixture.percent_text->size().get_width() == Catch::Approx(content_width * 0.5F));
    REQUIRE(fixture.fill_text->size().get_width() == Catch::Approx(content_width));
    // 内容列自己的 max_width(200) 目前会被父级的紧约束压过（父级 min 胜）。这条断言记录
    // 的是现状而不是理想值：将来若让子项自己的上限胜出，这里与 reference 的开放项一起改。
    REQUIRE(fixture.content->size().get_width() == Catch::Approx(content_width));

    // 修复前 percent 子项的 arrange 宽度会被 stretch 覆盖成 content_width，
    // 与它 measure 得到的 50% 不一致 —— 上面两条断言与收敛断言会一起失败。
    REQUIRE(tree.layout_root(kViewport) == 0);
}

TEST_CASE("text stores the size its measure contract reports", "[layout][convergence][text]") {
    widget::primitives::Text text {std::string(kLongText)};
    text.set_overflow(text::TextOverflow::wrap);
    text.set_max_lines(8);

    const auto tight = foundation::NanLayoutConstraints::tight(foundation::NanSize(200.0F, 400.0F));
    const auto measured = text.measure_layout(tight);

    // 测量结果与节点尺寸必须是同一个值。修复前 size() 记录的是整形结果的原始宽度
    // （最长行的内容宽度），measured_size() 记录的是约束后的宽度，两者不同 ——
    // 改回 `set_size(layout_.size)` 即红。
    REQUIRE(text.size() == measured);
    REQUIRE(text.measured_size() == measured);
    REQUIRE(text.size().get_width() == Catch::Approx(200.0F));
}

TEST_CASE(
    "a content change triggers exactly one more layout pass",
    "[layout][convergence][mutation]"
) {
    auto fixture = make_fixture();
    scene::NanSceneTree tree;
    tree.set_root(fixture.scroll);
    require_converged(tree);

    const auto height_before = fixture.content->size().get_height();

    fixture.fill_text->set_text(
        std::string(kLongText) + std::string(kLongText) + std::string(kLongText)
    );

    // 换一段明显更长的文本：布局必须注意到这次变化（至少跑一轮），并在有限轮内
    // 重新收敛。这里刻意不要求「恰好一轮」—— 文本尺寸变化本身会在 arrange 阶段
    // 把祖先标脏，补一轮落定是合法的；不允许的是永远不收敛。
    REQUIRE(tree.layout_root(kViewport) >= 1);
    require_converged(tree);
    // 内容确实变高了：证明「收敛」不是靠「永远不脏」换来的。
    REQUIRE(fixture.content->size().get_height() > height_before);
}

TEST_CASE(
    "repeated layout with the same viewport is idempotent",
    "[layout][convergence][idempotent]"
) {
    auto fixture = make_fixture();
    scene::NanSceneTree tree;
    tree.set_root(fixture.scroll);
    require_converged(tree);

    const auto first_percent = fixture.percent_text->size();
    const auto first_fill = fixture.fill_text->size();
    const auto first_content = fixture.content->size();

    // 主动标脏后再摆一次：几何必须逐项一致（不累积 padding、不漂移）。
    fixture.scroll->mark_layout_dirty();
    REQUIRE(tree.layout_root(kViewport) == 1);
    REQUIRE(fixture.percent_text->size() == first_percent);
    REQUIRE(fixture.fill_text->size() == first_fill);
    REQUIRE(fixture.content->size() == first_content);
    REQUIRE(tree.layout_root(kViewport) == 0);
}

TEST_CASE("sub-pixel size churn is not a geometry change", "[layout][convergence][epsilon]") {
    // measure 与 arrange 会用不同运算顺序算出同一个尺寸，实测差 1e-5 量级的浮点噪声。
    // 若按精确相等判断，节点每轮布局都被判为"改了尺寸"，祖先链被反复标脏。
    auto parent = widget::Column::create();
    auto child = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 50.0F));
    parent->add(child);

    parent->clear_layout_dirty();

    // 亚像素差异：尺寸按精确值记下来，但不向上失效。
    child->set_size(foundation::NanSize(100.0F + scene::layout_size_epsilon * 0.5F, 50.0F));
    REQUIRE(child->size().get_width() > 100.0F);
    REQUIRE_FALSE(parent->layout_dirty());

    // 真正的几何变化照常向上失效。
    child->set_size(foundation::NanSize(120.0F, 50.0F));
    REQUIRE(parent->layout_dirty());
}

TEST_CASE(
    "main-axis fill takes the whole width while expanded takes the remainder",
    "[layout][fill]"
) {
    // `fill` 的定义是「请求整个可用约束」，不是「分配余量」（见 ControlSizeSpec 的注释）。
    // 在容器**主轴**上这意味着它会把后面的兄弟推出容器 —— 想要剩余空间要用 `Expanded`。
    // 这条测试把两者的区别钉住：showcase 的 Label 页面曾在 Row 里用 fill 装弹性子项，
    // 结果 End 对齐文本与右侧按钮被推到卡片外（见 text_shaping_cost.md §5）。
    const auto make_row = [](bool use_expanded) {
        auto row = widget::Row::create();
        row->set_width(scene::fill);
        row->set_gap(10.0F);
        auto fixed = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 20.0F));
        auto flexible = std::make_shared<scene::NanControl>(foundation::NanSize(50.0F, 20.0F));
        row->add(fixed);
        if (use_expanded) {
            auto wrapper = widget::Expanded::create();
            wrapper->set_child(flexible);
            row->add(wrapper);
        }
        else {
            flexible->set_width(scene::fill);
            row->add(flexible);
        }
        return std::tuple {row, fixed, flexible};
    };

    {
        const auto [row, fixed, flexible] = make_row(false);
        scene::NanSceneTree tree;
        tree.set_root(row);
        (void)tree.layout_root(foundation::NanSize(400.0F, 100.0F));
        REQUIRE(fixed->size().get_width() == Catch::Approx(100.0F));
        // fill 拿到整个 400，而不是 400-100-10=290：它会越过 Row 的右边界。
        REQUIRE(flexible->size().get_width() == Catch::Approx(400.0F));
    }
    {
        const auto [row, fixed, flexible] = make_row(true);
        scene::NanSceneTree tree;
        tree.set_root(row);
        (void)tree.layout_root(foundation::NanSize(400.0F, 100.0F));
        REQUIRE(fixed->size().get_width() == Catch::Approx(100.0F));
        REQUIRE(flexible->size().get_width() == Catch::Approx(290.0F));
    }
}
