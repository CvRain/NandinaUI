#include <gtest/gtest.h>

import nandina.layout.container;
import nandina.layout.core;
import nandina.runtime.nan_widget;
import nandina.foundation.nan_constraints;
import nandina.foundation.nan_point;
import nandina.foundation.nan_size;

using namespace nandina::layout;
using namespace nandina::geometry;

namespace {

class FixedWidget final : public nandina::runtime::NanWidget {
public:
    explicit FixedWidget(const NanSize preferred)
        : preferred_(preferred) {
    }

    [[nodiscard]] auto preferred_size() const noexcept -> NanSize override {
        return preferred_;
    }

    auto measure(const NanConstraints& constraints) -> void override {
        set_measured_layout_state(constraints, constraints.constrain(preferred_));
    }

private:
    NanSize preferred_{};
};

} // namespace

// ─── BasicLayoutBackend: Column ──────────────────────────────

TEST(LayoutCoreTest, Column_SingleChild_FillsAvailable) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 50.0f}, .flex_factor = 0}
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
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 80.0f}, .flex_factor = 0},
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
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 50.0f}, .flex_factor = 0},  // fixed 50
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 0.0f}, .flex_factor = 1},    // flex 1
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 0.0f}, .flex_factor = 2},    // flex 2 (占 2/3 剩余空间)
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
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{150.0f, 60.0f}, .flex_factor = 0},
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
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 20.0f}, .flex_factor = 0},
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
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 50.0f}, .flex_factor = 0},
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
        LayoutChildSpec{.preferred_size = NanSize{40.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{40.0f, 50.0f}, .flex_factor = 0},
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
        LayoutChildSpec{.preferred_size = NanSize{200.0f, 200.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 100.0f}, .flex_factor = 0},
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

TEST(LayoutCoreTest, Row_StretchAlignmentRespectsChildMaxHeight) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.cross_alignment = LayoutAlignment::stretch;
    req.children = {
        LayoutChildSpec{
            .preferred_size = NanSize{50.0f, 20.0f},
            .min_size = NanSize{0.0f, 0.0f},
            .max_size = NanSize{NanConstraints::k_infinity, 40.0f},
        },
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].height(), 40.0f);
}

TEST(LayoutCoreTest, Column_FlexChildRespectsMinAndMaxHeight) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 300.0f};
    req.children = {
        LayoutChildSpec{
            .preferred_size = NanSize{100.0f, 0.0f},
            .min_size = NanSize{0.0f, 80.0f},
            .max_size = NanSize{NanConstraints::k_infinity, 120.0f},
            .flex_factor = 1,
        },
        LayoutChildSpec{
            .preferred_size = NanSize{100.0f, 0.0f},
            .min_size = NanSize{0.0f, 80.0f},
            .max_size = NanSize{NanConstraints::k_infinity, 120.0f},
            .flex_factor = 1,
        },
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].height(), 120.0f);
    EXPECT_FLOAT_EQ(frames[1].height(), 120.0f);
}

TEST(LayoutCoreTest, Column_SetBoundsDoesNotAutoLayoutChildren) {
    auto column = Column::Create();
    auto* child = column->add(std::make_unique<FixedWidget>(NanSize{40.0f, 20.0f})).children().back().get();

    column->measure(NanConstraints::tight(200.0f, 100.0f));
    column->set_bounds(10.0f, 20.0f, 200.0f, 100.0f);

    const auto child_bounds = child->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 0.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 0.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 0.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 0.0f);

    column->layout();

    const auto laid_out_bounds = child->bounds();
    EXPECT_FLOAT_EQ(laid_out_bounds.x(), 10.0f);
    EXPECT_FLOAT_EQ(laid_out_bounds.y(), 20.0f);
    EXPECT_FLOAT_EQ(laid_out_bounds.width(), 40.0f);
    EXPECT_FLOAT_EQ(laid_out_bounds.height(), 20.0f);
}

TEST(LayoutCoreTest, Column_LayoutExplicitlyPropagatesToNestedContainers) {
    auto parent = Column::Create();
    auto nested = Column::Create();
    auto* leaf = nested->add(std::make_unique<FixedWidget>(NanSize{60.0f, 30.0f})).children().back().get();
    parent->add(std::move(nested));

    parent->measure(NanConstraints::tight(240.0f, 160.0f));
    parent->set_bounds(4.0f, 6.0f, 240.0f, 160.0f);
    parent->layout();

    const auto leaf_bounds = leaf->bounds();
    EXPECT_FLOAT_EQ(leaf_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(leaf_bounds.y(), 6.0f);
    EXPECT_FLOAT_EQ(leaf_bounds.width(), 60.0f);
    EXPECT_FLOAT_EQ(leaf_bounds.height(), 30.0f);
}

// ─── 回归矩阵：Padding 行为 ──────────────────────────────────

TEST(LayoutCoreTest, Column_WithPadding_ReducesAvailableSpace) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.padding = {20.0f, 20.0f, 20.0f, 20.0f};  // 均匀 20px
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 50.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    // padding 使子节点起始偏移 (20, 20)
    EXPECT_FLOAT_EQ(frames[0].x(), 20.0f);
    EXPECT_FLOAT_EQ(frames[0].y(), 20.0f);
    EXPECT_FLOAT_EQ(frames[0].width(), 80.0f);   // preferred，未 stretch
    EXPECT_FLOAT_EQ(frames[0].height(), 50.0f);
}

