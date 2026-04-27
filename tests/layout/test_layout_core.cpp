#include <gtest/gtest.h>

import nandina.layout.core;
import nandina.foundation.nan_point;
import nandina.foundation.nan_size;

using namespace nandina::layout;
using namespace nandina::geometry;

// ─── BasicLayoutBackend: Column ──────────────────────────────

TEST(LayoutCoreTest, Column_SingleChild_FillsAvailable) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.children = {
        {NanSize{100.0f, 50.0f}, 0}
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].width(), 100.0f);  // cross alignment=start, 使用 preferred
    EXPECT_FLOAT_EQ(frames[0].height(), 50.0f);   // fixed, 使用 preferred
}

TEST(LayoutCoreTest, Column_TwoChildren_StackedVertically) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.gap = 10.0f;
    req.children = {
        {NanSize{100.0f, 50.0f}, 0},
        {NanSize{80.0f, 80.0f}, 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    // 第一个在顶部
    EXPECT_FLOAT_EQ(frames[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].height(), 50.0f);
    // 第二个在第一个下方 + gap
    EXPECT_FLOAT_EQ(frames[1].y(), 60.0f);    // 50 + 10
    EXPECT_FLOAT_EQ(frames[1].height(), 80.0f);
}

TEST(LayoutCoreTest, Column_WithFlexDistributesRemaining) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.gap = 0.0f;
    req.children = {
        {NanSize{100.0f, 50.0f}, 0},  // fixed 50
        {NanSize{80.0f, 0.0f}, 1},    // flex 1
        {NanSize{80.0f, 0.0f}, 2},    // flex 2 (占 2/3 剩余空间)
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 3);
    EXPECT_FLOAT_EQ(frames[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].height(), 50.0f);

    // 剩余: 400 - 50 = 350
    // flex 1 占 1/3 ≈ 116.667
    EXPECT_NEAR(frames[1].y(), 50.0f, 1e-4f);
    EXPECT_NEAR(frames[1].height(), 350.0f / 3.0f, 1e-4f);

    // flex 2 占 2/3 ≈ 233.333
    EXPECT_NEAR(frames[2].y(), 50.0f + 350.0f / 3.0f, 1e-4f);
    EXPECT_NEAR(frames[2].height(), 350.0f * 2.0f / 3.0f, 1e-4f);
}

// ─── BasicLayoutBackend: Row ─────────────────────────────────

TEST(LayoutCoreTest, Row_TwoChildren_ArrangedHorizontally) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 300.0f, 100.0f};
    req.gap = 5.0f;
    req.children = {
        {NanSize{100.0f, 50.0f}, 0},
        {NanSize{150.0f, 60.0f}, 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].width(), 100.0f);

    EXPECT_FLOAT_EQ(frames[1].x(), 105.0f);   // 100 + 5
    EXPECT_FLOAT_EQ(frames[1].width(), 150.0f);
}

TEST(LayoutCoreTest, Row_StretchAlign_FillsHeight) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.cross_alignment = LayoutAlignment::stretch;
    req.children = {
        {NanSize{50.0f, 20.0f}, 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].height(), 100.0f);  // stretch 填满
    EXPECT_FLOAT_EQ(frames[0].width(), 50.0f);    // width 使用 preferred
}

TEST(LayoutCoreTest, Row_CenterAlign_MainAxis) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.main_alignment = LayoutAlignment::center;
    req.children = {
        {NanSize{50.0f, 50.0f}, 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    // center: (200 - 50) / 2 = 75
    EXPECT_FLOAT_EQ(frames[0].x(), 75.0f);
    EXPECT_FLOAT_EQ(frames[0].width(), 50.0f);
}

TEST(LayoutCoreTest, Row_SpaceBetween) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.main_alignment = LayoutAlignment::space_between;
    req.children = {
        {NanSize{40.0f, 50.0f}, 0},
        {NanSize{40.0f, 50.0f}, 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    // 总宽 200, 子节点 40+40=80, 剩余 120, 中间 gap=120
    EXPECT_FLOAT_EQ(frames[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(frames[1].x(), 160.0f);  // 40 + 120
}

// ─── BasicLayoutBackend: Stack ───────────────────────────────

TEST(LayoutCoreTest, Stack_ChildrenOverlap) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::stack;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 200.0f};
    req.children = {
        {NanSize{200.0f, 200.0f}, 0},
        {NanSize{100.0f, 100.0f}, 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    // 都重叠在相同位置
    EXPECT_FLOAT_EQ(frames[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(frames[1].x(), 0.0f);
    EXPECT_FLOAT_EQ(frames[1].y(), 0.0f);
}

// ─── LayoutInsets ────────────────────────────────────────────

TEST(LayoutCoreTest, LayoutInsets_HorizontalVertical) {
    LayoutInsets insets{10.0f, 15.0f, 20.0f, 25.0f};
    EXPECT_FLOAT_EQ(insets.horizontal(), 30.0f);  // 10 + 20
    EXPECT_FLOAT_EQ(insets.vertical(), 40.0f);    // 15 + 25
}

// ─── LayoutRequest content_bounds ────────────────────────────

TEST(LayoutCoreTest, LayoutRequest_ContentBounds) {
    LayoutRequest req;
    req.container_bounds = {10.0f, 20.0f, 200.0f, 300.0f};
    req.padding = {5.0f, 10.0f, 5.0f, 10.0f};

    auto cb = req.content_bounds();
    EXPECT_FLOAT_EQ(cb.x(), 15.0f);         // 10 + 5
    EXPECT_FLOAT_EQ(cb.y(), 30.0f);         // 20 + 10
    EXPECT_FLOAT_EQ(cb.right(), 195.0f);    // 200 - 5
    EXPECT_FLOAT_EQ(cb.bottom(), 290.0f);   // 300 - 10
    EXPECT_FLOAT_EQ(cb.width(), 180.0f);    // 195 - 15
    EXPECT_FLOAT_EQ(cb.height(), 260.0f);   // 290 - 30
}

TEST(LayoutCoreTest, LayoutRequest_ContentBounds_ZeroPadding) {
    LayoutRequest req;
    req.container_bounds = {0.0f, 0.0f, 100.0f, 100.0f};

    auto cb = req.content_bounds();
    EXPECT_FLOAT_EQ(cb.x(), 0.0f);
    EXPECT_FLOAT_EQ(cb.y(), 0.0f);
    EXPECT_FLOAT_EQ(cb.width(), 100.0f);
    EXPECT_FLOAT_EQ(cb.height(), 100.0f);
}