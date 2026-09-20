# ToggleGroup

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。不要复述 API，说"人话"。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景（该用别的组件的情况）。反向场景最有价值，
     它能防止误用。 -->

| 场景 | 建议 |
| --- | --- |
| <!-- TODO --> | 本组件 |
| <!-- TODO --> | 改用 `RadioButton` / `RadioGroup` |
| <!-- TODO --> | 改用 `Tabs` |
| <!-- TODO --> | 改用 `Select` |

## 快速开始

<!-- TODO: 最小可运行示例。优先用 builder（ui.make<Toggle>(text, group)），
     而不是裸构造。示例要短、可编译、不依赖示例之外的上下文。 -->

```cpp
// TODO
```

## 外观与变体

<!-- TODO: 组本身只有容器造型与成员间距；成员外观由各个 Toggle 自己的 tone /
     treatment 决定。说明"组什么时候需要可见容器"。 -->

| 变体 | 默认值 | 用途 |
| --- | --- | --- |
| `set_mode(ToggleGroupMode)` | `ToggleGroupMode::single` | <!-- TODO --> |
| `set_allow_empty(bool)` | `true` | <!-- TODO --> |
| `set_orientation(RovingOrientation)` | `RovingOrientation::vertical` | <!-- TODO -->（委托给 `RovingFocus`） |

| 模式 | 语义 |
| --- | --- |
| `ToggleGroupMode::single` | 至多一个成员选中；选中新成员会取消旧成员 |
| `ToggleGroupMode::multiple` | 成员各自独立开关，组只报告选中集合 |

| 模式 + `allow_empty` | 行为 |
| --- | --- |
| `single` + `true`（默认） | 再点一次已选中的成员会取消它 |
| `single` + `false` | 忽略"取消最后一个选中项"的请求，组保持恰好一个选中 |
| `multiple` + 任意值 | `allow_empty` 不参与 |

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `create()` | <!-- TODO --> |
| `register_toggle(Toggle*)` / `unregister_toggle(Toggle*)` | <!-- TODO --> |
| `member_count()` / `index_of(const Toggle*)` | <!-- TODO --> |
| `set_mode(ToggleGroupMode)` / `mode()` | <!-- TODO --> |
| `set_allow_empty(bool)` / `allow_empty()` | <!-- TODO --> |
| `checked_indices()` | <!-- TODO --> |
| `select(int index)` | <!-- TODO --> |
| `toggle_member(Toggle*)` | <!-- TODO -->：成员激活入口，由 `Toggle::toggle()` 调用 |
| `selection_changed()` | <!-- TODO --> |
| `handle_key(Toggle*, const scene::KeyEvent&)` | <!-- TODO --> |
| `move_focus(Toggle*, int direction)` | <!-- TODO --> |
| `handle_text(Toggle*, const scene::TextInputEvent&)` | <!-- TODO --> |
| `advance_time(float dt)` | <!-- TODO --> |
| `typeahead_buffer()` | <!-- TODO --> |
| `set_orientation(RovingOrientation)` / `orientation()` | <!-- TODO --> |
| `set_override(theme::ToggleGroupRecipeRule)` | <!-- TODO --> |
| `set_theme(theme::NanTheme)` / `theme_ref()` | <!-- TODO --> |
| `visual_state()` / `resolved_style()` | <!-- TODO --> |

## 槽位与组合

<!-- TODO: 说明成员如何加入（构造时传入组 / `set_group`）、成员的场景树归属由谁负责，
     以及组与布局容器（`Row` / `Column`）的关系。 -->

无槽位（`ToggleGroup` 不是场景节点，成员由应用放进布局容器）。

## 事件与绑定

<!-- TODO: callback、reactive Event 各怎么用。`Toggle` 的 `on_change` 与组的
     `selection_changed` 各自报告什么。 -->

| 事件 | 载荷 | 触发时机 |
| --- | --- | --- |
| `Toggle::checked_changed()` | `bool` | 该成员自己被用户切换时（组内被连带取消的成员不发） |
| `ToggleGroup::selection_changed()` | `std::vector<int>`（变化后的选中索引集合） | 用户切换成员、`select()`、或 `set_mode()` 收敛选中集合时 |

