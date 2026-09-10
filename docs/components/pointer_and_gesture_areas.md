# PointerArea 与 GestureArea

NandinaUI 将控件语义、原始指针输入和复合手势分为三个层次。内置控件只公开自身应当拥有的行为；任意控件需要额外交互时，通过组合 `PointerArea` 或 `GestureArea` 获得能力。

> 这两个组件目前属于实验性 API。核心职责已经确定，但命名和事件上下文仍可能在 alpha 阶段细化。

## 如何选择

| 需求 | 推荐接口 | 原因 |
| --- | --- | --- |
| 点击按钮执行保存 | `Button::set_on_click` | 保留鼠标、键盘与无障碍激活语义 |
| 读取按下、移动、释放坐标 | `PointerArea` | 直接观察低层指针生命周期 |
| Label 双击、长按或拖拽 | `GestureArea` | 统一处理时间、距离阈值与指针捕获 |
| 创建新的语义控件 | 组合 primitive 并定义专属事件 | 对应用公开业务语义，而非设备细节 |

## PointerArea

`PointerArea` 是透明的单子容器。它采用子控件的测量尺寸，并在输入捕获阶段观察事件，因此即使子控件消费了冒泡事件，Area 仍能收到输入。

```cpp
ui.make<widget::PointerArea>()
    .on_pointer_down([](const scene::MouseButtonEvent& event) {
      const auto position = event.screen_pos();
      // ...
    })
    .on_pointer_move([](const scene::MouseMoveEvent& event) {
      const auto delta = event.delta();
      // ...
    })
    .on_pointer_up([](const scene::MouseButtonEvent&) {})
    .child(ui.make<widget::Image>("res://preview.png"));
```

指针按下后 Area 会为其内容建立指针捕获，使移出边界后的 move 与 release 仍沿原路径分发。事件默认只被观察而不被消费，因此子控件和祖先仍可处理同一输入。

### 回调

| 构建器方法 | 事件 | 说明 |
| --- | --- | --- |
| `.on_pointer_down()` | `MouseButtonEvent` | 任意鼠标按钮按下 |
| `.on_pointer_up()` | `MouseButtonEvent` | 任意鼠标按钮释放 |
| `.on_pointer_move()` | `MouseMoveEvent` | 指针移动，包括捕获后的边界外移动 |
| `.on_pointer_enter()` | `MouseEnterEvent` | 指针进入区域 |
| `.on_pointer_leave()` | `MouseLeaveEvent` | 指针离开区域 |

## GestureArea

`GestureArea` 继承 `PointerArea`，并在其上识别 click、double-click、long-press 与 drag：

```cpp
ui.make<widget::GestureArea>()
    .on_double_click([](const scene::MouseButtonEvent&) {})
    .on_long_press([](const scene::MouseButtonEvent&) {})
    .on_drag_start([](const scene::MouseButtonEvent&) {})
    .on_drag_move([](const scene::MouseMoveEvent& event) {
      const auto delta = event.delta();
      // ...
    })
    .on_drag_end([](const scene::MouseButtonEvent&) {})
    .child(ui.make<widget::Label>("Interactive label"));
```

默认双击间隔为 300 ms，长按间隔为 500 ms，拖拽阈值与双击位置容差均为 5 个逻辑像素。可以通过 `set_double_click_interval()`、`set_long_press_interval()`、`set_drag_threshold()` 与 `set_double_click_distance()` 调整。

## 手势竞争规则

- 配置 double-click 后，第一次 click 会延迟到双击窗口结束；识别出双击时不会再触发两个单击。
- 指针移动超过拖拽阈值后进入 drag，当前 click 候选立即取消。
- long-press 只触发一次，并取消当前 click 候选。
- 第二次点击与第一次距离超过容差时，两次输入按独立 click 处理。
- GestureArea 目前识别鼠标主键；右键和中键仍可通过 PointerArea 原始回调观察。

## 组件设计原则

语义控件应公开用户意图。例如 Button 使用 `on_click`，Checkbox 使用 `on_change`，TextField 使用 `on_submit`。这些操作可以由不同输入设备或无障碍技术触发。

只有在业务确实依赖设备信息时才使用 PointerArea；需要可复用的时间或运动判定时使用 GestureArea。不要用原始 mouse-down/mouse-up 重新实现 Button，否则容易丢失键盘操作、取消规则、焦点与无障碍语义。

输入在场景树中先经历从根到目标的 capture 阶段，再从目标向根 bubbling。Area 在 capture 阶段观察输入；普通控件在 bubbling 阶段处理并可消费事件。这使附加行为与子组件的固有行为可以同时工作。