TEST(LayoutCoreTest, Row_WithPadding_ReducesAvailableSpace) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 300.0f, 100.0f};
    req.padding = {10.0f, 5.0f, 10.0f, 5.0f};  // left=10, top=5, right=10, bottom=5
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{60.0f, 40.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 40.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    // 第一个子节点从 x=10 开始（左 padding）
    EXPECT_FLOAT_EQ(frames[0].x(), 10.0f);
    EXPECT_FLOAT_EQ(frames[0].y(), 5.0f);   // 顶部 padding
    // 第二个子节点紧跟第一个（无 gap）
    EXPECT_FLOAT_EQ(frames[1].x(), 70.0f);  // 10 + 60
}

TEST(LayoutCoreTest, Column_PaddingAndGap_Combined) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.padding = {10.0f, 10.0f, 10.0f, 10.0f};
    req.gap = 15.0f;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 80.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].y(), 10.0f);           // 顶部 padding
    EXPECT_FLOAT_EQ(frames[0].height(), 50.0f);
    EXPECT_FLOAT_EQ(frames[1].y(), 75.0f);           // 10 + 50 + 15
    EXPECT_FLOAT_EQ(frames[1].height(), 80.0f);
}

TEST(LayoutCoreTest, Column_PaddingWithFlexChild) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.padding = {20.0f, 20.0f, 20.0f, 20.0f};  // 上下各 20
    req.gap = 10.0f;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 60.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 0.0f},  .flex_factor = 1},
    };

    // available_height = 400 - 40 = 360
    // fixed child: 60, gap: 10, flex remaining: 360 - 60 - 10 = 290
    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].y(), 20.0f);           // 顶部 padding
    EXPECT_FLOAT_EQ(frames[0].height(), 60.0f);
    EXPECT_FLOAT_EQ(frames[1].y(), 90.0f);           // 20 + 60 + 10
    EXPECT_NEAR(frames[1].height(), 290.0f, 1e-4f);  // 剩余空间
}

// ─── 回归矩阵：对齐行为 ──────────────────────────────────────

TEST(LayoutCoreTest, Column_JustifyContent_End) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.main_alignment = LayoutAlignment::end;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 50.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].y(), 350.0f);  // 400 - 50
    EXPECT_FLOAT_EQ(frames[0].height(), 50.0f);
}

TEST(LayoutCoreTest, Column_JustifyContent_Center) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.main_alignment = LayoutAlignment::center;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{100.0f, 50.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].y(), 175.0f);  // (400 - 50) / 2
}

TEST(LayoutCoreTest, Column_CrossAlignment_Center) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.cross_alignment = LayoutAlignment::center;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 50.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].x(), 60.0f);   // (200 - 80) / 2
    EXPECT_FLOAT_EQ(frames[0].width(), 80.0f);
}

TEST(LayoutCoreTest, Column_CrossAlignment_End) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.cross_alignment = LayoutAlignment::end;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 50.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].x(), 120.0f);  // 200 - 80
}

TEST(LayoutCoreTest, Row_JustifyContent_End) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 300.0f, 100.0f};
    req.main_alignment = LayoutAlignment::end;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 50.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].x(), 250.0f);  // 300 - 50
}

TEST(LayoutCoreTest, Row_CrossAlignment_Center) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.cross_alignment = LayoutAlignment::center;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 30.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].y(), 35.0f);   // (100 - 30) / 2
    EXPECT_FLOAT_EQ(frames[0].height(), 30.0f);
}

TEST(LayoutCoreTest, Row_CrossAlignment_End) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.cross_alignment = LayoutAlignment::end;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 30.0f}, .flex_factor = 0},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].y(), 70.0f);   // 100 - 30
}

// ─── 回归矩阵：Flex + Gap 组合 ───────────────────────────────