```cpp
// TODO
```

## 键盘与指针行为

<!-- TODO: 对照 `Toggle::on_input()`、`ToggleGroup::handle_key()` 与
     `RovingFocus` 的实际实现说明。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | <!-- TODO -->：漫游进入的是成员，组本身不占 Tab 停靠点 |
| Enter / Space | <!-- TODO -->：由当前聚焦的成员自己处理（切换它的 `checked`） |
| Escape | <!-- TODO --> |
| 方向键 | <!-- TODO -->：交给共享的 `RovingFocus`，只移动焦点、不改选中值；按 orientation 过滤 |
| Home / End / PageUp / PageDown | <!-- TODO -->：由 `RovingFocus` 回答（首 / 末成员） |
| 可打印字符（typeahead） | <!-- TODO -->：按成员文本前缀移动焦点 |
| 指针 | <!-- TODO -->：点成员 = 该成员自己的指针路径 |
| 成员 disabled 时 | <!-- TODO -->：漫游跳过该成员（`RovingFocus::sync` 的可聚焦性回调） |

## 无障碍语义

<!-- TODO: 说明组不是节点、不进入语义树；每个成员暴露 checkbox 语义；
     以及"组的模式"目前无法被辅助技术读出这一事实。 -->

| 语义字段 | 值 |
| --- | --- |
| 组自身 | 不进入语义树（`ToggleGroup` 不是场景节点） |
| 成员的 `role` | `semantics::Role::checkbox` |
| 成员的 `label` | 该成员的 `text()` |
| 成员的 `state.checked` | 该成员的 `checked()` |
| 成员的 `actions` | 启用时 `activate \| focus`；禁用时 `none` |

## 主题与覆盖

<!-- TODO: 组不是节点，容器造型只作为宿主/主题作者的读取来源；说明 `resolved_style()`
     怎么用，以及成员间距（`metrics.gap`）与应用自带布局容器之间的关系。 -->

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container.fill` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO -->：透明默认 = 无分组底色 |
| `container.border` | `ThemeColor::transparent(ColorToken::primary)` | <!-- TODO --> |
| `container.border_width` | `0` | <!-- TODO --> |
| `container.radius` | `ScalarToken::radius_md`（8px） | <!-- TODO --> |
| `metrics.gap` | `ScalarToken::spacing_xs`（4px） | <!-- TODO -->：成员间距 |
| `metrics.padding_x` | `0` | <!-- TODO -->：组内边距 |
| `metrics.min_height` | `0` | <!-- TODO --> |

实例级覆盖示例：

```cpp
// TODO: set_override(theme::ToggleGroupRecipeRule {...})
```

## 常见坑

<!-- TODO: 至少写 2 条真实踩过的坑。凭空想的坑不如不写。 -->

- <!-- TODO -->

## 相关组件

<!-- TODO: 3~5 个链接，说明各自解决什么，帮助读者横向选择。 -->

- [`Toggle`](toggle.md) —— <!-- TODO -->
- [`Button`](button.md) —— <!-- TODO -->
- `RadioButton` / `RadioGroup` —— <!-- TODO -->（文档待补）
- [`selection_and_navigation`](selection_and_navigation.md) —— <!-- TODO -->（漫游键盘模型）

## 已知空白

<!-- TODO: 如实列出。没有就写"暂无"。 -->

- <!-- TODO -->：组不是场景节点，因此 `BuildContext::make<ToggleGroup>()` 不可用（走
  `ComponentTraits<ToggleGroup>::make(ui)` 或 `ToggleGroup::create()`），容器造型也不会由组自绘。
- <!-- TODO -->：组没有自己的 `dt` 来源，typeahead 缓冲的超时由持焦点的成员在
  `on_process` 里转发（`advance_time`）。
- <!-- TODO -->：RTL 下方向键极性未反转（与 `RovingFocus` 的已知空白一致）。
