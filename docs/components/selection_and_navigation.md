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

## 移动模型：两个独立的决定

方向命中后，容器要回答两个彼此独立的问题：

- **控件焦点是否移动到目标成员？** —— `Intent::move_widget_focus`（决定是否 `set_focus()`）；
- **选中是否跟随焦点？** —— `Intent::selection_follows_focus`（决定是否把选中值改成目标索引）。

`RovingMovement` 不再把两者捆进一个偏 radiogroup 视角的枚举值，而是显式命名三种组合：

| 模式 | 控件焦点移动 | 选中跟随 | 用它的组件 |
| --- | --- | --- | --- |
| `focus_and_selection` | ✅ | ✅ | `RadioGroup`（焦点环与选中值一起走） |
| `focus_only` | ✅ | ❌ | `ToggleGroup`（方向键只漫游，值由 `Enter` / `Space` 决定） |
| `selection_only` | ❌ | ✅ | `Tabs`、`Select` 弹出列表（焦点留在容器上） |

这不是实现细节上的差异，而是通用的可访问性模式：radiogroup 两者都动；tablist 与 combobox
只改选中值、焦点不离开容器；工具栏式的 toggle 组只移动焦点。容器直接读两个布尔字段分别落地
即可，不必再为了「只移动焦点」而在自己的代码里绕开某个模式。

## 程序化步进：`step()`

`handle_key()` 是真实输入路径，按 `RovingOrientation` 过滤键码；与键码无关的「沿当前轴走一步」
由 `RovingFocus::step()` 提供：

```cpp
[[nodiscard]] auto step(int delta) -> std::optional<Intent>;
```

- `delta < 0` 上一个，`delta > 0` 下一个，`delta == 0` 视为「不动」返回 `nullopt`；
- 沿用同一套 `set_loop(...)` 环绕与跳过 `accepts_focus == false` 成员的规则；
- 无处可去时返回 `nullopt`，且不改变 `active_index()`；
- 命中时更新 `active_index()`、结束当前 typeahead 查找，并返回与 `handle_key()` 同形的
  `Intent`，容器按上面两个决定落地即可。

因此 `RadioGroup::move_focus()` 与 `ToggleGroup::move_focus()` 直接调用 `step()`，不再按
orientation 合成上下 / 左右键码。`ToggleGroup` 的真实按键路径仍走 `handle_key()`，方向键的
orientation 过滤对实际输入保持权威。

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
| `RadioGroup` | ✅ | ❌ | ❌ | 走 `focus_and_selection` 模式；`move_focus()` 是薄封装，内部直接调用 `step()` |
| `Tabs` | ✅ | ✅ | ✅ | 走 `selection_only` 模式，保持「焦点不离开标签条」的既有行为（有意未对齐 ARIA tablist 的焦点跟随） |
| `Select` 弹出列表 | ✅ | ✅ | ✅ | 走 `selection_only` 模式 |
| `ToggleGroup` | ✅ | ✅ | ✅ | 走 `focus_only` 模式；方向键只移动焦点，不改变任何成员的 `checked` |
| `Slider` | ✅ | ✅ | — | 数值调节，不走本模型（无「成员」概念） |
| `Chip` 可移除 | — | — | — | 只处理 `Enter` / `Space` / `Backspace` / `Delete` |

## 已知空白

1. **非节点协调对象的 `dt` 归属**：`RadioGroup` / `ToggleGroup` 不是场景节点，没有自己的
   `on_process(dt)`，typeahead 缓冲的超时目前由持焦点的成员代为转发（见
   `docs/components/toggle_group.md`）。`RovingFocus::advance_time(dt)` 只接受调用方提供的
   时钟，但「谁该在每帧调用它」尚无统一约定。
2. **RTL 方向键极性**：`horizontal` 在 RTL 下应反转左右键。项目已接入 FriBidi，但既有实现
   都未处理，本轮**有意不做**，避免把行为变更混进重构。需要时按 `NanTextDirection`
   传入极性，单独立项。
3. **`RadioGroup` 尚未接入 Home/End 与 typeahead**：漫游设施已具备能力，`RadioButton`
   `on_input` 目前只把方向键转成 `move_focus()`。接入需要给组提供键盘入口，属于增量能力，
   不阻塞任何组件。

## 相关代码

- `nandina/widget/roving_focus.hpp` / `.cpp` —— 漫游与 typeahead 设施；
- `nandina/widget/key_codes.hpp` —— 全项目**唯一**的键码常量定义处（此前 7 个文件各抄一份）；
- `tests/roving_focus_tests.cpp` —— 漫游 / typeahead 契约与边界；
- `nandina/widget/radio_group.hpp`、`tabs.cpp`、`select.cpp`、`toggle_group.cpp` —— 接入方。
