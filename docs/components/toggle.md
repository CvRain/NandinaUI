# Toggle

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。不要复述 API，说"人话"。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景（该用别的组件的情况）。反向场景最有价值，
     它能防止误用。 -->

| 场景 | 建议 |
| --- | --- |
| <!-- TODO --> | 本组件 |
| <!-- TODO --> | 改用 `Switch` |
| <!-- TODO --> | 改用 `Checkbox` |
| <!-- TODO --> | 改用 `ToggleGroup` |

## 快速开始

<!-- TODO: 最小可运行示例。优先用 builder（ui.make<Toggle>(...)），而不是裸构造。
     示例要短、可编译、不依赖示例之外的上下文。 -->

```cpp
// TODO
```

## 外观与变体

<!-- TODO: 有哪些语义变体（tone / treatment）？各自的适用语义是什么？
     不要罗列所有组合，说清楚"什么时候用哪个"。
     选中态不是 treatment，而是 checked 解析入参：任何 tone / treatment 下 checked 都
     换成 tone 强调色实底 + on_accent 文本。 -->

| 变体 | 默认值 | 用途 |
| --- | --- | --- |
| `set_tone(theme::ButtonTone)` | `ButtonTone::primary` | <!-- TODO --> |
| `set_treatment(theme::ButtonTreatment)` | `ButtonTreatment::ghost` | <!-- TODO --> |
| `set_checked(bool)` | `false` | <!-- TODO --> |

