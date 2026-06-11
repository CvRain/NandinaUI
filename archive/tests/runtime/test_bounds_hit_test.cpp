#include <gtest/gtest.h>
#include <memory>
#include <thorvg-1/thorvg.h>
#include <vector>

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;

// ============================================================
// Helper: 用于测试的可命中 Widget 子类
// ============================================================
class TestWidget final: public nandina::runtime::NanWidget {
public:
    using NanWidget::NanWidget;

    static auto create() -> std::unique_ptr<TestWidget> {
        return std::make_unique<TestWidget>();
    }
};

// ============================================================
// Issue 013: Bounds 测试
// ============================================================

TEST(NanWidgetBounds, DefaultValues) {
    TestWidget widget;
    EXPECT_FLOAT_EQ(widget.x(), 0.0f);
    EXPECT_FLOAT_EQ(widget.y(), 0.0f);
    EXPECT_FLOAT_EQ(widget.width(), 0.0f);
    EXPECT_FLOAT_EQ(widget.height(), 0.0f);

    const auto rect = widget.bounds();
    EXPECT_FLOAT_EQ(rect.left(), 0.0f);
    EXPECT_FLOAT_EQ(rect.top(), 0.0f);
    EXPECT_FLOAT_EQ(rect.right(), 0.0f);
    EXPECT_FLOAT_EQ(rect.bottom(), 0.0f);
}

TEST(NanWidgetBounds, SetPosition) {
    TestWidget widget;
    widget.set_position(10.0f, 20.0f);
    EXPECT_FLOAT_EQ(widget.x(), 10.0f);
    EXPECT_FLOAT_EQ(widget.y(), 20.0f);

    const auto rect = widget.bounds();
    EXPECT_FLOAT_EQ(rect.left(), 10.0f);
    EXPECT_FLOAT_EQ(rect.top(), 20.0f);
    EXPECT_FLOAT_EQ(rect.right(), 10.0f);
    EXPECT_FLOAT_EQ(rect.bottom(), 20.0f);
}

TEST(NanWidgetBounds, SetSize) {
    TestWidget widget;
    widget.set_size(100.0f, 50.0f);
    EXPECT_FLOAT_EQ(widget.width(), 100.0f);
    EXPECT_FLOAT_EQ(widget.height(), 50.0f);

    const auto rect = widget.bounds();
    EXPECT_FLOAT_EQ(rect.left(), 0.0f);
    EXPECT_FLOAT_EQ(rect.top(), 0.0f);
    EXPECT_FLOAT_EQ(rect.right(), 100.0f);
    EXPECT_FLOAT_EQ(rect.bottom(), 50.0f);
}

TEST(NanWidgetBounds, PositionAndSizeCombined) {
    TestWidget widget;
    widget.set_position(5.0f, 10.0f);
    widget.set_size(200.0f, 100.0f);

    const auto rect = widget.bounds();
    EXPECT_FLOAT_EQ(rect.x(), 5.0f);
    EXPECT_FLOAT_EQ(rect.y(), 10.0f);
    EXPECT_FLOAT_EQ(rect.width(), 200.0f);
    EXPECT_FLOAT_EQ(rect.height(), 100.0f);
    EXPECT_FLOAT_EQ(rect.right(), 205.0f);
    EXPECT_FLOAT_EQ(rect.bottom(), 110.0f);
}

TEST(NanWidgetBounds, NanRectContainsIntegration) {
    using namespace nandina::geometry;

    TestWidget widget;
    widget.set_position(10.0f, 10.0f);
    widget.set_size(100.0f, 100.0f);

    const auto rect = widget.bounds();
    EXPECT_TRUE(rect.contains(NanPoint {10.0f, 10.0f})); // top-left
    EXPECT_TRUE(rect.contains(NanPoint {109.0f, 109.0f})); // inside
    EXPECT_TRUE(rect.contains(NanPoint {110.0f, 110.0f})); // right/bottom edge (inclusive)
    EXPECT_FALSE(rect.contains(NanPoint {0.0f, 0.0f}));
    EXPECT_FALSE(rect.contains(NanPoint {111.0f, 50.0f}));
    EXPECT_FALSE(rect.contains(NanPoint {50.0f, 200.0f}));
}

