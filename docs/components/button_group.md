# ButtonGroup

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

<!-- TODO: 有哪些语义变体？各自的适用语义是什么？ -->

| 变体 | 用途 |
| --- | --- |
| `LayoutAxis::horizontal`（`create()` 默认） | <!-- TODO --> |
| `LayoutAxis::vertical` | <!-- TODO --> |

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `ButtonGroup::create(LayoutAxis axis = LayoutAxis::horizontal, theme::NanTheme theme = theme::default_theme())` | <!-- TODO --> |
| `ui.make<ButtonGroup>(LayoutAxis axis = LayoutAxis::horizontal)` | <!-- TODO --> |
| `add_button(std::shared_ptr<scene::NanControl> button) -> ButtonGroup&` | <!-- TODO --> |
| `clear()` | <!-- TODO --> |
| `button_count() -> std::size_t` | <!-- TODO --> |
| `set_orientation(LayoutAxis axis) -> ButtonGroup&` | <!-- TODO --> |
| `orientation() -> LayoutAxis` | <!-- TODO --> |
| `set_gap(float gap) -> ButtonGroup&` | <!-- TODO --> |
| `clear_gap()` | <!-- TODO --> |
| `gap() -> float` | <!-- TODO --> |
| `set_theme(theme::NanTheme theme)` | <!-- TODO --> |
| `theme_ref() -> const theme::NanTheme&` | <!-- TODO --> |
| `set_override(theme::ButtonGroupRecipeRule rule)` | <!-- TODO --> |
| `visual_state() -> theme::ButtonGroupVisualState` | <!-- TODO --> |
| `resolved_style() -> theme::ResolvedButtonGroupStyle` | <!-- TODO --> |

## 槽位与组合

<!-- TODO: 子按钮通过 `add_button()`（内部 `add_child`）挂载；说明顺序、重复挂载、
     移除与生命周期约定。 -->

## 事件与绑定

<!-- TODO: 本组件自身没有 callback / reactive event；交互属于子按钮。 -->

## 键盘与指针行为

<!-- TODO: 表格化。键盘是契约第 5 条的必填项。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | <!-- TODO: 组自身不可聚焦，焦点按子按钮顺序进入 --> |
| Enter / Space | <!-- TODO --> |
| Escape | <!-- TODO --> |
| 方向键 | <!-- TODO --> |
| 指针 | <!-- TODO --> |
| disabled 时 | <!-- TODO --> |

## 无障碍语义

<!-- TODO: 说明容器不发明 role（`semantics::Role::none`，场景树折叠后直接暴露子按钮），
     调用方需要给每个子按钮自己的语义。 -->

## 主题与覆盖

<!-- TODO: 该组件消费哪些语义角色？字段若含单位语义必须写明。 -->

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container_fill` | `ThemeColor::transparent(ColorToken::background)` | <!-- TODO --> |
| `container_border` | `ThemeColor::transparent(ColorToken::primary)` | <!-- TODO --> |
| `container_border_width` | `0` | <!-- TODO --> |
| `container_radius` | `ThemeScalar::token(ScalarToken::radius_md)`（8px） | <!-- TODO --> |
| `metrics_gap` | `ThemeScalar::token(ScalarToken::spacing_sm)`（8px） | <!-- TODO --> |
| `metrics_padding_x` | `0` | <!-- TODO --> |
| `metrics_min_height` | `0` | <!-- TODO --> |

规则选择器：`state`（`theme::ButtonGroupVisualState`，当前只有 `normal`）。

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
- `Row` / `Column` —— <!-- TODO -->
- [`ToggleGroup`](toggle_group.md) —— <!-- TODO -->

## 已知空白

<!-- TODO: 如实列出。没有就写"暂无"。 -->

- <!-- TODO: `attached`（相邻按钮共边）未实现：当前配方 / 绘制模型无法按子项位置覆盖圆角并抑制相邻边框，理由见 nandina/widget/button_group.hpp 类注释 -->
