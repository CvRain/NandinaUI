// Anchors 第一阶段边界测试。
//
// 这些测试不依赖尚未实现的 anchors API，只冻结 anchors 求解器必须复用的
// 现有布局、可见性、滚动和层级契约。每条测试的故障注入点写在断言附近，
// 便于区分真正的回归网和“当前实现恰好通过”的测试。

#include <nandina/scene/control.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/scroll_view.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <memory>

using namespace nandina;

namespace
{
    class MeasureProbe final: public scene::NanControl {
    public:
        explicit MeasureProbe(const foundation::NanSize size): NanControl(size) {}

        int measure_calls = 0;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override {
            ++measure_calls;
            return NanControl::on_measure(constraints);
        }
    };

    constexpr auto unbounded = std::numeric_limits<float>::infinity();
} // namespace

TEST_CASE(
    "anchors boundary percentage sizing uses finite parent bounds",
    "[anchors][boundary][sizing]"
) {
    scene::NanControl control(foundation::NanSize(40.0F, 20.0F));
    control.set_width(scene::percent(50.0F));

    const auto measured = control.measure_layout(
        foundation::NanLayoutConstraints {
            .max_width = 400.0F,
            .max_height = 100.0F,
        }
    );

    REQUIRE(measured.get_width() == Catch::Approx(200.0F));
    REQUIRE(measured.get_height() == Catch::Approx(20.0F));

    // 故障注入：把 PercentLength 当作 content 解析会使宽度回到 40，而不是 200。
}

TEST_CASE(
    "anchors boundary percentage sizing falls back to content on an unbounded axis",
    "[anchors][boundary][sizing]"
) {
    scene::NanControl control(foundation::NanSize(42.0F, 18.0F));
    control.set_height(scene::percent(50.0F));

    const auto first = control.measure_layout(
        foundation::NanLayoutConstraints {
            .max_width = 200.0F,
            .max_height = unbounded,
        }
    );
    REQUIRE(first.get_height() == Catch::Approx(18.0F));

    control.set_size(foundation::NanSize(42.0F, 96.0F));
    const auto second = control.measure_layout(
        foundation::NanLayoutConstraints {
            .max_width = 200.0F,
            .max_height = unbounded,
        }
    );
    REQUIRE(second.get_height() == Catch::Approx(96.0F));

    // 故障注入：若无界百分比读取上一帧尺寸，第二次结果会错误地保持 18。
}

TEST_CASE(
    "anchors boundary percentage min and max are resolved against finite bounds",
    "[anchors][boundary][sizing]"
) {
    scene::NanControl capped(foundation::NanSize(40.0F, 20.0F));
    capped.set_width(scene::percent(50.0F)).set_max_width(scene::percent(25.0F));
    const auto capped_size = capped.measure_layout(
        foundation::NanLayoutConstraints {
            .max_width = 400.0F,
            .max_height = 100.0F,
        }
    );
    REQUIRE(capped_size.get_width() == Catch::Approx(100.0F));

    scene::NanControl floored(foundation::NanSize(40.0F, 20.0F));
    floored.set_width(scene::percent(10.0F)).set_min_width(scene::percent(30.0F));
    const auto floored_size = floored.measure_layout(
        foundation::NanLayoutConstraints {
            .max_width = 400.0F,
            .max_height = 100.0F,
        }
    );
    REQUIRE(floored_size.get_width() == Catch::Approx(120.0F));

    // 故障注入：只解析 width 而跳过百分比 min/max 会分别得到 200 和 40。
}

TEST_CASE(
    "anchors boundary hidden children do not participate in linear layout",
    "[anchors][boundary][visibility]"
) {
    auto column_hidden = std::make_shared<MeasureProbe>(foundation::NanSize(20.0F, 30.0F));
    column_hidden->set_visible(false);
    auto column_visible = std::make_shared<MeasureProbe>(foundation::NanSize(20.0F, 10.0F));
    auto column = widget::Column::create();
    column->set_gap(7.0F).add(column_hidden).add(column_visible);

    (void)column->measure_layout(
        foundation::NanLayoutConstraints::tight(foundation::NanSize(100.0F, 100.0F))
    );
    column->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 100.0F));

    REQUIRE(column_hidden->measure_calls == 0);
    REQUIRE(column_visible->measure_calls > 0);
    REQUIRE(column_visible->position().get_y() == Catch::Approx(0.0F));

    auto row_hidden = std::make_shared<MeasureProbe>(foundation::NanSize(30.0F, 20.0F));
    row_hidden->set_visible(false);
    auto row_visible = std::make_shared<MeasureProbe>(foundation::NanSize(10.0F, 20.0F));
    auto row = widget::Row::create();
    row->set_gap(5.0F).add(row_hidden).add(row_visible);
    (void)row->measure_layout(
        foundation::NanLayoutConstraints::tight(foundation::NanSize(100.0F, 40.0F))
    );
    row->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 40.0F));

    REQUIRE(row_hidden->measure_calls == 0);
    REQUIRE(row_visible->measure_calls > 0);
    REQUIRE(row_visible->position().get_x() == Catch::Approx(0.0F));

    // 故障注入：移除 visible() 过滤会让隐藏项被测量，并把可见项推到 gap 之后。
}

