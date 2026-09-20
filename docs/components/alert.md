# Alert

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

<!-- TODO: 有哪些语义变体（tone / treatment / size / variant）？各自的适用语义是什么？
     不要罗列所有组合，说清楚"什么时候用哪个"。 -->

`theme::AlertTone` 是本组件唯一的语义变体，四档 tone 各对应一对语义色角色；容器填充 / 边框 / 图标色由默认设计系统按 tone 规则给出。

| 变体 | 对应语义色对 | 用途 |
| --- | --- | --- |
| `theme::AlertTone::info` | `ColorToken::info` / `ColorToken::info_foreground` | <!-- TODO --> |
| `theme::AlertTone::success` | `ColorToken::success` / `ColorToken::success_foreground` | <!-- TODO --> |
| `theme::AlertTone::warning` | `ColorToken::warning` / `ColorToken::warning_foreground` | <!-- TODO --> |
| `theme::AlertTone::error` | `ColorToken::error` / `ColorToken::error_foreground` | <!-- TODO --> |

`theme::AlertVisualState` 只有 `normal`：Alert 自身不可交互，没有 disabled 语义。

## 公开 API

<!-- TODO: 只列 recommended API。内部或实验性 API 明确标注。 -->

| 成员 | 说明 |
| --- | --- |
| `Alert(theme::NanTheme = theme::default_theme())` | 构造；缺省主题为 `theme::default_theme()` |
| `static create(theme::NanTheme = theme::default_theme())` | 工厂，返回 `std::shared_ptr<Alert>` |
| `set_tone(theme::AlertTone)` / `tone()` | 语义色家族，默认 `theme::AlertTone::info` |
| `set_title(std::string)` / `title()` | 标题；空标题时语义节点不暴露 |
| `set_description(std::string)` / `description()` | 描述；按可用宽度换行，最多 4 行 |
| `set_icon(std::shared_ptr<scene::NanControl>)` / `icon()` | 图标槽位；控件必须尚未挂载 |
| `set_action(std::shared_ptr<scene::NanControl>)` / `action()` | 操作槽位；控件必须尚未挂载 |
| `set_dismissible(bool)` / `dismissible()` | 是否显示关闭按钮；默认 `false` |
| `set_on_dismiss(std::function<void()>)` | 关闭回调；只回调，不负责把节点移出场景树 |
| `set_override(theme::AlertRecipeRule)` | 只覆盖明确指定的配方字段，主题切换后仍跟随新快照重解析 |
| `set_theme(theme::NanTheme)` / `theme_ref()` | 以完整主题覆盖，此后不再跟随系统切换 |
| `visual_state()` / `resolved_style()` | 当前状态与解析后的具体样式（供测试 / 调试） |

`on_measure()` / `on_layout()` / `on_draw()` / `semantics_properties()` 为受保护覆写。

## 槽位与组合

<!-- TODO: 若该组件有命名槽位或子内容约定，写清楚"能放什么、放几个、如何替换"。
     没有槽位就写"无槽位"，不要留空。 -->

| 槽位 | 接受 | 缺省 | 替换语义 |
| --- | --- | --- | --- |
| `icon` | `shared_ptr<scene::NanControl>` | 空，不占位 | `set_icon()` 移除旧内容并挂载新内容，新内容必须游离 |
| `title` | `std::string` | 空 | `set_title()` 直接替换文本 |
| `description` | `std::string` | 空 | `set_description()` 直接替换文本 |
| `action` | `shared_ptr<scene::NanControl>` | 空，不占位 | 同 `icon` |
| `dismiss` | Alert 内部持有的关闭按钮 | `set_dismissible(false)` 时不挂载 | 仅由 `set_dismissible()` 开关，不对外暴露 |

## 事件与绑定

<!-- TODO: callback、reactive Event、Signal 绑定各怎么用。
     若有 `ui.bind(...)` 路径，给出示例。 -->

关闭走 `set_on_dismiss(std::function<void()>)`，没有 reactive `Event`，也没有 `Signal` 绑定路径。

## 键盘与指针行为

<!-- TODO: 表格化。键盘是契约第 5 条的必填项。 -->

| 输入 | 行为 |
| --- | --- |
| Tab | Alert 自身不可聚焦；`dismissible` 为真时焦点可落到关闭按钮，否则 Alert 内没有可聚焦节点 |
| Enter / Space | 关闭按钮聚焦时激活 `on_dismiss` 回调 |
| Escape | <!-- TODO --> |
| 方向键 | 不消费 |
| 指针 | 关闭按钮接收移动 / 按下 / 抬起，落点在其包围盒内时激活 `on_dismiss`；`icon` / `action` 槽位的输入由放入的控件自行处理 |
| disabled 时 | 没有 disabled 状态；`dismissible(false)` 时关闭按钮从场景树移除 |

