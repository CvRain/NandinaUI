#include <gtest/gtest.h>

#include <memory>

import nandina.app.authoring;
import nandina.runtime.nan_widget;
import nandina.layout;
import nandina.widgets.button;
import nandina.widgets.label;

// ============================================================================
// § .width() / .height() / .size() 自动包裹为 SizedBox
// ============================================================================

TEST(NodeAutoWrapTest, WidthAutoWrapsLabelInSizedBox) {
    using namespace nandina::app;

    auto node  = label("Hello").width(200);
    auto comp  = mount(std::move(node));
    ASSERT_NE(comp, nullptr);

    bool  found = false;
    float w     = 0.0f;
    comp->for_each_child([&](nandina::runtime::NanWidget& child) {
        if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(&child)) {
            found = true;
            w     = sb->preferred_size().width();
        }
    });
    EXPECT_TRUE(found) << "root child should be SizedBox after .width()";
    EXPECT_FLOAT_EQ(w, 200.0f);
}

TEST(NodeAutoWrapTest, HeightAutoWrapsLabelInSizedBox) {
    using namespace nandina::app;

    auto node = label("Hello").height(50);
    auto comp = mount(std::move(node));
    ASSERT_NE(comp, nullptr);

    bool  found = false;
    float h     = 0.0f;
    comp->for_each_child([&](nandina::runtime::NanWidget& child) {
        if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(&child)) {
            found = true;
            h     = sb->preferred_size().height();
        }
    });
    EXPECT_TRUE(found);
    EXPECT_FLOAT_EQ(h, 50.0f);
}

TEST(NodeAutoWrapTest, SizeAutoWrapsLabelInSizedBox) {
    using namespace nandina::app;

    auto node = label("Hello").size({200, 50});
    auto comp = mount(std::move(node));
    ASSERT_NE(comp, nullptr);

    bool  found = false;
    float w = 0.0f, h = 0.0f;
    comp->for_each_child([&](nandina::runtime::NanWidget& child) {
        if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(&child)) {
            found = true;
            w = sb->preferred_size().width();
            h = sb->preferred_size().height();
        }
    });
    EXPECT_TRUE(found);
    EXPECT_FLOAT_EQ(w, 200.0f);
    EXPECT_FLOAT_EQ(h, 50.0f);
}

TEST(NodeAutoWrapTest, WidthOnSizedBoxDoesNotDoubleWrap) {
    using namespace nandina::app;

    auto node = sized_box(label("X")).width(150);
    auto comp = mount(std::move(node));
    ASSERT_NE(comp, nullptr);

    int   sb_count = 0;
    float found_w  = 0.0f;
    comp->for_each_child([&](nandina::runtime::NanWidget& child) {
        if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(&child)) {
            ++sb_count;
            found_w = sb->preferred_size().width();
            sb->for_each_child([&](nandina::runtime::NanWidget& inner) {
                EXPECT_EQ(dynamic_cast<nandina::layout::SizedBox*>(&inner), nullptr)
                    << "should not double-wrap";
            });
        }
    });
    EXPECT_EQ(sb_count, 1);
    EXPECT_FLOAT_EQ(found_w, 150.0f);
}

TEST(NodeAutoWrapTest, ChainedWidthThenHeightWrapsOnce) {
    using namespace nandina::app;

    auto node = label("X").width(200).height(50);
    auto comp = mount(std::move(node));
    ASSERT_NE(comp, nullptr);

    int sb_count = 0;
    comp->for_each_child([&](nandina::runtime::NanWidget& child) {
        if (auto* sb = dynamic_cast<nandina::layout::SizedBox*>(&child)) {
            ++sb_count;
            EXPECT_FLOAT_EQ(sb->preferred_size().width(), 200.0f);
            EXPECT_FLOAT_EQ(sb->preferred_size().height(), 50.0f);
        }
    });
    EXPECT_EQ(sb_count, 1);
}

// ============================================================================
// § children() 接受 lvalue，无需 std::move
// ============================================================================

TEST(ChildrenNoMoveTest, AcceptsLvaluesWithoutStdMove) {
    using namespace nandina::app;

    auto a = label("A");
    auto b = label("B");
    auto c = label("C");

    auto kids = children(a, b, c);
    EXPECT_EQ(kids.size(), 3u);
}

TEST(ChildrenNoMoveTest, AcceptsMixedLvalueRvalue) {
    using namespace nandina::app;

    auto a = label("A");
    auto kids = children(a, label("B"), label("C"));

    EXPECT_EQ(kids.size(), 3u);
}

TEST(ChildrenNoMoveTest, AcceptsAllRvalues) {
    using namespace nandina::app;

    auto kids = children(label("X"), button("Y"), label("Z"));
    EXPECT_EQ(kids.size(), 3u);
}

TEST(ChildrenNoMoveTest, MountWithLvalueChildren) {
    using namespace nandina::app;

    auto heading = label("Heading");
    auto btn     = button("OK");

    auto component = mount(
        column(children(
            heading,
            row(children(btn, label("Extra")))
        ))
    );
    ASSERT_NE(component, nullptr);
}

TEST(ChildrenNoMoveTest, EmptyChildren) {
    using namespace nandina::app;

    auto kids = children();
    EXPECT_EQ(kids.size(), 0u);
}
