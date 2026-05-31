#include <gtest/gtest.h>

#include <SDL3/SDL.h>

import nandina.foundation.color;
import nandina.runtime.nan_event;
import nandina.theme;
import nandina.widgets.focus_ring;
import nandina.widgets.label;
import nandina.widgets.text_field;

namespace {

auto find_internal_label(nandina::widgets::TextField& text_field) -> nandina::widgets::Label* {
    for (auto& child : text_field.children()) {
        auto* label = dynamic_cast<nandina::widgets::Label*>(child.get());
        if (label) {
            return label;
        }
    }
    return nullptr;
}

auto find_focus_ring(nandina::widgets::TextField& text_field) -> nandina::widgets::FocusRing* {
    for (auto& child : text_field.children()) {
        auto* focus_ring = dynamic_cast<nandina::widgets::FocusRing*>(child.get());
        if (focus_ring) {
            return focus_ring;
        }
    }
    return nullptr;
}

} // namespace

TEST(WidgetsTextFieldTest, EmptyValueShowsPlaceholderAndFocusRingTracksFocus) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_placeholder("Email");

    auto* label = find_internal_label(*text_field);
    auto* focus_ring = find_focus_ring(*text_field);
    ASSERT_NE(label, nullptr);
    ASSERT_NE(focus_ring, nullptr);

    EXPECT_TRUE(text_field->value().empty());
    EXPECT_EQ(label->text(), "Email");

    const auto placeholder = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(placeholder.red(), 154u);
    EXPECT_EQ(placeholder.green(), 157u);
    EXPECT_EQ(placeholder.blue(), 180u);

    EXPECT_FALSE(focus_ring->active());
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(focus_ring->active());

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = false}));
    EXPECT_FALSE(focus_ring->active());
}

TEST(WidgetsTextFieldTest, TextInputBackspaceDeleteAndSubmitUpdateValue) {
    auto text_field = nandina::widgets::TextField::create();
    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    int change_count = 0;
    int submit_count = 0;
    std::string last_value;
    std::string submitted_value;

    text_field->on_change([&](std::string_view value) {
        ++change_count;
        last_value = std::string{value};
    });
    text_field->on_submit([&](std::string_view value) {
        ++submit_count;
        submitted_value = std::string{value};
    });

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "abc"}));
    EXPECT_EQ(text_field->value(), "abc");
    EXPECT_EQ(label->text(), "abc");
    EXPECT_EQ(change_count, 1);
    EXPECT_EQ(last_value, "abc");

    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_LEFT, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_BACKSPACE, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_EQ(text_field->value(), "ac");
    EXPECT_EQ(change_count, 2);
    EXPECT_EQ(last_value, "ac");

    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_DELETE, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_EQ(text_field->value(), "a");
    EXPECT_EQ(change_count, 3);
    EXPECT_EQ(last_value, "a");

    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_RETURN, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_EQ(submit_count, 1);
    EXPECT_EQ(submitted_value, "a");
}

TEST(WidgetsTextFieldTest, DisabledAndReadOnlyBlockMutationPaths) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("seed");

    int change_count = 0;
    text_field->on_change([&](std::string_view) {
        ++change_count;
    });

    text_field->set_read_only(true);
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_FALSE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "x"}));
    EXPECT_EQ(text_field->value(), "seed");
    EXPECT_EQ(change_count, 0);

    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_BACKSPACE, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_EQ(text_field->value(), "seed");
    EXPECT_EQ(change_count, 0);

    text_field->set_read_only(false);
    text_field->set_disabled(true);
    EXPECT_FALSE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "x"}));
    EXPECT_FALSE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_BACKSPACE, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_FALSE(text_field->focused());
    EXPECT_EQ(text_field->value(), "seed");
    EXPECT_EQ(change_count, 0);
}

TEST(WidgetsTextFieldTest, SetValueDoesNotTriggerOnChange) {
    auto text_field = nandina::widgets::TextField::create();

    int change_count = 0;
    text_field->on_change([&](std::string_view) {
        ++change_count;
    });

    text_field->set_value("hello");
    EXPECT_EQ(text_field->value(), "hello");
    EXPECT_EQ(change_count, 0);

    text_field->set_value("world");
    EXPECT_EQ(change_count, 0);

    // 只有用户输入才触发 on_change
    text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "x"});
    EXPECT_EQ(change_count, 0); // 未 focus，输入被忽略
}

TEST(WidgetsTextFieldTest, CaretRightKeyMovesPosition) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("abc");

    text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true});

    // caret 默认在末尾，先左移再右移
    text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_LEFT, .is_repeat = false},
        nandina::runtime::EventType::KeyDown);
    text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_LEFT, .is_repeat = false},
        nandina::runtime::EventType::KeyDown);

    // 在 "a" 之后插入
    text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "X"});
    EXPECT_EQ(text_field->value(), "aXbc");
}

TEST(WidgetsTextFieldTest, ReadOnlyAllowsFocusButBlocksMutation) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("original");

    text_field->set_read_only(true);
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(text_field->focused());

    // 事件仍被消费（return true），但值不变
    text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_BACKSPACE, .is_repeat = false},
        nandina::runtime::EventType::KeyDown);
    EXPECT_EQ(text_field->value(), "original");

    // 文本输入被阻断
    EXPECT_FALSE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "x"}));
    EXPECT_EQ(text_field->value(), "original");

    // submit 事件仍可触发
    text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_RETURN, .is_repeat = false},
        nandina::runtime::EventType::KeyDown);
    EXPECT_EQ(text_field->value(), "original");
}