// ============================================================
// Issue 013: Dirty 状态测试
// ============================================================

TEST(NanWidgetDirty, InitiallyDirty) {
    TestWidget widget;
    EXPECT_TRUE(widget.dirty());
}

TEST(NanWidgetDirty, ClearDirty) {
    TestWidget widget;
    widget.clear_dirty();
    EXPECT_FALSE(widget.dirty());
}

TEST(NanWidgetDirty, MarkDirty) {
    TestWidget widget;
    widget.clear_dirty();
    widget.mark_dirty();
    EXPECT_TRUE(widget.dirty());
}

TEST(NanWidgetDirty, SetPositionMakesDirty) {
    TestWidget widget;
    widget.clear_dirty();
    widget.set_position(5.0f, 5.0f);
    EXPECT_TRUE(widget.dirty());
}

TEST(NanWidgetDirty, SetSizeMakesDirty) {
    TestWidget widget;
    widget.clear_dirty();
    widget.set_size(100.0f, 100.0f);
    EXPECT_TRUE(widget.dirty());
}

TEST(NanWidgetDirty, SetVisibleMakesDirty) {
    TestWidget widget;
    widget.clear_dirty();
    widget.set_visible(false);
    EXPECT_TRUE(widget.dirty());
}

TEST(NanWidgetDirty, DirtyPropagationToParent) {
    auto parent = TestWidget::create();
    auto child = TestWidget::create();
    auto* child_ptr = child.get();

    parent->add_child(std::move(child));

    parent->clear_dirty();
    child_ptr->clear_dirty();
    EXPECT_FALSE(parent->dirty());
    EXPECT_FALSE(child_ptr->dirty());

    // 修改子节点 → 父节点也应变脏
    child_ptr->set_position(10.0f, 10.0f);
    EXPECT_TRUE(child_ptr->dirty());
    EXPECT_TRUE(parent->dirty());
}

TEST(NanWidgetDirty, AddChildMakesParentDirty) {
    auto parent = TestWidget::create();
    auto child = TestWidget::create();

    parent->clear_dirty();
    parent->add_child(std::move(child));
    EXPECT_TRUE(parent->dirty());
}

// ============================================================
// Issue 013: 可见性测试
// ============================================================

TEST(NanWidgetVisible, DefaultVisible) {
    TestWidget widget;
    EXPECT_TRUE(widget.visible());
}

TEST(NanWidgetVisible, SetVisible) {
    TestWidget widget;
    widget.set_visible(false);
    EXPECT_FALSE(widget.visible());

    widget.set_visible(true);
    EXPECT_TRUE(widget.visible());
}

TEST(NanWidgetOverflow, DefaultsToVisible) {
    TestWidget widget;
    EXPECT_EQ(widget.overflow_behavior(), nandina::runtime::OverflowBehavior::visible);
    EXPECT_FALSE(widget.child_clip_rect().has_value());
    EXPECT_FLOAT_EQ(widget.child_clip_corner_radius(), 0.0f);
}

TEST(NanWidgetOverflow, ClipBehaviorUsesOwnBoundsByDefault) {
    TestWidget widget;
    widget.set_bounds(10.0f, 20.0f, 80.0f, 40.0f);
    widget.set_overflow_behavior(nandina::runtime::OverflowBehavior::clip);

    const auto clip_rect = widget.child_clip_rect();
    ASSERT_TRUE(clip_rect.has_value());
    EXPECT_FLOAT_EQ(clip_rect->x(), 10.0f);
    EXPECT_FLOAT_EQ(clip_rect->y(), 20.0f);
    EXPECT_FLOAT_EQ(clip_rect->width(), 80.0f);
    EXPECT_FLOAT_EQ(clip_rect->height(), 40.0f);
}

// ============================================================
// Issue 014: Hit Test 测试
// ============================================================

TEST(NanWidgetHitTest, HitOnSelf) {
    TestWidget widget;
    widget.set_size(100.0f, 100.0f);

    auto* hit = widget.hit_test(50.0f, 50.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, &widget);
}