TEST_CASE(
    "anchors boundary scroll views constrain only their non-scrolling axis",
    "[anchors][boundary][scroll]"
) {
    const auto finite = 200.0F;
    const auto viewport = foundation::NanLayoutConstraints {
        .max_width = finite,
        .max_height = 100.0F,
    };

    auto vertical_child = std::make_shared<scene::NanControl>(foundation::NanSize(80.0F, 300.0F));
    auto vertical = widget::ScrollView::create(widget::ScrollAxis::vertical);
    vertical->set_child(vertical_child);
    (void)vertical->measure_layout(viewport);
    REQUIRE(vertical_child->last_layout_constraints().max_width == Catch::Approx(finite));
    REQUIRE(std::isinf(vertical_child->last_layout_constraints().max_height));

    auto horizontal_child = std::make_shared<scene::NanControl>(foundation::NanSize(300.0F, 80.0F));
    auto horizontal = widget::ScrollView::create(widget::ScrollAxis::horizontal);
    horizontal->set_child(horizontal_child);
    (void)horizontal->measure_layout(viewport);
    REQUIRE(std::isinf(horizontal_child->last_layout_constraints().max_width));
    REQUIRE(horizontal_child->last_layout_constraints().max_height == Catch::Approx(100.0F));

    auto both_child = std::make_shared<scene::NanControl>(foundation::NanSize(300.0F, 300.0F));
    auto both = widget::ScrollView::create(widget::ScrollAxis::both);
    both->set_child(both_child);
    (void)both->measure_layout(viewport);
    REQUIRE(std::isinf(both_child->last_layout_constraints().max_width));
    REQUIRE(std::isinf(both_child->last_layout_constraints().max_height));

    // 故障注入：给滚动轴也施加有限约束会让百分比/内容尺寸错误地依赖视口。
}

TEST_CASE(
    "anchors boundary scrolling moves content without changing its layout size",
    "[anchors][boundary][scroll]"
) {
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(500.0F, 300.0F));
    auto view = widget::ScrollView::create(widget::ScrollAxis::vertical);
    view->set_child(content);

    (void)view->measure_layout(
        foundation::NanLayoutConstraints {
            .max_width = 200.0F,
            .max_height = 100.0F,
        }
    );
    view->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 200.0F, 100.0F));
    REQUIRE(content->width() == Catch::Approx(200.0F));
    REQUIRE(content->height() == Catch::Approx(300.0F));

    view->set_scroll_offset(foundation::NanPoint(0.0F, 40.0F));
    REQUIRE(content->position().get_y() == Catch::Approx(-40.0F));
    REQUIRE(content->height() == Catch::Approx(300.0F));

    // 故障注入：把滚动实现成重布局或改尺寸会破坏内容高度与滚动位置的独立性。
}

TEST_CASE(
    "anchors boundary reparent invalidates both layout containers",
    "[anchors][boundary][reparent]"
) {
    auto source = widget::Column::create();
    auto target = widget::Column::create();
    auto child = std::make_shared<scene::NanControl>(foundation::NanSize(20.0F, 20.0F));
    source->add(child);

    source->clear_layout_dirty();
    target->clear_layout_dirty();
    target->reparent(child);

    REQUIRE(child->parent() == target.get());
    REQUIRE(source->child_count() == 0);
    REQUIRE(target->child_count() == 1);
    REQUIRE(source->layout_dirty());
    REQUIRE(target->layout_dirty());

    // 故障注入：跳过 detach 或 attach 的 mark_layout_dirty 会让一侧容器继续使用旧解。
}
