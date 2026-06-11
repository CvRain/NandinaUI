# NandinaUI Runtime 模块设计文档

> **模块路径**：`nandina.runtime.*`  
> **命名空间**：`nandina::runtime`  
> **设计状态**：M1 已实现（NanWindow / NanWidget / NanEvent）
>
> 关联 Issue：
> - [Issue 009] 窗口创建与生命周期（NanWindow::Builder / run）
> - [Issue 010] 基础事件通路（SDL → NanWindow 事件翻译）
> - [Issue 011] 统一事件类型体系（NanEvent）
> - [Issue 015] Widget 层级与事件接口（NanWidget）
> - [Issue 017] 最小渲染闭环（ThorVG + SDL 渲染管线）

---

## 1. 模块概述

Runtime 层是 NandinaUI 框架的**核心运行时层**，位于 `foundation` 之上，`app / layout / widgets` 之下。它提供：

| 组件 | 职责 | 依赖 |
|------|------|------|
| **NanWindow** | 原生窗口创建、输入事件翻译、帧渲染管线 | SDL3 + ThorVG |
| **NanWidget** | UI 控件基类、空间属性、组合、事件分发与冒泡 | 仅标准库 |
| **NanEvent** | 统一事件类型枚举与数据结构 | **foundation::nan_types** |

### 模块依赖图

```
                     app (NanApplication / AppWindow)
                              │
                              ▼
                     ┌────────────────┐
                     │   nandina.runtime    │
                     │   ┌──────────┐  │
                     │   │ NanWindow │──│── SDL3 + ThorVG (实现隐藏)
                     │   ├──────────┤  │
                     │   │ NanWidget │──│── std::vector<unique_ptr<Ptr>>
                     │   ├──────────┤  │
                     │   │ NanEvent  │──│── foundation::nan_types (PointerButton)
                     │   └──────────┘  │
                     └────────────────┘
                              │
                              ▼
                     nandina.foundation.*
```

---

## 2. NanEvent — 统一事件类型体系（Issue 011）

### 2.1 设计目标

为整个框架提供**单一事件数据来源**，消除各层重复定义事件类型的问题。

### 2.2 核心设计决策

| 决策 | 选择 | 理由 |
|------|------|------|
| 事件载体 | `std::variant` | 避免继承体系的开销（虚表+堆分配），值语义，SSO 友好 |
| 类型标签 | `EventType` 独立枚举 | variant 的 index 不可保证与枚举值对齐（枚举有跳号） |
| 事件命名 | 独立的 `*Event` 结构体 | 命名明确，与 `EventType` 枚举值一一对应 |
| 数据复用 | `PointerButtonEvent` 复用（Down/Up/Click 共用） | 三种事件数据结构完全一致，避免冗余类型 |

### 2.3 事件类型清单

```cpp
enum class EventType : std::uint8_t {
    PointerMove,   // 鼠标移动
    PointerDown,   // 鼠标按下
    PointerUp,     // 鼠标抬起
    PointerClick,  // 鼠标点击（Down+Up）
    PointerWheel,  // 滚轮滚动
    KeyDown,       // 键盘按下
    KeyUp,         // 键盘抬起
    TextInput,     // 文本输入（IME 最终结果）
    FocusIn,       // 获得焦点
    FocusOut,      // 失去焦点
    WindowResize,  // 窗口尺寸变化
    WindowClose,   // 窗口关闭
};

using Event = std::variant<
    PointerMoveEvent,
    PointerButtonEvent,   // 对应 Down / Up / Click
    PointerWheelEvent,
    KeyEvent,
    TextInputEvent,
    FocusEvent,
    WindowResizeEvent,
    WindowCloseEvent
>;
```

### 2.4 `event_type()` 映射

`Event` 变体的 `.index()` 返回的是线性索引（0~7），而 `EventType` 枚举值由 `std::uint8_t` 决定且包含跳号（如 `PointerClick = 3` 在枚举顺序中）。因此需要显式映射，不可直接转换：

```cpp
[[nodiscard]] inline auto event_type(const Event &ev) noexcept -> EventType {
    switch (ev.index()) {
    case 0:  return EventType::PointerMove;
    case 1:  return EventType::PointerDown;    // PointerButtonEvent
    case 2:  return EventType::PointerWheel;
    case 3:  return EventType::KeyDown;
    case 4:  return EventType::TextInput;
    case 5:  return EventType::FocusIn;
    case 6:  return EventType::WindowResize;
    case 7:  return EventType::WindowClose;
    default: return EventType::PointerMove;
    }
}
```