TEST(NanWidgetHitTest, MissOutsideBounds) {
    TestWidget widget;
    widget.set_size(100.0f, 100.0f);

    auto* hit = widget.hit_test(150.0f, 150.0f);
    EXPECT_EQ(hit, nullptr);
}

TEST(NanWidgetHitTest, HitOnChild) {
    auto parent = TestWidget::create();
    parent->set_size(200.0f, 200.0f);

    auto child = TestWidget::create();
    child->set_position(50.0f, 50.0f);
    child->set_size(100.0f, 100.0f);
    auto* child_ptr = child.get();
    parent->add_child(std::move(child));

    // 命中子节点区域 → 返回子节点
    auto* hit = parent->hit_test(75.0f, 75.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, child_ptr);

    // 命中父节点但不在子节点内 → 返回父节点
    hit = parent->hit_test(10.0f, 10.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, parent.get());
}

TEST(NanWidgetHitTest, HitOnGrandchild) {
    auto grandparent = TestWidget::create();
    grandparent->set_size(300.0f, 300.0f);

    auto parent = TestWidget::create();
    parent->set_position(50.0f, 50.0f);
    parent->set_size(200.0f, 200.0f);
    auto* parent_ptr = parent.get();

    auto child = TestWidget::create();
    child->set_position(50.0f, 50.0f);
    child->set_size(100.0f, 100.0f);
    auto* child_ptr = child.get();

    parent_ptr->add_child(std::move(child));
    grandparent->add_child(std::move(parent));

    // 命中最深层子节点
    auto* hit = grandparent->hit_test(120.0f, 120.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, child_ptr);
}

TEST(NanWidgetHitTest, ZOrderLastChildOnTop) {
    auto parent = TestWidget::create();
    parent->set_size(200.0f, 200.0f);

    // 两个子节点位置重叠：child1 先添加，child2 后添加（在上层）
    auto child1 = TestWidget::create();
    child1->set_position(0.0f, 0.0f);
    child1->set_size(100.0f, 100.0f);

    auto child2 = TestWidget::create();
    child2->set_position(0.0f, 0.0f);
    child2->set_size(100.0f, 100.0f);
    auto* child2_ptr = child2.get();

    parent->add_child(std::move(child1));
    parent->add_child(std::move(child2));

    // 后添加的 child2 应优先命中
    auto* hit = parent->hit_test(50.0f, 50.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, child2_ptr);
}

TEST(NanWidgetHitTest, InvisibleWidgetNotHit) {
    TestWidget widget;
    widget.set_size(100.0f, 100.0f);
    widget.set_visible(false);

    auto* hit = widget.hit_test(50.0f, 50.0f);
    EXPECT_EQ(hit, nullptr);
}

TEST(NanWidgetHitTest, HitTestVisibleFalse) {
    TestWidget widget;
    widget.set_size(100.0f, 100.0f);
    widget.set_hit_test_visible(false);

    auto* hit = widget.hit_test(50.0f, 50.0f);
    EXPECT_EQ(hit, nullptr);
}

TEST(NanWidgetHitTest, InvisibleChildSkipped) {
    auto parent = TestWidget::create();
    parent->set_size(200.0f, 200.0f);

    auto child = TestWidget::create();
    child->set_position(50.0f, 50.0f);
    child->set_size(100.0f, 100.0f);
    auto* child_ptr = child.get();
    child_ptr->set_visible(false);

    parent->add_child(std::move(child));

    // 子节点不可见 → 命中父节点自身
    auto* hit = parent->hit_test(75.0f, 75.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, parent.get());
}

TEST(NanWidgetHitTest, HitTestVisibleFalseChildNotHit) {
    auto parent = TestWidget::create();
    parent->set_size(200.0f, 200.0f);

    auto child = TestWidget::create();
    child->set_position(50.0f, 50.0f);
    child->set_size(100.0f, 100.0f);
    auto* child_ptr = child.get();
    child_ptr->set_hit_test_visible(false);

    parent->add_child(std::move(child));

    // 子节点 hit_test_visible=false → 命中父节点自身
    auto* hit = parent->hit_test(75.0f, 75.0f);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit, parent.get());
}

