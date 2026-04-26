#include <gtest/gtest.h>

#include <string>

import nandina.runtime.nan_event;

// ============================================================
// EventType 枚举测试（Issue 011）
// ============================================================

TEST(NanEvent, EventTypeToString) {
    using nandina::runtime::EventType;
    using nandina::runtime::to_string;

    EXPECT_EQ(to_string(EventType::PointerMove),   "PointerMove");
    EXPECT_EQ(to_string(EventType::PointerDown),   "PointerDown");
    EXPECT_EQ(to_string(EventType::PointerUp),     "PointerUp");
    EXPECT_EQ(to_string(EventType::PointerClick),  "PointerClick");
    EXPECT_EQ(to_string(EventType::PointerWheel),  "PointerWheel");
    EXPECT_EQ(to_string(EventType::KeyDown),       "KeyDown");
    EXPECT_EQ(to_string(EventType::KeyUp),         "KeyUp");
    EXPECT_EQ(to_string(EventType::TextInput),     "TextInput");
    EXPECT_EQ(to_string(EventType::FocusIn),       "FocusIn");
    EXPECT_EQ(to_string(EventType::FocusOut),      "FocusOut");
    EXPECT_EQ(to_string(EventType::WindowResize),  "WindowResize");
    EXPECT_EQ(to_string(EventType::WindowClose),   "WindowClose");
}

// ============================================================
// 事件数据结构体测试
// ============================================================

TEST(NanEvent, PointerMoveEventDefaults) {
    nandina::runtime::PointerMoveEvent ev;
    EXPECT_DOUBLE_EQ(ev.x, 0.0);
    EXPECT_DOUBLE_EQ(ev.y, 0.0);
    EXPECT_DOUBLE_EQ(ev.delta_x, 0.0);
    EXPECT_DOUBLE_EQ(ev.delta_y, 0.0);
}

TEST(NanEvent, PointerMoveEventCustom) {
    nandina::runtime::PointerMoveEvent ev{
        .x = 100.5,
        .y = 200.3,
        .delta_x = 1.5,
        .delta_y = -0.5
    };
    EXPECT_DOUBLE_EQ(ev.x, 100.5);
    EXPECT_DOUBLE_EQ(ev.y, 200.3);
    EXPECT_DOUBLE_EQ(ev.delta_x, 1.5);
    EXPECT_DOUBLE_EQ(ev.delta_y, -0.5);
}

TEST(NanEvent, PointerButtonEventDefaults) {
    using nandina::runtime::PointerButtonEvent;
    using nandina::types::PointerButton;

    PointerButtonEvent ev;
    EXPECT_EQ(ev.button, PointerButton::Unknown);
    EXPECT_DOUBLE_EQ(ev.x, 0.0);
    EXPECT_DOUBLE_EQ(ev.y, 0.0);
    EXPECT_FALSE(ev.is_repeat);
}

TEST(NanEvent, PointerButtonEventCustom) {
    using nandina::runtime::PointerButtonEvent;
    using nandina::types::PointerButton;

    PointerButtonEvent ev{
        .button = PointerButton::Left,
        .x = 50.0,
        .y = 75.0,
        .is_repeat = false
    };
    EXPECT_EQ(ev.button, PointerButton::Left);
    EXPECT_DOUBLE_EQ(ev.x, 50.0);
    EXPECT_DOUBLE_EQ(ev.y, 75.0);
}

TEST(NanEvent, KeyEventDefaults) {
    nandina::runtime::KeyEvent ev;
    EXPECT_EQ(ev.key_code, 0);
    EXPECT_FALSE(ev.is_repeat);
}

TEST(NanEvent, KeyEventCustom) {
    nandina::runtime::KeyEvent ev{
        .key_code = 32,  // space
        .is_repeat = true
    };
    EXPECT_EQ(ev.key_code, 32);
    EXPECT_TRUE(ev.is_repeat);
}

TEST(NanEvent, TextInputEvent) {
    nandina::runtime::TextInputEvent ev{
        .text = "hello 世界"
    };
    EXPECT_EQ(ev.text, "hello 世界");
}

TEST(NanEvent, FocusEventDefaults) {
    nandina::runtime::FocusEvent ev;
    EXPECT_FALSE(ev.got_focus);
}

TEST(NanEvent, FocusEventCustom) {
    nandina::runtime::FocusEvent ev{
        .got_focus = true
    };
    EXPECT_TRUE(ev.got_focus);
}

TEST(NanEvent, WindowResizeEventDefaults) {
    nandina::runtime::WindowResizeEvent ev;
    EXPECT_EQ(ev.width, 0);
    EXPECT_EQ(ev.height, 0);
}

TEST(NanEvent, WindowResizeEventCustom) {
    nandina::runtime::WindowResizeEvent ev{
        .width = 1920,
        .height = 1080
    };
    EXPECT_EQ(ev.width, 1920);
    EXPECT_EQ(ev.height, 1080);
}

// ============================================================
// Event 变体与 event_type() 测试
// ============================================================

TEST(NanEvent, EventVariantContainsTypeTag) {
    using nandina::runtime::Event;
    using nandina::runtime::EventType;
    using nandina::runtime::event_type;

    Event ev = nandina::runtime::PointerMoveEvent{};
    EXPECT_EQ(event_type(ev), EventType::PointerMove);

    ev = nandina::runtime::PointerButtonEvent{};
    EXPECT_EQ(event_type(ev), EventType::PointerDown);  // variant index 1

    ev = nandina::runtime::PointerWheelEvent{};
    EXPECT_EQ(event_type(ev), EventType::PointerWheel);

    ev = nandina::runtime::KeyEvent{};
    EXPECT_EQ(event_type(ev), EventType::KeyDown);

    ev = nandina::runtime::TextInputEvent{};
    EXPECT_EQ(event_type(ev), EventType::TextInput);

    ev = nandina::runtime::FocusEvent{};
    EXPECT_EQ(event_type(ev), EventType::FocusIn);

    ev = nandina::runtime::WindowResizeEvent{};
    EXPECT_EQ(event_type(ev), EventType::WindowResize);

    ev = nandina::runtime::WindowCloseEvent{};
    EXPECT_EQ(event_type(ev), EventType::WindowClose);
}

TEST(NanEvent, EventVariantRoundTrip) {
    using nandina::runtime::Event;
    using nandina::runtime::EventType;
    using nandina::runtime::event_type;
    using nandina::runtime::PointerMoveEvent;
    using nandina::types::PointerButton;

    // PointerUp
    Event ev = nandina::runtime::PointerButtonEvent{
        .button = PointerButton::Right,
        .x = 10.0,
        .y = 20.0,
        .is_repeat = false
    };
    EXPECT_EQ(event_type(ev), EventType::PointerDown);  // variant index 1

    // KeyDown with data
    ev = nandina::runtime::KeyEvent{
        .key_code = 65,  // 'A'
        .is_repeat = false
    };
    EXPECT_EQ(event_type(ev), EventType::KeyDown);
    const auto &key = std::get<nandina::runtime::KeyEvent>(ev);
    EXPECT_EQ(key.key_code, 65);

    // WindowClose (empty struct)
    ev = nandina::runtime::WindowCloseEvent{};
    EXPECT_EQ(event_type(ev), EventType::WindowClose);
}