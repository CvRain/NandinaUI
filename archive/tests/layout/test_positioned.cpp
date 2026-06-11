#include <gtest/gtest.h>

import nandina.layout.positioned;
import nandina.foundation.nan_constraints;
import nandina.foundation.nan_size;

using namespace nandina::layout;
using namespace nandina::geometry;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: 构建一个最小化的可测试 Positioned 容器
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// 用于测试的简单叶子 Widget，返回固定 preferred_size
class StubWidget final : public nandina::runtime::NanWidget {
public:
    explicit StubWidget(NanSize pref) : m_pref(pref) {}

    [[nodiscard]] auto preferred_size() const noexcept -> NanSize override {
        return m_pref;
    }

private:
    NanSize m_pref;
};

auto make_stub(float w, float h) -> std::unique_ptr<StubWidget> {
    return std::make_unique<StubWidget>(NanSize{w, h});
}

}  // namespace

// ─── GeomRef ─────────────────────────────────────────────────────────────────

TEST(GeomRefTest, DefaultIsInvalid) {
    GeomRef ref;
    EXPECT_FALSE(ref.is_valid());
}

// ─── AnchorExpr / resolve ────────────────────────────────────────────────────

TEST(AnchorExprTest, StaticValue) {
    AnchorExpr a = 42.0f;
    EXPECT_TRUE(anchor_has_value(a));
    EXPECT_FLOAT_EQ(anchor_resolve(a), 42.0f);
}

TEST(AnchorExprTest, LambdaValue) {
    AnchorExpr a = std::function<float()>{[] { return 7.5f; }};
    EXPECT_TRUE(anchor_has_value(a));
    EXPECT_FLOAT_EQ(anchor_resolve(a), 7.5f);
}

TEST(AnchorExprTest, EmptyHasNoValue) {
    AnchorExpr a;
    EXPECT_FALSE(anchor_has_value(a));
    EXPECT_FLOAT_EQ(anchor_resolve(a), 0.0f);  // default returns 0
}

// ─── Positioned — 无 anchor（原点 + preferred_size）─────────────────────────

TEST(PositionedTest, NoAnchors_PlacesAtOriginWithPreferredSize) {
    auto container = Positioned::Create();
    container->add_positioned_child(make_stub(80, 40), PositionedProps{});

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.x(),      0.0f);
    EXPECT_FLOAT_EQ(child.y(),      0.0f);
    EXPECT_FLOAT_EQ(child.width(),  80.0f);
    EXPECT_FLOAT_EQ(child.height(), 40.0f);
}

// ─── Positioned — left + top ─────────────────────────────────────────────────

TEST(PositionedTest, LeftTop_PlacesAtOffset) {
    auto container = Positioned::Create();
    PositionedProps props;
    props.left = 20.0f;
    props.top  = 10.0f;
    container->add_positioned_child(make_stub(80, 40), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.x(),      20.0f);
    EXPECT_FLOAT_EQ(child.y(),      10.0f);
    EXPECT_FLOAT_EQ(child.width(),  80.0f);   // preferred
    EXPECT_FLOAT_EQ(child.height(), 40.0f);   // preferred
}

// ─── Positioned — right + bottom ─────────────────────────────────────────────

