# 选择与导航的键盘模型

本文记录 NandinaUI 中「一组同类条目、同一时刻只有一个当前项」的键盘交互模型。适用组件：
`RadioGroup`、`Tabs`、`Select` 的弹出列表、`ToggleGroup`，以及后续的 `DropdownMenu`、
`Combobox`、`CommandPalette`（阶段 4）。

## 为什么需要共享模型

这类组件此前各写一套键盘处理，而且语义不一致：`RadioGroup` 移动控件焦点，`Tabs` 与
`Select` 只改选中值。三套实现各自维护键码、环绕、边界判断，`Tabs` 和 `Select` 也因此
一直没有 Home/End 与 typeahead。现在它们共用 `widget::RovingFocus`。

`RovingFocus` 的职责边界：

- **只回答「下一个是谁」** —— 不做布局、绘制、事件派发，也不持有控件；
- 成员身份用**索引**表示（注册顺序 = 视觉顺序 = 方向键顺序），成员增删不会留下悬垂指针；
- 按键命中时返回 `Intent`，**由容器决定怎么落地**，因为「成员是什么」（裸索引还是控件指针）
  只有容器知道。

## 两种移动模式

`RovingMovement` 区分了此前被混为一谈的两种语义：

| 模式 | 方向键做什么 | 用它的组件 |
| --- | --- | --- |
| `widget_focus` | 同时移动**控件焦点**与选中（焦点环跟着走） | `RadioGroup`、`ToggleGroup`（只落地焦点） |
| `selection_only` | 只改**选中值**，焦点留在组容器上 | `Tabs`、`Select` 弹出列表 |

容器按 `Intent::move_widget_focus` 决定是否需要 `set_focus()`。这不是实现细节上的差异，
而是两种通用的可访问性模式：radiogroup 用前者，tablist 与 combobox 用后者。
`ToggleGroup` 也走 `widget_focus`（需要 `move_widget_focus`），但只落地焦点、不改选中值 ——
toggle 的值语义属于成员自己的显式激活（`Enter` / `Space` / 点击）。

## 键位

| 键 | 行为 |
| --- | --- |
| 方向键 | 在成员间移动；方向由 `RovingOrientation` 过滤（`vertical` 忽略左右，`horizontal` 忽略上下，`both` 全收） |
| `Home` | 跳到首个可聚焦成员 |
| `End` | 跳到最后一个可聚焦成员 |
| `PageUp` / `PageDown` | 当前无「可见项高度」信息，语义等价于 `Home` / `End` |
| 字母数字 | typeahead（见下） |
| `Enter` / `Space` / `Escape` / `Tab` | **不由本设施处理**，返回「不是我的键」，仍归组件自己 |

到边界后默认**环绕**（`set_loop(false)` 可关闭）。

## typeahead

- 缓冲**累积前缀**：按 `a` 再到 `v`，匹配以 `av` 开头的成员；
- 查找从**当前项之后**开始，因此重复按同一字母会在同首字母成员间轮转；
- 若累积后的缓冲不再是任何成员的前缀（例如先按 `b` 再按 `a`），自动**回退为单字符**重新查找
  —— 否则一次误按会让 typeahead 永久失效；
- 缓冲超过 `typeahead_timeout`（默认 0.5 秒）后清空；容器需在 `on_process(dt)` 里调用
  `advance_time(dt)`，时钟由调用方提供，便于测试推进虚拟时间；
- 文本来源是 `scene::TextInputEvent`，与按键分流。

typeahead 文本由容器通过 `sync()` 的 `label` 回调提供，通常是成员显示文本。

## 各组件当前状态

| 组件 | 方向键 | Home/End | typeahead | 备注 |
| --- | --- | --- | --- | --- |
| `RadioGroup` | ✅ | ❌ | ❌ | 走 `widget_focus` 模式；`move_focus()` 保留为兼容入口 |
| `Tabs` | ✅ | ✅ | ✅ | 走 `selection_only` 模式，保持「焦点不离开标签条」的既有行为 |
| `Select` 弹出列表 | ✅ | ✅ | ✅ | 走 `selection_only` 模式 |
| `ToggleGroup` | ✅ | ✅ | ✅ | 走 `widget_focus` 模式；方向键只移动焦点，不改变任何成员的 `checked` |
| `Slider` | ✅ | ✅ | — | 数值调节，不走本模型（无「成员」概念） |
| `Chip` 可移除 | — | — | — | 只处理 `Enter` / `Space` / `Backspace` / `Delete` |

## 已知空白

1. **RTL 方向键极性**：`horizontal` 在 RTL 下应反转左右键。项目已接入 FriBidi，但三处既有
   实现都未处理，本轮**有意不做**，避免把行为变更混进重构。需要时按 `NanTextDirection`
   传入极性，单独立项。
2. **`Tabs` 未对齐 ARIA tablist 的「焦点跟随」模式**：目前是 `selection_only`，方向键不移动
   控件焦点。改成 `widget_focus` 属于行为变更，需要额外的语义树与焦点环验证，单独处理。
3. **`RadioGroup` 尚未接入 Home/End 与 typeahead**：漫游设施已具备能力，`RadioButton`
   `on_input` 目前只把方向键转成 `move_focus()`。接入需要给组提供键盘入口，属于增量能力，
   不阻塞任何组件。
4. **`PageUp` / `PageDown` 未按「可见项数」翻页**：缺少可见项高度信息，当前等价于跳首尾。
5. **`RovingMovement` 把「移动焦点」和「选中跟随」绑在同一个枚举值上**：`widget_focus` 的
   文档语义是两者都动，但 `ToggleGroup` 只需要 `Intent::move_widget_focus`、落地时并不改选中。
   当前靠「容器自己决定怎么落地 Intent」绕过，枚举名与注释仍偏 radiogroup 视角；若要彻底
   正交化，应把「焦点是否跟随」与「选中是否跟随」拆成两个开关。

## 相关代码

- `nandina/widget/roving_focus.hpp` / `.cpp` —— 漫游与 typeahead 设施；
- `nandina/widget/key_codes.hpp` —— 全项目**唯一**的键码常量定义处（此前 7 个文件各抄一份）；
- `tests/roving_focus_tests.cpp` —— 漫游 / typeahead 契约与边界；
- `nandina/widget/radio_group.hpp`、`tabs.cpp`、`select.cpp`、`toggle_group.cpp` —— 接入方。