## 无障碍语义

<!-- TODO: role / label / value / state / action 各是什么，以及调用方需要提供什么
     （例如必须给 label 才能被辅助技术读出）。 -->

`Alert::semantics_properties()`：

| 字段 | 值 |
| --- | --- |
| `role` | `semantics::Role::generic`（`Role` 枚举没有 alert / banner 成员） |
| `label` | 标题文本 |
| `hint` | 描述文本 |
| `state` / `actions` | 不设置 |

标题为空时返回 `{}`（`role none`），该节点不会进入语义树。

关闭按钮是独立暴露的子节点：`role = semantics::Role::button`，`label = "Dismiss"`，`actions = semantics::Action::activate`，`state.focusable` 为真。

## 主题与覆盖

<!-- TODO: 该组件消费哪些语义角色？配方字段有哪些？
     字段若含**单位**语义（像素半径 vs 圆角半径之类），必须写明 —— 本项目已因这类
     语义错配出过一次事故。 -->

配方字段全部在 `theme::AlertRecipe` / `theme::AlertRecipeRule` / `theme::ResolvedAlertStyle` / `theme::AlertRecipes` 四处同步，解析入口为 `theme::resolve_alert(system, appearance, tone, state)`。

| 配方字段 | 默认 | 说明 |
| --- | --- | --- |
| `container.fill` | 每档 tone：`ThemeColor::with_alpha(<tone>, 0.12)` | 低透明度色调填充；base 为透明 |
| `container.border` | 每档 tone：`ThemeColor::token(<tone>)` | 实色 tone 边框 |
| `container.border_width` | `ThemeScalar::token(ScalarToken::border_thin)` | 描边宽度 |
| `container.radius` | `ThemeScalar::token(ScalarToken::radius_md)` | **圆角半径**（不是像素半径） |
| `title.color` | `ThemeColor::token(ColorToken::foreground)` | 刻意不用 tone 的 `*_foreground`（见「已知空白」） |
| `title.font_size` | `ThemeScalar::token(ScalarToken::typography_label_lg)` | 标题字号 |
| `description.color` | `ThemeColor::token(ColorToken::muted_foreground)` | 描述颜色 |
| `description.font_size` | `ThemeScalar::token(ScalarToken::typography_label_sm)` | 描述字号 |
| `icon` | 每档 tone：`ThemeColor::token(<tone>)` | 图标与关闭按钮字形颜色 |
| `metrics.gap` | `ThemeScalar::token(ScalarToken::spacing_sm)` | 列间距，同时用于标题与描述之间 |
| `metrics.padding_x` | `ThemeScalar::token(ScalarToken::spacing_lg)` | 水平内边距 |
| `metrics.padding_y` | `ThemeScalar::token(ScalarToken::spacing_md)` | 垂直内边距 |
| `metrics.min_height` | `ThemeScalar::literal(0.0F)` | 最小高度 |
| `metrics.box_size` | `ThemeScalar::literal(20.0F)` | 关闭按钮方形边长（组件几何，无语义 token） |
| `metrics.preferred_width` | `ThemeScalar::literal(320.0F)` | 无界约束下的首选宽度 |

实例级覆盖示例：

```cpp
// TODO: set_override(...)
```

## 常见坑

<!-- TODO: 至少写 2 条真实踩过的坑。凭空想的坑不如不写。 -->

- <!-- TODO -->

## 相关组件

<!-- TODO: 3~5 个链接，说明各自解决什么，帮助读者横向选择。 -->

- [`XXX`](xxx.md) —— <!-- TODO -->

## 已知空白

<!-- TODO: 如实列出。没有就写"暂无"。包括：未实现的状态、未覆盖的平台、
     尚未接通的无障碍能力等。 -->

- 关闭按钮的焦点环颜色直接取 `ColorToken::ring`，没有进 `AlertRecipe` / `AlertRecipeRule`，因此主题作者能改 `ring` token 但无法只为 Alert 的关闭按钮覆盖环色。
- 标题用 `foreground` 而不是 tone 的 `*_foreground`：后者是"实色 tone 填充上的文字色"，默认主题里 `info_foreground` / `error_foreground` 接近白色，落在 12% 透明度的浅色填充上不可读。若要按 tone 着色标题，目前只能靠 `set_override()`。
- 描述最多 4 行且固定 `TextOverflow::wrap`，没有配方字段可调；超长描述会被截断。
- `Escape` 不关闭、也不收起 Alert；关闭只能通过关闭按钮或调用方自己移除节点。
- 关闭按钮的 accessible label 固定为 `"Dismiss"`，没有公开 setter。