TEST(WidgetsTextFieldTest, MultipleTextInputsAccumulateCorrectly) {
    auto text_field = nandina::widgets::TextField::create();

    text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true});

    text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "h"});
    EXPECT_EQ(text_field->value(), "h");

    text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "e"});
    EXPECT_EQ(text_field->value(), "he");

    text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "llo"});
    EXPECT_EQ(text_field->value(), "hello");
}

TEST(WidgetsTextFieldTest, PointerDownMovesCaretToClickedPosition) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("abcd");
    text_field->set_bounds(0.0f, 0.0f, 240.0f, 40.0f);
    text_field->layout();

    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    const float click_x = text_field->padding().left() + label->font().estimate_text_width("ab") + 1.0f;
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = click_x,
            .y = 20.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "X"}));

    EXPECT_EQ(text_field->value(), "abXcd");
}

TEST(WidgetsTextFieldTest, DragSelectionReplacesSelectedTextOnInput) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("abcd");
    text_field->set_bounds(0.0f, 0.0f, 240.0f, 40.0f);
    text_field->layout();

    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    const float start_x = text_field->padding().left() + label->font().estimate_text_width("a") + 1.0f;
    const float end_x = text_field->padding().left() + label->font().estimate_text_width("abc") + 1.0f;

    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = start_x,
            .y = 20.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::PointerMoveEvent{
        .x = end_x,
        .y = 20.0,
        .delta_x = end_x - start_x,
        .delta_y = 0.0,
    }));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = end_x,
            .y = 20.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerUp));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "X"}));

    EXPECT_EQ(text_field->value(), "aXd");
}

TEST(WidgetsTextFieldTest, BackspaceDeletesWholeUtf8Codepoint) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("你a");

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_LEFT, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{.key_code = SDLK_BACKSPACE, .is_repeat = false},
        nandina::runtime::EventType::KeyDown));

    EXPECT_EQ(text_field->value(), "a");
}

TEST(WidgetsTextFieldTest, ShiftArrowExtendsSelectionAndTypingReplacesIt) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("abcd");

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{
            .key_code = SDLK_LEFT,
            .modifiers = nandina::runtime::KeyModifiers::Shift,
            .is_repeat = false,
        },
        nandina::runtime::EventType::KeyDown));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{
            .key_code = SDLK_LEFT,
            .modifiers = nandina::runtime::KeyModifiers::Shift,
            .is_repeat = false,
        },
        nandina::runtime::EventType::KeyDown));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "X"}));

    EXPECT_EQ(text_field->value(), "abX");
}

TEST(WidgetsTextFieldTest, CtrlASelectsAllBeforeTypingReplacement) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("replace me");

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::KeyEvent{
            .key_code = SDLK_A,
            .modifiers = nandina::runtime::KeyModifiers::Ctrl,
            .is_repeat = false,
        },
        nandina::runtime::EventType::KeyDown));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "done"}));

    EXPECT_EQ(text_field->value(), "done");
}

TEST(WidgetsTextFieldTest, DoubleClickSelectsWordBeforeTypingReplacement) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("hello world");
    text_field->set_bounds(0.0f, 0.0f, 320.0f, 40.0f);
    text_field->layout();

    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    const float click_x = text_field->padding().left() + label->font().estimate_text_width("hello wo") + 1.0f;
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = click_x,
            .y = 20.0,
            .click_count = 2,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "Nandina"}));

    EXPECT_EQ(text_field->value(), "hello Nandina");
}

TEST(WidgetsTextFieldTest, DoubleClickAtCaretBoundaryDoesNotSelectAdjacentWord) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("hello");
    text_field->set_bounds(0.0f, 0.0f, 320.0f, 40.0f);
    text_field->layout();

    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    const float boundary_x = text_field->padding().left() + label->font().estimate_text_width("he");
    EXPECT_TRUE(text_field->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = boundary_x,
            .y = 20.0,
            .click_count = 2,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "X"}));

    EXPECT_EQ(text_field->value(), "heXllo");
}

TEST(WidgetsTextFieldTest, TextEditingShowsPreeditBeforeCommit) {
    auto text_field = nandina::widgets::TextField::create();
    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextEditingEvent{
        .text = "zhong",
        .start = 5,
        .length = 0,
    }));
    EXPECT_EQ(label->text(), "zhong");
    EXPECT_EQ(text_field->value(), "");

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextInputEvent{.text = "中"}));
    EXPECT_EQ(text_field->value(), "中");
    EXPECT_EQ(label->text(), "中");

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::TextEditingEvent{}));
    EXPECT_EQ(text_field->value(), "中");
}

TEST(WidgetsTextFieldTest, FocusedTextFieldExposesTextInputAreaWithCaretOffset) {
    auto text_field = nandina::widgets::TextField::create();
    text_field->set_value("hello");
    text_field->set_bounds(10.0f, 20.0f, 240.0f, 40.0f);
    text_field->layout();

    auto* label = find_internal_label(*text_field);
    ASSERT_NE(label, nullptr);

    EXPECT_TRUE(text_field->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));

    const auto area = text_field->text_input_area();
    ASSERT_TRUE(area.has_value());
    EXPECT_GE(area->rect.x(), text_field->x());
    EXPECT_GT(area->rect.width(), 0.0f);
    EXPECT_NEAR(
        static_cast<float>(area->cursor),
        label->font().estimate_text_width("hello"),
        2.0f);
}