#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.foundation.nan_constraints;
import nandina.runtime.nan_event;
import nandina.theme;
import nandina.widgets.field;
import nandina.widgets.label;
import nandina.widgets.text_field;

TEST(WidgetsFieldTest, DefaultStateHasEmptySlots) {
    auto field = nandina::widgets::Field::create();
    EXPECT_TRUE(field->label_text().empty());
    EXPECT_TRUE(field->helper_text().empty());
    EXPECT_TRUE(field->error_text().empty());
    EXPECT_FALSE(field->required());
    EXPECT_FALSE(field->invalid());
    EXPECT_FALSE(field->disabled());
    EXPECT_EQ(field->control(), nullptr);
}

TEST(WidgetsFieldTest, LabelAndHelperTextAreSetAndRetrieved) {
    auto field = nandina::widgets::Field::create();

    field->set_label("Username");
    EXPECT_EQ(field->label_text(), "Username");

    field->set_helper_text("Enter your username");
    EXPECT_EQ(field->helper_text(), "Enter your username");

    field->set_error_text("Username is required");
    EXPECT_EQ(field->error_text(), "Username is required");
}

TEST(WidgetsFieldTest, InvalidStateTogglesErrorAndPropagatesToTextField) {
    auto field = nandina::widgets::Field::create();

    auto control = nandina::widgets::TextField::create();
    control->set_placeholder("Type here");
    auto* control_ptr = control.get();
    field->set_control(std::move(control));
    ASSERT_EQ(field->control(), control_ptr);

    field->set_helper_text("Valid input required");
    field->set_error_text("Invalid value");

    EXPECT_FALSE(field->invalid());
    EXPECT_FALSE(control_ptr->invalid());

    field->set_invalid(true);
    EXPECT_TRUE(field->invalid());
    EXPECT_TRUE(control_ptr->invalid());

    field->set_invalid(false);
    EXPECT_FALSE(field->invalid());
    EXPECT_FALSE(control_ptr->invalid());
}

TEST(WidgetsFieldTest, InvalidToggleCyclesHelperErrorVisibility) {
    auto field = nandina::widgets::Field::create();

    auto control = nandina::widgets::TextField::create();
    field->set_control(std::move(control));
    field->set_helper_text("Valid input required");
    field->set_error_text("Invalid value");

    // invalid=false: helper 可见, error 不可见
    field->set_invalid(false);
    EXPECT_FALSE(field->invalid());

    // invalid=true: error 替换 helper
    field->set_invalid(true);
    EXPECT_TRUE(field->invalid());

    // 再切回
    field->set_invalid(false);
    EXPECT_FALSE(field->invalid());

    // 再切过去
    field->set_invalid(true);
    EXPECT_TRUE(field->invalid());
}

TEST(WidgetsFieldTest, ControlReceivesDisabledState) {
    auto field = nandina::widgets::Field::create();

    auto control = nandina::widgets::TextField::create();
    control->set_value("test");
    auto* control_ptr = control.get();
    field->set_control(std::move(control));
    ASSERT_EQ(field->control(), control_ptr);

    EXPECT_FALSE(field->disabled());
    EXPECT_FALSE(control_ptr->disabled());

    field->set_disabled(true);
    EXPECT_TRUE(field->disabled());
    EXPECT_TRUE(control_ptr->disabled());
}

TEST(WidgetsFieldTest, RequiredState) {
    auto field = nandina::widgets::Field::create();
    field->set_label("Email");

    EXPECT_FALSE(field->required());
    field->set_required(true);
    EXPECT_TRUE(field->required());
}

TEST(WidgetsFieldTest, ControlReplacementWorks) {
    auto field = nandina::widgets::Field::create();

    auto first = nandina::widgets::TextField::create();
    first->set_placeholder("First");
    field->set_control(std::move(first));
    EXPECT_NE(field->control(), nullptr);

    auto second = nandina::widgets::TextField::create();
    second->set_placeholder("Second");
    auto* second_ptr = second.get();
    field->set_control(std::move(second));
    EXPECT_EQ(field->control(), second_ptr);
}

TEST(WidgetsFieldTest, MeasureReturnsReasonableSize) {
    auto field = nandina::widgets::Field::create();
    field->set_label("Email Address");
    field->set_helper_text("We will never share your email.");

    auto control = nandina::widgets::TextField::create();
    control->set_placeholder("you@example.com");
    field->set_control(std::move(control));

    field->measure(nandina::geometry::NanConstraints::tight(300.0f, nandina::geometry::NanConstraints::k_infinity));

    const auto ms = field->measured_size();
    EXPECT_GT(ms.width(), 100.0f);
    EXPECT_GT(ms.height(), 50.0f);
}

TEST(WidgetsFieldTest, DisabledPropagatesToTextControl) {
    auto field = nandina::widgets::Field::create();

    auto control = nandina::widgets::TextField::create();
    control->set_value("test");
    auto* control_ptr = control.get();
    field->set_control(std::move(control));

    field->set_disabled(true);
    EXPECT_TRUE(control_ptr->disabled());

    field->set_disabled(false);
    EXPECT_FALSE(control_ptr->disabled());
}

TEST(WidgetsFieldTest, EmptyControlDoesNotCrash) {
    auto field = nandina::widgets::Field::create();
    field->set_label("Just Label");
    field->set_helper_text("No control yet");
    EXPECT_EQ(field->control(), nullptr);

    field->set_invalid(true);
    field->set_disabled(true);
    field->set_required(true);
    // 不应崩溃
}

TEST(WidgetsFieldTest, LabelOnlyNoHelperOrError) {
    auto field = nandina::widgets::Field::create();

    auto control = nandina::widgets::TextField::create();
    control->set_placeholder("Name");
    field->set_control(std::move(control));
    field->set_label("Name");

    field->measure(nandina::geometry::NanConstraints::tight(200.0f, nandina::geometry::NanConstraints::k_infinity));
    const auto ms = field->measured_size();
    EXPECT_GT(ms.height(), 20.0f);
}