---

## 3. NanWindow — 原生窗口封装（Issue 009 / 010 / 017）

### 3.1 架构概览

NanWindow 采用 **PIMPL 惯用法**隔离平台相关类型：

```
┌─────────────────────────────────────────────────┐
│  NanWindow (接口层, nan_window.cppm)             │
│  - 可继承基类                                    │
│  - 头文件中零 SDL 类型暴露                       │
│  - 仅 ThorVG tvg::SwCanvas 因 on_draw 签名可见  │
│  - Builder 链式构造                              │
├─────────────────────────────────────────────────┤
│  NanWindow::Impl (实现层, nan_window.cpp)        │
│  - SDL_Window / SDL_Renderer / SDL_Texture      │
│  - tvg::SwCanvas + pixel_buffer (CPU 光栅化)     │
│  - rebuild_surface() 统一处理尺寸变化            │
└─────────────────────────────────────────────────┘
```

### 3.2 渲染管线

```
每一帧 (present_frame):
  1. canvas.remove(nullptr)         清空 ThorVG 绘制指令
  2. on_draw(canvas)                用户向 canvas 添加图元（ThorVG 矢量 API）
  3. canvas.draw(true) + sync()     CPU 端光栅化 → pixel_buffer (ARGB8888)
  4. SDL_UpdateTexture              pixel_buffer → SDL 流式纹理（CPU→GPU 上传）
  5. SDL_RenderClear + RenderTexture + RenderPresent  屏幕呈现
```

**关键约束**：
- ThorVG `SwCanvas::target()` 要求 stride（步长）以**像素**为单位，此处等于 `width`
- pixel_buffer 在 resize 时通过 `rebuild_surface()` 统一重建：纹理 → 缓冲区 → 画布重绑
- `canvas.draw(true)` 中 `true` 表示 clear，每帧刷新背景

### 3.3 生命周期

```
NanWindow::Builder::build()
    → SDL_CreateWindowAndRenderer
    → tvg::SwCanvas::gen() + target() 绑定
    → SDL_ShowWindow

NanWindow::run()
    → [首次] on_ready()
    → 循环: poll_events() → on_update(dt) → present_frame()
    → 循环结束 → 析构

~NanWindow()
    → canvas.reset() 先释放（解除对 pixel_buffer 的引用）
    → SDL_Quit / ThorVG term（引用计数管理）
    → unique_ptr 按逆序：texture → renderer → window
```

### 3.4 运行时引导（RuntimeBootstrap）

使用静态引用计数实现**进程内单例**的 SDL_Init / SDL_Quit + ThorVG init / term：

```cpp
struct RuntimeBootstrap {
    std::mutex mutex;
    int ref_count{0};

    auto acquire() -> void;   // 首次初始化 SDL + ThorVG
    auto release() -> void;   // 末次释放资源
};
```

**目的**：支持同一个进程中多个 NanWindow 实例安全共存、独立析构。

### 3.5 事件翻译（SDL → NanEvent）

`poll_events()` 将 SDL 原生事件翻译为框架统一的 `Event` 类型，并调用对应的 `on_xxx()` 虚方法：

| SDL 事件 | 翻译为 | 对应虚方法 |
|----------|--------|-----------|
| `SDL_EVENT_QUIT` | — | `on_close_requested()` |
| `SDL_EVENT_WINDOW_CLOSE_REQUESTED` | — | `on_close_requested()` |
| `SDL_EVENT_WINDOW_RESIZED` | `WindowResizeEvent` → 传给 widget tree | `on_resize(w, h)` |
| `SDL_EVENT_MOUSE_MOTION` | `PointerMoveEvent` | `on_pointer_move()` |
| `SDL_EVENT_MOUSE_BUTTON_DOWN` | `PointerButtonEvent` | `on_pointer_down()` |
| `SDL_EVENT_MOUSE_BUTTON_UP` | `PointerButtonEvent` | `on_pointer_up()` |
| `SDL_EVENT_MOUSE_WHEEL` | `PointerWheelEvent` | `on_pointer_wheel()` |
| `SDL_EVENT_KEY_DOWN` | `KeyEvent` | `on_key_down()` |
| `SDL_EVENT_KEY_UP` | `KeyEvent` | `on_key_up()` |
| `SDL_EVENT_TEXT_INPUT` | `std::string_view` | `on_text_input()` |

**设计约束**：
- **所有 SDL 类型完全隐藏在实现单元** `nan_window.cpp` 中，接口层 `nan_window.cppm` 零 SDL 引用
- ThorVG 类型仅因 `on_draw(tvg::SwCanvas&)` 签名而在接口层可见