// ============================================================
// Draw clip stack 测试
// ============================================================

class ClipCaptureWidget final: public nandina::runtime::NanWidget {
public:
    std::optional<nandina::runtime::ClipEntry> captured_clip;
    bool on_draw_called {false};

    static auto create() -> std::unique_ptr<ClipCaptureWidget> {
        return std::make_unique<ClipCaptureWidget>();
    }

protected:
    void on_draw(tvg::SwCanvas& /*canvas*/) override {
        on_draw_called = true;
        captured_clip = nandina::runtime::active_draw_clip();
    }
};

// RAII helper: initializes ThorVG and creates a SwCanvas
class ThorvgScope {
public:
    ThorvgScope(uint32_t w, uint32_t h): pixels_(w * h, 0u) {
        tvg::Initializer::init(0u);
        canvas_ = tvg::SwCanvas::gen();
        canvas_->target(pixels_.data(), w, w, h, tvg::ColorSpace::ARGB8888);
    }
    ~ThorvgScope() {
        delete canvas_;
        canvas_ = nullptr;
        tvg::Initializer::term();
    }
    auto& canvas() {
        return *canvas_;
    }

private:
    std::vector<uint32_t> pixels_;
    tvg::SwCanvas* canvas_ {nullptr};
};

TEST(NanWidgetDrawClip, NoClipByDefault) {
    ThorvgScope tvg(200, 200);

    auto parent = TestWidget::create();
    parent->set_bounds(0.0f, 0.0f, 200.0f, 200.0f);

    auto child = ClipCaptureWidget::create();
    child->set_bounds(10.0f, 10.0f, 50.0f, 50.0f);
    auto* child_ptr = child.get();
    parent->add_child(std::move(child));

    parent->draw(tvg.canvas());

    EXPECT_TRUE(child_ptr->on_draw_called);
    EXPECT_FALSE(child_ptr->captured_clip.has_value());
}

TEST(NanWidgetDrawClip, ParentClipPropagatesToChildDraw) {
    ThorvgScope tvg(200, 200);

    auto parent = TestWidget::create();
    parent->set_bounds(0.0f, 0.0f, 200.0f, 200.0f);
    parent->set_overflow_behavior(nandina::runtime::OverflowBehavior::clip);

    auto child = ClipCaptureWidget::create();
    child->set_bounds(10.0f, 10.0f, 50.0f, 50.0f);
    auto* child_ptr = child.get();
    parent->add_child(std::move(child));

    parent->draw(tvg.canvas());

    EXPECT_TRUE(child_ptr->on_draw_called);
    ASSERT_TRUE(child_ptr->captured_clip.has_value());
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.x(), 0.0f);
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.y(), 0.0f);
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.width(), 200.0f);
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.height(), 200.0f);
}

TEST(NanWidgetDrawClip, NestedClipIntersects) {
    ThorvgScope tvg(300, 300);

    auto grandparent = TestWidget::create();
    grandparent->set_bounds(0.0f, 0.0f, 300.0f, 300.0f);
    grandparent->set_overflow_behavior(nandina::runtime::OverflowBehavior::clip);

    auto parent = TestWidget::create();
    parent->set_bounds(50.0f, 50.0f, 100.0f, 100.0f);
    parent->set_overflow_behavior(nandina::runtime::OverflowBehavior::clip);
    auto* parent_ptr = parent.get();

    auto child = ClipCaptureWidget::create();
    child->set_bounds(0.0f, 0.0f, 200.0f, 200.0f);
    auto* child_ptr = child.get();

    parent_ptr->add_child(std::move(child));
    grandparent->add_child(std::move(parent));

    grandparent->draw(tvg.canvas());

    EXPECT_TRUE(child_ptr->on_draw_called);
    ASSERT_TRUE(child_ptr->captured_clip.has_value());
    // 交集应为 parent 的 bounds (50,50,100,100)
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.x(), 50.0f);
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.y(), 50.0f);
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.width(), 100.0f);
    EXPECT_FLOAT_EQ(child_ptr->captured_clip->rect.height(), 100.0f);
}
