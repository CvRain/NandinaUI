# Breadcrumb

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。不要复述 API，说"人话"。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景（该用别的组件的情况）。反向场景最有价值，
     它能防止误用。 -->

| 场景 | 建议 |
| --- | --- |
| <!-- TODO --> | 本组件 |
| <!-- TODO --> | 改用 `XXX` |

## 快速开始

<!-- TODO: 最小可运行示例。优先用 builder（ui.make<X>(...)），而不是裸构造。
     示例要短、可编译、不依赖示例之外的上下文。 -->

```cpp
// TODO
```

## 外观与变体

<!-- TODO: 可点击条目（link）与当前页 / 纯文本条目（current）的语义差别，以及分隔符。 -->

| 变体 | 用途 |
| --- | --- |
| 带回调条目（`link` 配方） | <!-- TODO --> |
| 无回调条目（`current` 配方） | <!-- TODO --> |
| 分隔符（`separator` 配方，默认 `/`） | <!-- TODO --> |

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `Breadcrumb::create(theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `ui.make<Breadcrumb>()` | <!-- TODO --> |
| `add_item(std::string label, std::function<void()> on_click = {})` | <!-- TODO --> |
| `clear()` | <!-- TODO --> |
| `item_count() -> std::size_t` | <!-- TODO --> |
| `item_label(std::size_t index) -> std::string_view` | <!-- TODO --> |
| `item_clickable(std::size_t index) -> bool` | <!-- TODO --> |
| `set_separator(std::string separator)` | <!-- TODO --> |
| `separator() -> std::string_view`（默认 `/`） | <!-- TODO --> |
| `set_theme(theme::NanTheme theme)` | <!-- TODO --> |
| `theme_ref() -> const theme::NanTheme&` | <!-- TODO --> |
| `set_override(theme::BreadcrumbRecipeRule rule)` | <!-- TODO --> |
| `visual_state() -> theme::BreadcrumbVisualState` | <!-- TODO --> |
| `resolved_style() -> theme::ResolvedBreadcrumbStyle` | <!-- TODO --> |

## 槽位与组合

<!-- TODO: 没有命名槽位；条目是扁平列表，可点击条目在内部创建 `internal::BreadcrumbLink`
     子控件。说明追加顺序、当前页为什么没有回调、`clear()` 的生命周期处理。 -->

## 事件与绑定

<!-- TODO: 每个可点击条目的 `std::function<void()>` 回调；没有 reactive event。 -->

## 键盘与指针行为

<!-- TODO: 表格化。键盘是契约第 5 条的必填项。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | <!-- TODO: 依次进入每个可点击条目的 `internal::BreadcrumbLink`；无回调条目不可聚焦 --> |
| Enter / Space | <!-- TODO --> |
| Escape | <!-- TODO --> |
| 方向键 | <!-- TODO --> |
| 指针 | <!-- TODO --> |
| disabled 时 | <!-- TODO: 组件没有 disabled；条目要么是链接要么是纯文本 --> |

## 无障碍语义

<!-- TODO: 说明容器是 `semantics::Role::generic`、label 为用分隔符连接的整条路径；
     可点击条目暴露 `semantics::Role::button`（平台语义树没有 nav / breadcrumb / link role）。 -->

## 主题与覆盖

<!-- TODO: 该组件消费哪些语义角色？链接焦点环为什么默认开启？ -->

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container_fill` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `container_border` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `container_border_width` | `0` | <!-- TODO --> |
| `container_radius` | `0` | <!-- TODO --> |
| `link_color` | `ThemeColor::token(ColorToken::muted_foreground)` | <!-- TODO --> |
| `link_font_size` | `ThemeScalar::token(ScalarToken::typography_label_sm)`（14px） | <!-- TODO --> |
| `link_hover_color` | `ThemeColor::token(ColorToken::foreground)` | <!-- TODO --> |
| `current_color` | `ThemeColor::token(ColorToken::foreground)` | <!-- TODO --> |
| `current_font_size` | `ThemeScalar::token(ScalarToken::typography_label_sm)`（14px） | <!-- TODO --> |
| `separator_color` | `ThemeColor::token(ColorToken::muted_foreground)` | <!-- TODO --> |
| `link_focus_ring_color` | `ThemeColor::token(ColorToken::ring)` | <!-- TODO --> |
| `link_focus_ring_width` | `ThemeScalar::token(ScalarToken::border_focus_ring)`（2px） | <!-- TODO --> |
| `metrics_gap` | `ThemeScalar::token(ScalarToken::spacing_xs)`（4px） | <!-- TODO --> |
| `metrics_padding_x` | `0` | <!-- TODO --> |
| `metrics_min_height` | `0` | <!-- TODO --> |

规则选择器：`state`（`theme::BreadcrumbVisualState`，当前只有 `normal`）。

实例级覆盖示例：

```cpp
// TODO: set_override(...)
```

## 常见坑

<!-- TODO: 至少写 2 条真实踩过的坑。凭空想的坑不如不写。 -->

- <!-- TODO -->

## 相关组件

<!-- TODO: 3~5 个链接，说明各自解决什么，帮助读者横向选择。 -->

- [`Button`](button.md) —— <!-- TODO -->
- [`ToggleGroup`](toggle_group.md) —— <!-- TODO -->
- [`Pagination`](pagination.md) —— <!-- TODO -->

## 已知空白

<!-- TODO: 如实列出。没有就写"暂无"。 -->

- <!-- TODO: 无 nav / breadcrumb 语义 role，容器只能用 `Role::generic` + 整条路径 label；可点击条目用 `Role::button` -->
- <!-- TODO: 条目过多时不折叠、不换行，只按自然宽度排布并可能溢出容器 -->
