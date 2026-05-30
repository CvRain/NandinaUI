#include <gtest/gtest.h>

#include <memory>

import nandina.foundation.nan_constraints;
import nandina.foundation.nan_insets;
import nandina.runtime.nan_widget;
import nandina.widgets.panel;

namespace {

class FixedWidget final : public nandina::runtime::NanWidget {
public:
    static auto create() -> std::unique_ptr<FixedWidget> {
        return std::make_unique<FixedWidget>();
    }

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return {24.0f, 12.0f};
    }
};

} // namespace

TEST(WidgetsPanelTest, PreferredSizeIncludesHeaderAndPadding) {
    auto panel = nandina::widgets::Panel::create();
    panel->set_header_height(28.0f);
    panel->set_padding(nandina::geometry::NanInsets{10.0f});
    panel->add_child(FixedWidget::create());

    const auto preferred = panel->preferred_size();
    EXPECT_FLOAT_EQ(preferred.width(), 44.0f);
    EXPECT_FLOAT_EQ(preferred.height(), 60.0f);
}

TEST(WidgetsPanelTest, LayoutPlacesContentBelowHeaderAndPadding) {
    auto panel = nandina::widgets::Panel::create();
    panel->set_header_height(30.0f);
    panel->set_padding(nandina::geometry::NanInsets{8.0f, 6.0f, 10.0f, 12.0f});

    auto child = FixedWidget::create();
    auto* child_ptr = child.get();
    panel->add_child(std::move(child));

    panel->measure(nandina::geometry::NanConstraints::tight(200.0f, 120.0f));
    panel->set_bounds(5.0f, 7.0f, 200.0f, 120.0f);
    panel->layout();

    ASSERT_NE(child_ptr, nullptr);
    EXPECT_FLOAT_EQ(child_ptr->bounds().x(), 13.0f);
    EXPECT_FLOAT_EQ(child_ptr->bounds().y(), 43.0f);
    EXPECT_FLOAT_EQ(child_ptr->bounds().width(), 182.0f);
    EXPECT_FLOAT_EQ(child_ptr->bounds().height(), 72.0f);
}
