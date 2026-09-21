# TextArea

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。不要复述 API，说"人话"。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景（该用别的组件的情况）。反向场景最有价值，
     它能防止误用。 -->

| 场景 | 建议 |
| --- | --- |
| <!-- TODO --> | 本组件 |
| <!-- TODO --> | 改用 `TextField` |

## 快速开始

<!-- TODO: 最小可运行示例。优先用 builder（ui.make<TextArea>(...)），而不是裸构造。
     示例要短、可编译、不依赖示例之外的上下文。 -->

```cpp
// TODO
```

## 外观与变体

<!-- TODO: 有哪些语义变体？各自的适用语义是什么？ -->

| 变体 | 用途 |
| --- | --- |
| <!-- TODO --> | <!-- TODO --> |

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `TextArea(std::string value = {}, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `static create(std::string value = {}, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `set_value(std::string)` / `value() -> std::string_view` | <!-- TODO --> |
| `set_placeholder(std::string)` / `placeholder() -> std::string_view` | <!-- TODO --> |
| `set_read_only(bool)` / `read_only() -> bool` | <!-- TODO --> |
| `set_disabled(bool)` / `disabled() -> bool` | <!-- TODO --> |
| `set_rows(int)` / `rows() -> int` | <!-- TODO --> |
| `set_on_change(std::function<void(std::string_view)>)` | <!-- TODO --> |
| `value_changed() -> const reactive::Event<std::string_view>&` | <!-- TODO --> |
| `set_theme(theme::NanTheme)` / `theme_ref() -> const theme::NanTheme&` | <!-- TODO --> |
| `set_override(theme::TextAreaRecipeRule)` | <!-- TODO --> |
| `visual_state() -> theme::TextAreaVisualState` | <!-- TODO --> |
| `resolved_style() -> theme::ResolvedTextAreaStyle` | <!-- TODO --> |
| `set_text_pipeline(primitives::TextPipeline)` / `text_pipeline()` | <!-- TODO --> |
| `editable_text() -> primitives::EditableText&` | <!-- TODO --> |
| `scroll_offset() -> foundation::NanPoint` | <!-- TODO --> |
| `ui.make<TextArea>(std::string value, std::string placeholder)` | <!-- TODO --> |
| `ui.make<TextArea>(reactive::Signal<std::string>&, std::string placeholder)` | <!-- TODO --> |

## 槽位与组合

<!-- TODO: 无槽位就写"无槽位"，不要留空。 -->

## 事件与绑定

<!-- TODO: callback、reactive Event、Signal 绑定各怎么用。 -->

## 键盘与指针行为

<!-- TODO: 表格化。键盘是契约第 5 条的必填项。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | <!-- TODO --> |
| Enter / Space | <!-- TODO --> |
| Escape | <!-- TODO --> |
| 方向键 | <!-- TODO --> |
| 指针 | <!-- TODO --> |
| disabled 时 | <!-- TODO --> |

## 无障碍语义

<!-- TODO: role / label / value / state / action 各是什么，以及调用方需要提供什么。 -->

## 主题与覆盖

<!-- TODO: 该组件消费哪些语义角色？配方字段有哪些？字段若含**单位**语义，必须写明。 -->

配方遵循四步同步：`TextAreaVisualState` → `TextAreaRecipe` / `TextAreaRecipeRule` / `ResolvedTextAreaStyle` / `TextAreaRecipes` → `resolve_text_area()` → `default_text_area_recipe()`。

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container.fill` | `token(background)` | <!-- TODO --> |
| `container.border` | `token(input)` | <!-- TODO --> |
| `container.border_width` | `token(border_thin)` | <!-- TODO --> |
| `container.radius` | `token(radius_md)` | <!-- TODO --> |
| `value.color` | `token(foreground)` | <!-- TODO --> |
| `value.font_size` | `token(typography_label_sm)` | <!-- TODO --> |
| `placeholder.color` | `token(muted_foreground)` | <!-- TODO --> |
| `placeholder.font_size` | `token(typography_label_sm)` | <!-- TODO --> |
| `selection` | `token(selection)` | <!-- TODO --> |
| `focus.color` | `token(ring)` | <!-- TODO --> |
| `focus.width` | `literal(0.0F)`（focused 规则开启为 `token(border_focus_ring)`） | <!-- TODO --> |
| `metrics.rows` | `literal(3.0F)` | <!-- TODO --> |
| `metrics.line_height` | `literal(20.0F)` | <!-- TODO --> |
| `metrics.padding_x` | `token(spacing_md)` | <!-- TODO --> |
| `metrics.padding_y` | `token(spacing_sm)` | <!-- TODO --> |
| `TextAreaRecipeRule::state` | `std::optional<TextAreaVisualState>` | <!-- TODO --> |
| `TextAreaRecipeRule::read_only` | `std::optional<bool>`（`true` 命中只读实例，容器底切到 `token(muted)`） | <!-- TODO --> |

实例级覆盖示例：

```cpp
// TODO: set_override(...)
```

## 常见坑

<!-- TODO: 至少写 2 条真实踩过的坑。凭空想的坑不如不写。 -->

- <!-- TODO -->

## 相关组件

<!-- TODO: 3~5 个链接，说明各自解决什么，帮助读者横向选择。 -->

- `TextField`（单行文本输入）—— <!-- TODO -->
- `Label`（只读文本展示）—— <!-- TODO -->
- `ScrollView`（通用滚动容器）—— <!-- TODO -->

## 已知空白

<!-- TODO: 如实列出。包括：未实现的状态、未覆盖的平台、尚未接通的无障碍能力等。 -->

- <!-- TODO: 语义 role：`semantics::Role` 枚举没有多行文本成员，当前沿用 `Role::text_field`。 -->
- <!-- TODO: 未绘制滚动条，`TextAreaRecipe` 也没有 scrollbar 字段。 -->
- <!-- TODO: IME 组合文本（`primitives::TextComposition`）只保存不改变布局，与 `TextField` 同一已知空白。 -->
