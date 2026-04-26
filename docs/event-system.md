# Event System Design (Issue 011)

## 概述

NandinaUI 统一事件类型体系，定义框架内所有 UI 事件的数据结构与类型标签，供 Widget 事件分发与 Window 事件翻译共用。

## 设计原则

1. **类型安全**：所有事件类型通过 C++ 强类型结构体表达，使用 `std::variant` 作为统一容器。
2. **向后兼容**：NanWindow 保留原有的 `on_pointer_xxx`/`on_key_xxx` 签名，仅将事件结构体定义从 `nan_window.cppm` 迁移至 `nan_event.cppm`。
3. **静态分发**：Widget 的 `dispatchEvent()` 通过 `EventType` 枚举做静态分发，避免 `std::variant::visit` 的模板膨胀开销。

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

通过 `variant::index()` 映射为 `EventType` 枚举值，零开销。

## Widget 事件分发

`NanWidget` 提供两级事件分发 API：

### dispatch_event

接收 `Event` 变体，通过 `EventType` switch 分派到对应的 `on_xxx()` 虚方法。

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

当前阶段（Issue 011）已完成迁移步骤 1→2，事件定义集中到 `nan_event.cppm`，`nan_window.cppm` 移除重复定义。Widget 的事件分发接口已就绪，后续 Widget 子类可通过重写 `on_xxx()` 方法响应事件。