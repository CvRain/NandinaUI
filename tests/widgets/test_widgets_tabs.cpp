#include <gtest/gtest.h>

#include <memory>

import nandina.foundation.nan_constraints;
import nandina.runtime.nan_widget;
import nandina.theme;
import nandina.widgets.button;
import nandina.widgets.tabs;

namespace {

class FixedWidget final : public nandina::runtime::NanWidget {
public:
    explicit FixedWidget(nandina::geometry::NanSize preferred)
        : preferred_(preferred) {}

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return preferred_;
    }

    auto measure(const nandina::geometry::NanConstraints& constraints) -> void override {
        set_measured_layout_state(constraints, constraints.constrain(preferred_));
    }

private:
    nandina::geometry::NanSize preferred_{};
};

auto count_buttons(nandina::runtime::NanWidget& widget) -> std::size_t {
    std::size_t total = 0;
    for (auto& child : widget.children()) {
        if (dynamic_cast<nandina::widgets::Button*>(child.get())) {
            ++total;
        }
    }
    return total;
}

} // namespace

TEST(WidgetsTabsTest, AddTabCreatesTriggersAndSelectsFirstContent) {
    auto tabs = nandina::widgets::Tabs::create();
    tabs->add_tab("Overview", std::make_unique<FixedWidget>(nandina::geometry::NanSize{120.0f, 40.0f}))
        .add_tab("Details", std::make_unique<FixedWidget>(nandina::geometry::NanSize{160.0f, 60.0f}));

    EXPECT_EQ(tabs->tab_count(), 2u);
    EXPECT_EQ(tabs->active_index(), 0u);
    EXPECT_EQ(count_buttons(*tabs), 2u);
    ASSERT_NE(tabs->active_content(), nullptr);
    EXPECT_TRUE(tabs->active_content()->visible());
    EXPECT_TRUE(tabs->children()[3]->visible() == false);
}

TEST(WidgetsTabsTest, ActiveIndexSwitchesVisibleContentAndEmitsSignal) {
    auto tabs = nandina::widgets::Tabs::create();
    tabs->add_tab("Overview", std::make_unique<FixedWidget>(nandina::geometry::NanSize{120.0f, 40.0f}))
        .add_tab("Details", std::make_unique<FixedWidget>(nandina::geometry::NanSize{160.0f, 60.0f}));

    int changes = 0;
    std::size_t last_index = 0;
    tabs->on_changed([&](const std::size_t index) {
        ++changes;
        last_index = index;
    });

    tabs->active_index(1);

    EXPECT_EQ(changes, 1);
    EXPECT_EQ(last_index, 1u);
    EXPECT_EQ(tabs->active_index(), 1u);
    EXPECT_FALSE(tabs->children()[1]->visible());
    EXPECT_TRUE(tabs->children()[3]->visible());
}

TEST(WidgetsTabsTest, PreferredSizeAndLayoutReserveHeaderAndContentArea) {
    auto tabs = nandina::widgets::Tabs::create();
    tabs->add_tab("Overview", std::make_unique<FixedWidget>(nandina::geometry::NanSize{120.0f, 40.0f}))
        .add_tab("Details", std::make_unique<FixedWidget>(nandina::geometry::NanSize{160.0f, 60.0f}));

    const auto preferred = tabs->preferred_size();
    EXPECT_GT(preferred.width(), 160.0f);
    EXPECT_GT(preferred.height(), 100.0f);

    tabs->measure(nandina::geometry::NanConstraints::tight(360.0f, 220.0f));
    tabs->set_bounds(10.0f, 20.0f, 360.0f, 220.0f);
    tabs->layout();

    auto* active = tabs->active_content();
    ASSERT_NE(active, nullptr);
    EXPECT_GT(active->bounds().y(), tabs->y());
    EXPECT_LT(active->bounds().width(), tabs->width() + 1.0f);
    EXPECT_LT(active->bounds().height(), tabs->height());
}