TEST(LayoutCoreTest, Row_FlexWithGap_AccountsForGap) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 300.0f, 100.0f};
    req.gap = 10.0f;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{0.0f,  50.0f}, .flex_factor = 1},
        LayoutChildSpec{.preferred_size = NanSize{50.0f, 50.0f}, .flex_factor = 0},
    };

    // 固定: 50+50=100, gap: 2×10=20, flex 可用: 300-100-20=180
    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 3);
    EXPECT_FLOAT_EQ(frames[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(frames[0].width(), 50.0f);
    EXPECT_FLOAT_EQ(frames[1].x(), 60.0f);          // 50 + 10
    EXPECT_NEAR(frames[1].width(), 180.0f, 1e-4f);
    EXPECT_NEAR(frames[2].x(), 60.0f + 180.0f + 10.0f, 1e-4f);  // flex_end + gap
    EXPECT_FLOAT_EQ(frames[2].width(), 50.0f);
}

TEST(LayoutCoreTest, Column_FlexWithGap_AccountsForGap) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.gap = 20.0f;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 60.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 0.0f},  .flex_factor = 1},
    };

    // fixed: 60, gap: 20, flex: 400-60-20=320
    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].height(), 60.0f);
    EXPECT_FLOAT_EQ(frames[1].y(), 80.0f);           // 60 + 20
    EXPECT_NEAR(frames[1].height(), 320.0f, 1e-4f);
}

TEST(LayoutCoreTest, Row_ThreeEqualFlexChildren) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 300.0f, 100.0f};
    req.gap = 0.0f;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{0.0f, 50.0f}, .flex_factor = 1},
        LayoutChildSpec{.preferred_size = NanSize{0.0f, 50.0f}, .flex_factor = 1},
        LayoutChildSpec{.preferred_size = NanSize{0.0f, 50.0f}, .flex_factor = 1},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 3);
    EXPECT_NEAR(frames[0].width(), 100.0f, 1e-4f);
    EXPECT_NEAR(frames[1].width(), 100.0f, 1e-4f);
    EXPECT_NEAR(frames[2].width(), 100.0f, 1e-4f);
    EXPECT_NEAR(frames[0].x(), 0.0f,   1e-4f);
    EXPECT_NEAR(frames[1].x(), 100.0f, 1e-4f);
    EXPECT_NEAR(frames[2].x(), 200.0f, 1e-4f);
}

TEST(LayoutCoreTest, Column_SingleFlexFillsContainer) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 0.0f}, .flex_factor = 1},
    };

    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_FLOAT_EQ(frames[0].y(), 0.0f);
    EXPECT_NEAR(frames[0].height(), 400.0f, 1e-4f);
}

// ─── 回归矩阵：边界条件 ──────────────────────────────────────

TEST(LayoutCoreTest, Column_Empty_ReturnsEmptyFrames) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 400.0f};
    req.children = {};

    auto frames = backend.compute(req);
    EXPECT_TRUE(frames.empty());
}

TEST(LayoutCoreTest, Row_Empty_ReturnsEmptyFrames) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 300.0f, 100.0f};
    req.children = {};

    auto frames = backend.compute(req);
    EXPECT_TRUE(frames.empty());
}

TEST(LayoutCoreTest, Stack_Empty_ReturnsEmptyFrames) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::stack;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 200.0f};
    req.children = {};

    auto frames = backend.compute(req);
    EXPECT_TRUE(frames.empty());
}

TEST(LayoutCoreTest, Row_SpaceAround_TwoChildren) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::row;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 100.0f};
    req.main_alignment = LayoutAlignment::space_around;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{40.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{40.0f, 50.0f}, .flex_factor = 0},
    };

    // space_around: 总空白=120, 每侧1份，中间2份 → 每份=30
    // child0.x=30, child1.x=30+40+60=130
    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].x(), 30.0f);
    EXPECT_FLOAT_EQ(frames[1].x(), 130.0f);
}

TEST(LayoutCoreTest, Column_SpaceBetween_TwoChildren) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::column;
    req.container_bounds = {0.0f, 0.0f, 200.0f, 300.0f};
    req.main_alignment = LayoutAlignment::space_between;
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 50.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{80.0f, 50.0f}, .flex_factor = 0},
    };

    // 总高 300, 两子各 50, 剩余 200 分布在中间
    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(frames[1].y(), 250.0f);  // 300 - 50
}

TEST(LayoutCoreTest, Stack_FillsContainerBounds) {
    BasicLayoutBackend backend;

    LayoutRequest req;
    req.axis = LayoutAxis::stack;
    req.container_bounds = {5.0f, 10.0f, 200.0f, 300.0f};
    req.children = {
        LayoutChildSpec{.preferred_size = NanSize{200.0f, 300.0f}, .flex_factor = 0},
        LayoutChildSpec{.preferred_size = NanSize{50.0f,  50.0f},  .flex_factor = 0},
    };

    // Stack 所有子节点均从容器左上角开始
    auto frames = backend.compute(req);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_FLOAT_EQ(frames[0].x(), 5.0f);
    EXPECT_FLOAT_EQ(frames[0].y(), 10.0f);
    EXPECT_FLOAT_EQ(frames[1].x(), 5.0f);
    EXPECT_FLOAT_EQ(frames[1].y(), 10.0f);
}