TEST(PositionedTest, RightBottom_PinsToBottomRight) {
    auto container = Positioned::Create();
    PositionedProps props;
    props.right  = 16.0f;
    props.bottom = 16.0f;
    container->add_positioned_child(make_stub(80, 36), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    // x = parent.w - right - preferred.w = 400 - 16 - 80 = 304
    // y = parent.h - bottom - preferred.h = 300 - 16 - 36 = 248
    EXPECT_FLOAT_EQ(child.x(),      304.0f);
    EXPECT_FLOAT_EQ(child.y(),      248.0f);
    EXPECT_FLOAT_EQ(child.width(),  80.0f);
    EXPECT_FLOAT_EQ(child.height(), 36.0f);
}

// ─── Positioned — left + right（推导宽度）────────────────────────────────────

TEST(PositionedTest, LeftRight_DerivesWidth) {
    auto container = Positioned::Create();
    PositionedProps props;
    props.left  = 10.0f;
    props.right = 20.0f;
    container->add_positioned_child(make_stub(0, 50), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.x(),     10.0f);
    EXPECT_FLOAT_EQ(child.width(), 370.0f);   // 400 - 10 - 20
}

// ─── Positioned — top + bottom（推导高度）────────────────────────────────────

TEST(PositionedTest, TopBottom_DerivesHeight) {
    auto container = Positioned::Create();
    PositionedProps props;
    props.top    = 50.0f;
    props.bottom = 60.0f;
    container->add_positioned_child(make_stub(100, 0), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.y(),      50.0f);
    EXPECT_FLOAT_EQ(child.height(), 190.0f);   // 300 - 50 - 60
}

// ─── Positioned — fill（全四边锚点均为 0）────────────────────────────────────

TEST(PositionedTest, Fill_FillsEntireParent) {
    auto container = Positioned::Create();
    PositionedProps props;
    props.left   = 0.0f;
    props.top    = 0.0f;
    props.right  = 0.0f;
    props.bottom = 0.0f;
    container->add_positioned_child(make_stub(10, 10), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.x(),      0.0f);
    EXPECT_FLOAT_EQ(child.y(),      0.0f);
    EXPECT_FLOAT_EQ(child.width(),  400.0f);
    EXPECT_FLOAT_EQ(child.height(), 300.0f);
}

// ─── Positioned — explicit width/height only（居中）──────────────────────────

TEST(PositionedTest, WidthOnly_CentersHorizontally) {
    auto container = Positioned::Create();
    PositionedProps props;
    props.width = 100.0f;
    container->add_positioned_child(make_stub(0, 50), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.x(),     150.0f);   // (400-100)/2
    EXPECT_FLOAT_EQ(child.width(), 100.0f);
}

// ─── Positioned — GeomRef 填充 ───────────────────────────────────────────────

TEST(PositionedTest, GeomRef_FilledAfterLayout) {
    auto container = Positioned::Create();

    GeomRef ref;
    PositionedProps props;
    props.left      = 10.0f;
    props.top       = 20.0f;
    props.width     = 80.0f;
    props.height    = 40.0f;
    props.geom_ref  = &ref;
    container->add_positioned_child(make_stub(0, 0), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    ASSERT_TRUE(ref.is_valid());
    EXPECT_FLOAT_EQ(ref.left(),   10.0f);
    EXPECT_FLOAT_EQ(ref.top(),    20.0f);
    EXPECT_FLOAT_EQ(ref.width(),  80.0f);
    EXPECT_FLOAT_EQ(ref.height(), 40.0f);
    EXPECT_FLOAT_EQ(ref.right(),  90.0f);
    EXPECT_FLOAT_EQ(ref.bottom(), 60.0f);
}

// ─── Positioned — 兄弟节点 anchor lambda 引用 GeomRef ────────────────────────

TEST(PositionedTest, SiblingRef_SecondChildAnchorsBelowFirst) {
    auto container = Positioned::Create();
    GeomRef header_ref;

    // 第一个子节点：固定在顶部，高度 50
    PositionedProps header_props;
    header_props.left       = 0.0f;
    header_props.top        = 0.0f;
    header_props.right      = 0.0f;
    header_props.height     = 50.0f;
    header_props.geom_ref   = &header_ref;
    container->add_positioned_child(make_stub(0, 0), header_props);

    // 第二个子节点：紧靠第一个子节点下方（通过 lambda 引用 GeomRef）
    PositionedProps content_props;
    content_props.left   = 0.0f;
    content_props.right  = 0.0f;
    content_props.top    = std::function<float()>{[&header_ref] { return header_ref.bottom(); }};
    content_props.bottom = 0.0f;
    container->add_positioned_child(make_stub(0, 0), content_props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& header  = *container->children()[0];
    const auto& content = *container->children()[1];

    EXPECT_FLOAT_EQ(header.y(),       0.0f);
    EXPECT_FLOAT_EQ(header.height(),  50.0f);

    // content 应从 y=50 开始，高度 = 300-50 = 250
    EXPECT_FLOAT_EQ(content.y(),      50.0f);
    EXPECT_FLOAT_EQ(content.height(), 250.0f);
}

// ─── Positioned — 响应式 lambda 每次 layout 重新求值 ─────────────────────────

TEST(PositionedTest, ReactiveLambda_ReEvaluatesOnRelayout) {
    auto container = Positioned::Create();

    float dynamic_right = 16.0f;
    PositionedProps props;
    props.right  = std::function<float()>{[&dynamic_right] { return dynamic_right; }};
    props.bottom = std::function<float()>{[&dynamic_right] { return dynamic_right; }};
    container->add_positioned_child(make_stub(80, 36), props);

    container->measure(NanConstraints::tight(400, 300));
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    const auto& child = *container->children()[0];
    EXPECT_FLOAT_EQ(child.x(), 304.0f);   // 400 - 16 - 80
    EXPECT_FLOAT_EQ(child.y(), 248.0f);   // 300 - 16 - 36

    // 改变 lambda 捕获的值，重新 layout
    dynamic_right = 32.0f;
    container->set_bounds(0, 0, 400, 300);
    container->layout();

    EXPECT_FLOAT_EQ(child.x(), 288.0f);   // 400 - 32 - 80
    EXPECT_FLOAT_EQ(child.y(), 232.0f);   // 300 - 32 - 36
}
