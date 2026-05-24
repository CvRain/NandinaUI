# Event System Design (Issue 011)

> 状态校正（2026-05）：事件类型模块与 Widget 分发接口已经落地，但本文的部分早期描述仍带有“只靠 `std::variant` 索引恢复完整事件语义”的假设。当前实现应以 `runtime/src/nan_event.cppm` 与 `runtime/src/nan_widget.cppm` 为准。

## 概述

NandinaUI 统一事件类型体系，定义框架内所有 UI 事件的数据结构与类型标签，供 Widget 事件分发与 Window 事件翻译共用。

## 设计原则

1. **类型安全**：所有事件类型通过 C++ 强类型结构体表达；当前主线同时保留 `EventType` 标签与 `Event` 变体容器。
2. **向后兼容**：NanWindow 保留原有的 `on_pointer_xxx`/`on_key_xxx` 签名，仅将事件结构体定义从 `nan_window.cppm` 迁移至 `nan_event.cppm`。
3. **静态分发**：Widget 的分发主路径通过显式 `EventType` 做静态分发，避免 `std::variant::visit` 的模板膨胀开销。

## 模块结构

```
runtime/src/nan_event.cppm        # 事件类型定义（C++20 Module）
runtime/src/nan_widget.cppm        # Widget 事件分发接口（导入 nan_event）
runtime/src/nan_window.cppm        # Window 事件翻译（导入 nan_event）
runtime/src/nan_window.cpp         # SDL → Event 翻译实现（导入 nan_event）
```

## 事件类型体系

### EventType 枚举

```cpp
enum class EventType : std::uint8_t {
    PointerMove,
    PointerDown,
    PointerUp,
    PointerClick,
    PointerWheel,
    KeyDown,
    KeyUp,
    TextInput,
    FocusIn,
    FocusOut,
    WindowResize,
    WindowClose,
};
```

### Event 数据结构体

| 结构体                | 字段                                | 对应 EventType      |
|-----------------------|-------------------------------------|---------------------|
| `PointerMoveEvent`    | x, y, delta_x, delta_y              | PointerMove         |
| `PointerButtonEvent`  | button, x, y, is_repeat             | PointerDown/Up/Click |
| `PointerWheelEvent`   | x, y                                | PointerWheel        |
| `KeyEvent`            | key_code, is_repeat                 | KeyDown/Up          |
| `TextInputEvent`      | text                                | TextInput           |
| `FocusEvent`          | got_focus                           | FocusIn/Out         |
| `WindowResizeEvent`   | width, height                       | WindowResize        |
| `WindowCloseEvent`    | （无数据）                          | WindowClose         |

### Event 统一变体

```cpp
using Event = std::variant<
    PointerMoveEvent,
    PointerButtonEvent,
    PointerWheelEvent,
    KeyEvent,
    TextInputEvent,
    FocusEvent,
    WindowResizeEvent,
    WindowCloseEvent
>;
```

### 类型标签提取

```cpp
auto event_type(const Event &ev) noexcept -> EventType;
```

当前实现只能从 `Event` 变体恢复“基础种类”；对于 `PointerButtonEvent`、`KeyEvent`、`FocusEvent` 这类复用同一 payload 结构的事件，不能仅靠 `variant::index()` 无损区分 `PointerDown/PointerUp/PointerClick`、`KeyDown/KeyUp`、`FocusIn/FocusOut`。

因此当前安全语义是：

- `Event` 适合统一传递、排队和日志场景
- 具体分发点仍应显式提供 `EventType`
- `NanWidget` 已提供 `dispatch_event(payload, EventType)` 这类重载用于 button/key 等共享 payload 的事件

## Widget 事件分发

`NanWidget` 提供两级事件分发 API：

### dispatch_event

接收 `Event` 变体，并在需要时结合显式 `EventType` 分派到对应的 `on_xxx()` 虚方法。

```cpp
virtual auto dispatch_event(const Event &ev) -> bool;
```

### bubble_event

将事件分发给所有子节点，直到某个子节点消费。

```cpp
auto bubble_event(const Event &ev) -> bool;
```

### 事件处理虚方法

每个事件类型对应一个 `on_` 虚方法，默认返回 `false`（未消费）。子类可重写：

```cpp
virtual auto on_pointer_move(const PointerMoveEvent &event) -> bool;
virtual auto on_pointer_down(const PointerButtonEvent &event) -> bool;
virtual auto on_pointer_up(const PointerButtonEvent &event) -> bool;
virtual auto on_pointer_click(const PointerButtonEvent &event) -> bool;
virtual auto on_pointer_wheel(const PointerWheelEvent &event) -> bool;
virtual auto on_key_down(const KeyEvent &event) -> bool;
virtual auto on_key_up(const KeyEvent &event) -> bool;
virtual auto on_text_input(const TextInputEvent &event) -> bool;
virtual auto on_focus_in() -> bool;
virtual auto on_focus_out() -> bool;
virtual auto on_window_resize(const WindowResizeEvent &event) -> bool;
virtual auto on_window_close() -> bool;
```

## 迁移路径

```
[v2.x] nan_window.cppm (自有事件定义) 
    → [v3.0] nan_window.cppm (导入 nan_event.cppm)
    → [v4.0] Widget 启用 dispatch_event/bubble_event
```

当前阶段（Issue 011）已完成以下主线能力：

- 事件定义集中到 `runtime/src/nan_event.cppm`
- `NanWindow` 已按 SDL 事件类型分别调用 `on_pointer_down()` / `on_pointer_up()` / `on_key_down()` 等接口
- `NanWidget` 已提供 `dispatch_event(payload, EventType)` 重载以保留 down/up 等语义

当前仍需注意的边界：如果后续要把 `Event` 作为统一事件队列长期使用，仍应补足独立的显式事件标签承载方式，避免把 `PointerButtonEvent` 等事件重新退化成“只能猜测具体类型”。