### 3.6 Builder 模式

```cpp
class NanWindow::Builder {
    auto set_title(string_view)  -> Builder&;
    auto set_width(int)          -> Builder&;
    auto set_height(int)         -> Builder&;
    auto set_size(int w, int h)  -> Builder&;
    auto set_resizable(bool)     -> Builder&;
    auto set_high_dpi(bool)      -> Builder&;
    auto to_config() const       -> Config;
    auto build()                 -> NanWindow;   // 消费 Builder，抛出异常
};
```

**为什么用 Builder 而非直接参数列表**：
1. 窗口配置参数太多（5+ 个），直接传参可读性差
2. 链式调用允许只设置需要的参数，其他保持默认
3. 未来扩展新参数（如 vsync、min_size、icon）无需改动构造函数签名
4. `to_config()` 允许继承类在构造函数中传递 `Config` 而非暴露 builder 的私有字段

---

## 4. NanWidget — UI 控件基类（Issue 015）

### 4.1 设计目标

- 提供轻量级的 UI 控件基类，支持**空间属性、组合能力、统一绘制、事件分发**
- 作为 `widgets/` 目录下所有具体控件的基类
- 使用值语义和 unique_ptr 管理生命周期

### 4.2 空间属性

```cpp
class NanWidget {
    float m_x{0.0f};
    float m_y{0.0f};
    float m_width{0.0f};
    float m_height{0.0f};
    std::vector<Ptr> m_children;   // Ptr = std::unique_ptr<NanWidget>
};
```

- 坐标使用 `float` 以支持布局系统的子像素精度
- `set_position(x, y)` / `set_size(w, h)` 设置空间属性
- 当前为平铺存储，后续 layout 模块将实现自动布局计算

### 4.3 组合模型

```
NanWidget
  ├── add_child(unique_ptr<NanWidget>) → raw pointer
  ├── clear_children()
  └── m_children: vector<unique_ptr<NanWidget>>
     ├── child A
     │     └── grandchild A1
     ├── child B
     └── child C
```

- 使用 `std::unique_ptr` 树形所有权，Widget 析构时自动释放所有子树
- `add_child()` 返回裸指针便于后续操作，但不转移所有权
- 不支持移除单个子节点（M1 阶段简化），`clear_children()` 批量移除

### 4.4 绘制树遍历

```cpp
virtual void draw(tvg::SwCanvas &canvas) {
    on_draw(canvas);          // 子类自定义绘制
    for (const auto &child : m_children) {
        child->draw(canvas);  // 递归子节点
    }
}
```

- 先绘制自身，再遍历子节点（依赖绘制顺序，背景→前景）
- `on_draw()` 是 protected 虚方法，子类在 ThorVG canvas 上添加图元
- 默认实现为空操作

### 4.5 事件分发（dispatch_event）

```cpp
virtual auto dispatch_event(const Event &ev) -> bool {
    switch (event_type(ev)) {
    case EventType::PointerMove:
        return on_pointer_move(std::get<PointerMoveEvent>(ev));
    case EventType::PointerDown:
        return on_pointer_down(std::get<PointerButtonEvent>(ev));
    // ... 12 种事件类型
    }
    return false;  // 未消费
}
```

- 使用 `std::get<T>` 从 `Event` 变体中安全提取具体事件类型
- 返回 `true` 表示事件已被消费，停止冒泡
- 12 个 `on_xxx()` 虚方法默认返回 `false`，子类按需覆盖

### 4.6 事件冒泡（bubble_event）

```cpp
auto bubble_event(const Event &ev) -> bool {
    for (auto &child : m_children) {
        if (child->dispatch_event(ev))
            return true;
    }
    return false;
}
```

- **自叶向根冒泡**：先遍历子节点，某个子消费即停止
- 当前实现为遍历所有子节点，后续可按需改为命中测试优先（如点击事件按 Z-order 排序）
- 返回 `true` 表示事件被某个子节点消费

### 4.7 可覆盖的事件处理虚方法