| treatment | 容器填充 | 容器边框 | 文本 |
| --- | --- | --- | --- |
| `filled`（未选中） | `ColorToken::secondary` | 透明 | `ColorToken::secondary_foreground` |
| `tonal`（未选中） | `mix(ColorToken::secondary, accent, 0.30)` | 透明 | tone 强调色 |
| `outlined`（未选中） | 透明 | `ColorToken::border` + `border_thin` | `ColorToken::foreground` |
| `ghost`（未选中） | 透明 | 透明 | `ColorToken::muted_foreground` |
| `link`（未选中） | 透明 | 透明 | tone 强调色，`padding_x = 0` |
| 任意 treatment（`checked = true`） | tone 强调色 | tone 强调色 | `on_accent`，`padding_x = spacing_lg` |

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `Toggle(std::string text, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `Toggle(std::string text, std::shared_ptr<ToggleGroup> group, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `create(std::string text, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `create(std::string text, std::shared_ptr<ToggleGroup> group, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `set_text(std::string)` / `text()` | <!-- TODO --> |
| `set_checked(bool)` / `checked()` | <!-- TODO --> |
| `toggle()` | <!-- TODO --> |
| `set_tone(theme::ButtonTone)` / `tone()` | <!-- TODO --> |
| `set_treatment(theme::ButtonTreatment)` / `treatment()` | <!-- TODO --> |
| `set_disabled(bool)` / `disabled()` | <!-- TODO -->（继承自 `primitives::Pressable`） |
| `set_on_change(std::function<void(bool)>)` | <!-- TODO --> |
| `checked_changed()` | <!-- TODO --> |
| `set_group(std::shared_ptr<ToggleGroup>)` / `group()` | <!-- TODO --> |
| `set_override(theme::ToggleRecipeRule)` | <!-- TODO --> |
| `set_theme(theme::NanTheme)` / `theme_ref()` | <!-- TODO --> |
| `visual_state()` / `resolved_style()` | <!-- TODO --> |
| `set_text_pipeline(...)` / `text_pipeline()` | <!-- TODO --> |

## 槽位与组合

<!-- TODO: 无槽位就写"无槽位"。 -->

无槽位：`Toggle` 只有单个文本标签，没有图标或子内容槽位。

## 事件与绑定

<!-- TODO: callback、reactive Event、Signal 绑定各怎么用。
     若有 `ui.bind(...)` 路径，给出示例。 -->

```cpp
// TODO
```

## 键盘与指针行为

<!-- TODO: 对照 `Toggle::on_input()` 与 `primitives::Pressable::on_input()` 的
     实际实现说明。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | <!-- TODO -->（`is_focusable()` = `!disabled()`） |
| Enter / Space | <!-- TODO --> |
| Escape | <!-- TODO --> |
| 方向键（独立控件） | <!-- TODO --> |
| 方向键（组内成员） | <!-- TODO -->：转交 `ToggleGroup::handle_key()` |
| 指针 | <!-- TODO -->：按下不切换，原地抬起才切换 |
| 组内 typeahead | <!-- TODO -->：`ToggleGroup::handle_text()` |
| disabled 时 | <!-- TODO -->：不可聚焦、清空焦点、不接收输入、不发事件 |

## 无障碍语义

<!-- TODO: role / label / value / state / action 各是什么，以及调用方需要提供什么。 -->

| 语义字段 | 值 |
| --- | --- |
| `role` | `semantics::Role::checkbox`（`Role` 枚举没有 `toggle`；两态按钮按 checkbox 暴露） |
| `label` | `text()` |
| `state.checked` | `checked()` |
| `state.focusable` | `!disabled()` |
| `state.focused` | 是否持有焦点 |
| `state.disabled` | `disabled()` |
| `actions` | 启用时 `activate \| focus`；禁用时 `none` |

## 主题与覆盖

<!-- TODO: 该组件消费哪些语义角色？配方字段有哪些？字段若含**单位**语义必须写明。 -->

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container.fill` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `container.border` | `ThemeColor::transparent(ColorToken::primary)` | <!-- TODO --> |
| `container.border_width` | `0`（`outlined` 规则 → `border_thin`） | <!-- TODO --> |
| `container.radius` | `ScalarToken::radius_md`（8px） | <!-- TODO --> |
| `label.color` | `ColorToken::foreground`（`ghost` 规则 → `muted_foreground`） | <!-- TODO --> |
| `label.font_size` | `ScalarToken::typography_label_sm`（14px） | <!-- TODO --> |
| `focus.color` | `ColorToken::ring` | <!-- TODO --> |
| `focus.width` | `0`（`focused` 规则 → `border_focus_ring` = 2px） | <!-- TODO --> |
| `state_layer.hover` | 透明（`checked` 规则 → 强调色 / `on_accent` × `opacity_hover_overlay` = 0.10） | <!-- TODO --> |
| `state_layer.pressed` | 透明（`checked` 规则 → 强调色 / `on_accent` × `opacity_pressed_overlay` = 0.16） | <!-- TODO --> |
| `metrics.height` | 36px | <!-- TODO --> |
| `metrics.padding_x` | `ScalarToken::spacing_lg`（16px；`link` 规则 → 0） | <!-- TODO --> |
| `metrics.min_height` | 32px | <!-- TODO --> |
| `metrics.gap` | 0 | <!-- TODO -->：无第二个内容槽位，恒为 0 |

实例级覆盖示例：

```cpp
// TODO: set_override(theme::ToggleRecipeRule {...})
```

## 常见坑

<!-- TODO: 至少写 2 条真实踩过的坑。凭空想的坑不如不写。 -->

- <!-- TODO -->

## 相关组件

<!-- TODO: 3~5 个链接，说明各自解决什么，帮助读者横向选择。 -->

- [`Button`](button.md) —— <!-- TODO -->
- [`ToggleGroup`](toggle_group.md) —— <!-- TODO -->
- `Switch` —— <!-- TODO -->（文档待补）
- `Checkbox` —— <!-- TODO -->（文档待补）

## 已知空白

<!-- TODO: 如实列出。没有就写"暂无"。 -->

- <!-- TODO -->：没有 `toggle` / `button_pressed` 语义角色，`checked` 借用 checkbox 角色。
- <!-- TODO -->：没有图标槽位，纯图标 toggle 需要自绘。
- <!-- TODO -->：RTL 下方向键极性未反转（与 `RovingFocus` 的已知空白一致）。
