# Pagination

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

<!-- TODO: 普通槽位（ghost + hover 叠加）、当前页槽位（primary 实色）、省略号槽位的
     视觉语义。 -->

| 变体 | 用途 |
| --- | --- |
| 普通页码 / 上一页 / 下一页（`item` 配方） | <!-- TODO --> |
| 当前页（`item_active` 配方） | <!-- TODO --> |
| 省略号（`ellipsis` 配方） | <!-- TODO --> |

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `Pagination::create(theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `ui.make<Pagination>(int page_count = 0, int current_page = 1)` | <!-- TODO --> |
| `ui.make<Pagination>(reactive::Signal<int>& current_page, int page_count)` | <!-- TODO --> |
| `set_page_count(int count)` | <!-- TODO: 负值按 0 处理；&lt;1 时整体塌缩为 0 尺寸 --> |
| `page_count() -> int` | <!-- TODO --> |
| `set_current_page(int page)` | <!-- TODO: 1-based，静默，钳制到 `[1, page_count]` --> |
| `current_page() -> int` | <!-- TODO --> |
| `set_sibling_count(int siblings)` | <!-- TODO: 默认 1，钳制到 &ge;0 --> |
| `sibling_count() -> int` | <!-- TODO --> |
| `set_disabled(bool disabled)` / `disabled() -> bool` | <!-- TODO --> |
| `go_to_page(int page)` | <!-- TODO: 用户激活路径，触发回调与事件 --> |
| `set_on_page_change(std::function<void(int)> callback)` | <!-- TODO --> |
| `page_changed() -> const reactive::Event<int>&` | <!-- TODO --> |
| `visible_item_count() -> std::size_t` | <!-- TODO --> |
| `roving_member_count() -> std::size_t` | <!-- TODO --> |
| `focused_item_index() -> int` | <!-- TODO --> |
| `item_is_page(std::size_t index) -> bool` | <!-- TODO --> |
| `item_page(std::size_t index) -> int` | <!-- TODO --> |
| `set_theme(theme::NanTheme theme)` | <!-- TODO --> |
| `theme_ref() -> const theme::NanTheme&` | <!-- TODO --> |
| `set_override(theme::PaginationRecipeRule rule)` | <!-- TODO --> |
| `visual_state() -> theme::PaginationVisualState` | <!-- TODO --> |
| `resolved_style() -> theme::ResolvedPaginationStyle` | <!-- TODO --> |

## 槽位与组合

<!-- TODO: 没有命名槽位；页码槽位由 `page_count` / `current_page` / `sibling_count`
     派生并按 `metrics.box_size` 绘制。 -->

## 事件与绑定

<!-- TODO: `set_on_page_change` 与 `page_changed` 只在用户激活时触发，程序化
     `set_current_page` 静默；给出 `ui.make<Pagination>(Signal<int>&, int)` 绑定示例。 -->

## 键盘与指针行为

<!-- TODO: 表格化。键盘是契约第 5 条的必填项。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | <!-- TODO: 整条分页器是一个焦点停靠点 --> |
| Enter / Space | <!-- TODO: 激活当前漫游槽位 --> |
| Escape | <!-- TODO --> |
| 方向键 | <!-- TODO: 复用 RovingFocus，跳过省略号与被禁用的边界槽位，可环绕 --> |
| Home / End | <!-- TODO: 跳到首个 / 末个可聚焦槽位 --> |
| 指针 | <!-- TODO: 点击槽位激活并把漫游焦点移到该槽位；hover 按槽位绘制 --> |
| disabled 时 | <!-- TODO: 不可聚焦、忽略输入、整体按 opacity.disabled 变暗 --> |

## 无障碍语义

<!-- TODO: 容器是 `semantics::Role::generic`，label 为 "Page X of Y"，value 为当前页；
     页码槽位不是子控件，无法各自成为语义节点。 -->

## 主题与覆盖

<!-- TODO: 该组件消费哪些语义角色？`box_size` 是无语义 token 的组件几何。 -->

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container_fill` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `container_border` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `container_border_width` | `0` | <!-- TODO --> |
| `container_radius` | `ThemeScalar::token(ScalarToken::radius_md)`（8px） | <!-- TODO --> |
| `item_fill` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `item_border` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `item_border_width` | `0` | <!-- TODO --> |
| `item_radius` | `ThemeScalar::token(ScalarToken::radius_md)`（8px） | <!-- TODO --> |
| `item_hover` | `ThemeColor::with_alpha(ColorToken::accent, ScalarToken::opacity_hover_overlay)`（alpha 0.10） | <!-- TODO --> |
| `item_active_fill` | `ThemeColor::token(ColorToken::primary)` | <!-- TODO --> |
| `item_active_border` | `ThemeColor::transparent(ColorToken::primary)` | <!-- TODO --> |
| `item_active_border_width` | `0` | <!-- TODO --> |
| `item_active_radius` | `ThemeScalar::token(ScalarToken::radius_md)`（8px） | <!-- TODO --> |
| `label_color` | `ThemeColor::token(ColorToken::foreground)` | <!-- TODO --> |
| `label_font_size` | `ThemeScalar::token(ScalarToken::typography_label_sm)`（14px） | <!-- TODO --> |
| `label_active_color` | `ThemeColor::token(ColorToken::primary_foreground)` | <!-- TODO --> |
| `label_active_font_size` | `ThemeScalar::token(ScalarToken::typography_label_sm)`（14px） | <!-- TODO --> |
| `ellipsis_color` | `ThemeColor::token(ColorToken::muted_foreground)` | <!-- TODO --> |
| `focus_ring_color` | `ThemeColor::token(ColorToken::ring)` | <!-- TODO --> |
| `focus_ring_width` | `0`（`focused` 规则设为 `ThemeScalar::token(ScalarToken::border_focus_ring)`，2px） | <!-- TODO --> |
| `metrics_box_size` | `32` | <!-- TODO --> |
| `metrics_gap` | `ThemeScalar::token(ScalarToken::spacing_xs)`（4px） | <!-- TODO --> |
| `metrics_padding_x` | `0` | <!-- TODO --> |
| `metrics_min_height` | `0` | <!-- TODO --> |

规则选择器：`state`（`theme::PaginationVisualState`：`normal` / `focused` / `disabled`）。

实例级覆盖示例：

```cpp
// TODO: set_override(...)
```

## 常见坑

<!-- TODO: 至少写 2 条真实踩过的坑。凭空写不如不写。 -->

- <!-- TODO -->

## 相关组件

<!-- TODO: 3~5 个链接，说明各自解决什么，帮助读者横向选择。 -->

- [`Button`](button.md) —— <!-- TODO -->
- [`ToggleGroup`](toggle_group.md) —— <!-- TODO -->
- [`Breadcrumb`](breadcrumb.md) —— <!-- TODO -->

## 已知空白

<!-- TODO: 如实列出。没有就写"暂无"。 -->

- <!-- TODO: 页码槽位是绘制出来的，无法各自成为语义节点，辅助技术只能读到容器级 "Page X of Y" -->
- <!-- TODO: 页码窗口算法不做 "1 … 3" 这类小间隙合并以外的特殊优化；`sibling_count` 很大时仍受总量小则全展开的规则约束 -->