| 方法 | 参数类型 | 说明 |
|------|---------|------|
| `on_pointer_move` | `const PointerMoveEvent&` | 鼠标移动，含 x/y/delta_x/delta_y |
| `on_pointer_down` | `const PointerButtonEvent&` | 鼠标按下，含 button/x/y/is_repeat |
| `on_pointer_up` | `const PointerButtonEvent&` | 鼠标抬起 |
| `on_pointer_click` | `const PointerButtonEvent&` | 鼠标点击（Down+Up 完整动作） |
| `on_pointer_wheel` | `const PointerWheelEvent&` | 滚轮滚动，含 x/y |
| `on_key_down` | `const KeyEvent&` | 键盘按下，含 key_code/is_repeat |
| `on_key_up` | `const KeyEvent&` | 键盘抬起 |
| `on_text_input` | `const TextInputEvent&` | IME 文本输入 |
| `on_focus_in` | （无参数） | 获得焦点 |
| `on_focus_out` | （无参数） | 失去焦点 |
| `on_window_resize` | `const WindowResizeEvent&` | 窗口尺寸变化，含 width/height |
| `on_window_close` | （无参数） | 窗口关闭 |

---

## 5. 各组件间的协作流程

### 5.1 事件从平台到 Widget 的完整路径

```
SDL 事件队列
    │
    ▼
NanWindow::poll_events()           SDL → Event 翻译
    │
    ▼
NanWindow::on_pointer_down(event)  NanWindow 层虚方法
    │
    ▼  [用户重写，分发给 widget tree]
    │
widget.children[target_idx]        hit test → 找到目标
    └─ dispatch_event(event)       Widget 层事件分发
         └─ on_pointer_down()      具体 Widget 的处理逻辑
              └─ return true/false 是否消费
```

### 5.2 渲染流程

```
NanWindow::present_frame()
    │
    ├── canvas.remove(nullptr)        清空 ThorVG 绘制指令
    ├── on_draw(canvas)               用户绘制（遍历 widget tree）
    │      └── widget.draw(canvas)
    │           └── on_draw(canvas)
    │                └── child.draw(canvas)    递归绘制
    ├── canvas.draw(true) + sync()    CPU 光栅化
    ├── SDL_UpdateTexture()           CPU→GPU 上传
    └── SDL_RenderPresent()           屏幕呈现
```

---

## 6. 模块导出关系

```cpp
// nandina.runtime.nan_event
export import nandina.foundation.nan_types;     // PointerButton
export namespace nandina::runtime {
    enum class EventType : std::uint8_t { ... };
    struct PointerMoveEvent { ... };
    struct PointerButtonEvent { ... };
    // ... 其他事件数据结构体
    using Event = std::variant<...>;
    auto event_type(const Event &) -> EventType;
}

// nandina.runtime.nan_widget
export import nandina.runtime.nan_event;
export namespace nandina::runtime {
    class NanWidget { ... };
}

// nandina.runtime.nan_window
export import nandina.runtime.nan_event;
export namespace nandina::runtime {
    class NanWindow { ... };
}
```

---

## 7. TODO（M2+ 规划）

| 功能 | 说明 | 优先级 |
|------|------|--------|
| HitTest 命中测试 | 根据坐标找到对应 Widget（nan_widget + 事件冒泡增强） | P0 |
| Focus 焦点管理 | 键盘事件的目标 Widget 切换 / Tab 导航 | P0 |
| Event 事件队列 | 防止在 poll_events 外嵌套调用（当前直接分发性能优但非线程安全） | P1 |
| Clipboard 剪贴板 | SDL 剪贴板 API 封装 | P1 |
| Cursor 光标 | 根据 MouseCursor 枚举切换 SDL 光标 | P1 |
| DPI 动态变更 | resize 时自动重算 DPI / 字号 | P2 |
| Widget 移除单个子 | remove_child(raw_ptr) 支持 | P2 |
| 多窗口 | 同一进程中多个 NanWindow 独立运行 | P2 |

---

## 8. 与相关框架的对比

| 维度 | NandinaUI | Flutter | Qt Quick | Godot UI |
|------|-----------|---------|----------|----------|
| 窗口基类 | `NanWindow` | `Window` | `QQuickWindow` | `Window` |
| 控件基类 | `NanWidget` | `Widget` | `QQuickItem` | `Control` |
| 事件类型 | `Event` (variant) | `PointerEvent` (class) | `QMouseEvent` (class) | `InputEvent` (class) |
| 渲染后端 | ThorVG (CPU) / SDL | Skia (GPU) | RHI (GPU) | Vulkan/Metal (GPU) |
| 事件分发 | dispatch + bubble | HitTest + dispatch | 信号/槽 | Group + 信号 |
| 布局系统 | 待实现 | RenderObject | Qml Row/Column | Container |

---

> **设计文档版本**：v1.0  
> **更新日期**：2026-04-26  
> **维护者**：CvRain