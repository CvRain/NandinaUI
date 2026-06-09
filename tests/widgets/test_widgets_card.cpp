#include <gtest/gtest.h>

#include <memory>

import nandina.foundation.nan_constraints;
import nandina.foundation.nan_insets;
import nandina.runtime.nan_widget;
import nandina.widgets.card;

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

} // namespace

TEST(WidgetsCardTest, TitleDescriptionAndAccentStateAreStored) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Account")
        .set_description("Manage preferences")
        .set_show_accent(true)
        .set_elevation(4.0f);

    EXPECT_EQ(card->title(), "Account");
    EXPECT_EQ(card->description(), "Manage preferences");
    EXPECT_FLOAT_EQ(card->elevation(), 4.0f);
    EXPECT_TRUE(card->is_layout_dirty());
}

TEST(WidgetsCardTest, PreferredSizeIncludesTitleContentFooterAndPadding) {
    auto card = nandina::widgets::Card::create();
    card->set_padding(nandina::geometry::NanInsets{10.0f});
    card->set_title("Title");
    card->add_child(std::make_unique<FixedWidget>(nandina::geometry::NanSize{80.0f, 40.0f}));
    card->set_footer(std::make_unique<FixedWidget>(nandina::geometry::NanSize{60.0f, 20.0f}));

    const auto preferred = card->preferred_size();

    EXPECT_GT(preferred.width(), 80.0f);
    EXPECT_GT(preferred.height(), 80.0f);
    ASSERT_NE(card->footer(), nullptr);
}

TEST(WidgetsCardTest, DescriptionContributesToHeaderHeight) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Title");
    const auto title_only = card->preferred_size();

    card->set_description("Longer supporting text for the card header");
    const auto with_description = card->preferred_size();

    EXPECT_GT(with_description.height(), title_only.height());
}

TEST(WidgetsCardTest, DescriptionOnlyStillKeepsHeaderLayout) {
    auto card = nandina::widgets::Card::create();
    card->set_description("Description without title");

    const auto preferred = card->preferred_size();
    EXPECT_GT(preferred.height(), 0.0f);
}

TEST(WidgetsCardTest, SmallSizeReducesCardPaddingFootprint) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Title");
    card->add_child(std::make_unique<FixedWidget>(nandina::geometry::NanSize{120.0f, 40.0f}));
    const auto regular = card->preferred_size();

    card->set_size(nandina::widgets::CardSize::sm);
    const auto compact = card->preferred_size();

    EXPECT_LT(compact.height(), regular.height());
}

TEST(WidgetsCardTest, CardSpacingIncreasesPreferredHeight) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Title");
    card->set_description("Description");
    card->add_child(std::make_unique<FixedWidget>(nandina::geometry::NanSize{120.0f, 40.0f}));
    const auto base = card->preferred_size();

    card->set_card_spacing(28.0f);
    const auto spaced = card->preferred_size();

    EXPECT_GT(spaced.height(), base.height());
}

TEST(WidgetsCardTest, HeaderActionOnlyStillKeepsHeaderLayout) {
    auto card = nandina::widgets::Card::create();
    card->set_header_action(std::make_unique<FixedWidget>(nandina::geometry::NanSize{56.0f, 24.0f}));

    const auto preferred = card->preferred_size();

    ASSERT_NE(card->header_action(), nullptr);
    EXPECT_GT(preferred.height(), 0.0f);
    EXPECT_GT(preferred.width(), 0.0f);
}

TEST(WidgetsCardTest, HeaderActionWithTitleKeepsHeaderLayout) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Title");
    card->set_header_action(std::make_unique<FixedWidget>(nandina::geometry::NanSize{72.0f, 24.0f}));
    const auto with_action = card->preferred_size();

    ASSERT_NE(card->header_action(), nullptr);
    EXPECT_GT(with_action.height(), 0.0f);
}

TEST(WidgetsCardTest, NarrowHeaderActionStacksBelowHeaderText) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Workspace Settings")
        .set_description("A longer description that should keep readable header text when the card becomes narrow.")
        .set_header_action(std::make_unique<FixedWidget>(nandina::geometry::NanSize{96.0f, 24.0f}));

    card->measure(nandina::geometry::NanConstraints::tight(360.0f, 220.0f));
    const auto wide = card->measured_size();

    card->measure(nandina::geometry::NanConstraints::tight(220.0f, 260.0f));
    const auto narrow = card->measured_size();

    EXPECT_GT(narrow.height(), wide.height());
}

TEST(WidgetsCardTest, NarrowHeaderActionRemainsInsideCardBounds) {
    auto card = nandina::widgets::Card::create();
    card->set_title("Workspace Settings")
        .set_description("Header action should remain inside bounds when stacked.")
        .set_header_action(std::make_unique<FixedWidget>(nandina::geometry::NanSize{96.0f, 24.0f}));

    card->measure(nandina::geometry::NanConstraints::tight(220.0f, 260.0f));
    card->set_bounds(10.0f, 12.0f, 220.0f, 260.0f);
    card->layout();

    ASSERT_NE(card->header_action(), nullptr);
    EXPECT_GE(card->header_action()->bounds().x(), card->bounds().x());
    EXPECT_LE(card->header_action()->bounds().right(), card->bounds().right());
    EXPECT_GT(card->header_action()->bounds().y(), card->bounds().y());
}

TEST(WidgetsCardTest, LayoutPlacesContentBetweenHeaderAndFooter) {
    auto card = nandina::widgets::Card::create();
    card->set_padding(nandina::geometry::NanInsets{8.0f});
    card->set_title("Title");

    auto content = std::make_unique<FixedWidget>(nandina::geometry::NanSize{80.0f, 40.0f});
    auto* content_ptr = content.get();
    card->add_child(std::move(content));

    auto footer = std::make_unique<FixedWidget>(nandina::geometry::NanSize{80.0f, 20.0f});
    card->set_footer(std::move(footer));
    auto* footer_ptr = card->footer();

    card->measure(nandina::geometry::NanConstraints::tight(240.0f, 180.0f));
    card->set_bounds(4.0f, 6.0f, 240.0f, 180.0f);
    card->layout();

    ASSERT_NE(content_ptr, nullptr);
    ASSERT_NE(footer_ptr, nullptr);
    EXPECT_GT(content_ptr->bounds().y(), card->bounds().y());
    EXPECT_LT(content_ptr->bounds().bottom(), footer_ptr->bounds().y() + 1.0f);
    EXPECT_FLOAT_EQ(footer_ptr->bounds().bottom(), card->bounds().bottom());
}
