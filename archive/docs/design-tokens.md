# Design Tokens

> 状态：已校正（2026-05-31）
> 当前判断：NandinaUI 已具备 primitive tokens、palette、ThemeManager 与第一批 style primitives；本文用于固定 token 的职责边界与选型规则，避免后续控件继续直接写死颜色、圆角、字体和描边。

## 目的

NandinaUI 当前的 theme/widgets 主线，不只是“给每个控件找一组默认值”，而是建立一套可以被 primitives、controls 和未来 style 层共同消费的 token 体系。

这套体系要回答四个问题：

- 哪些值属于 primitive token，而不是组件私有常量。
- 哪些值应该先通过 semantic palette 表达，再落到组件样式。
- `preset`、`size`、`colorVariant` 分别解决什么问题。
- 控件作者什么时候应该复用现有 token，什么时候才该增加新 token。

## 分层模型

当前推荐的心智模型如下：

```text
Primitive Tokens
  -> Palette / Typography / Border / Radius / Spacing
  -> Resolved Component Style
  -> Widgets / Controls
  -> Authoring DSL / Showcase
```

其中：

- primitive token 负责提供可复用的原子值。
- palette 负责提供 semantic color role，而不是直接告诉 Button 应该画什么色。
- resolved component style 负责把 token 和语义状态翻译成具体的组件默认样式。
- widgets/control 负责行为、布局和语义 API，不应该继续私有一套颜色/尺寸表。

## Primitive Tokens

当前 theme 层已经有以下 primitive token 组：

- spacing：控制 padding、gap、内容区留白。
- radius：控制 corner radius。
- border：控制 border width、divider width、focus ring width。
- typography：控制字号、字重、行高、字间距与 TypographyRole 对应关系。
- elevation：控制层级深度。
- opacity：控制 disabled/scrim 等透明度语义。

这些 token 的职责是“提供尺度”，不是“决定组件行为”。

例如：

- `spacing.large` 可以被 Button padding、Card padding、Field 间距同时复用。
- `border.focus_ring` 应被 FocusRing 和输入类控件共享，而不是每个控件各写一个 `focus_ring_width`。
- `radius.small` 应优先作为 Button/Input/Card 的统一圆角来源。

## Semantic Palette

颜色不能直接围绕具体组件命名，否则 theme 很快会退化成局部硬编码表。

当前 palette 采用 semantic color role：

- `primary` / `onPrimary`
- `secondary` / `onSecondary`
- `error` / `onError`
- `surface` / `onSurface`
- `surfaceVariant` / `onSurfaceVariant`
- `outline` / `outlineVariant`

控件不应该优先声明“我要一组蓝色按钮”，而应该先表达：

- 默认 filled button 用 `primary` + `onPrimary`
- outlined button 边框用 `outline`
- 弱提示文本优先用 `onSurfaceVariant`
- 错误态优先用 `error`

这样 palette 才能在 light/dark 切换时统一生效。

## TypographyRole

Typography token 已经提供完整 type scale，但控件作者不应继续主要依赖裸 `font_size` / `font_weight` 数值。

当前推荐优先使用 role：

- `display_*`：展示级大标题
- `headline_*`：页面或区块标题
- `title_*`：卡片、面板、字段标题
- `body_*`：正文与输入文本
- `label_*`：按钮、小标签、辅助说明

选择规则：

- 页面主标题优先 `headline_large` 或 `display_small`
- Section 标题优先 `title_large` / `title_medium`
- 普通正文优先 `body_medium`
- Button 文本优先 `label_large`
- 辅助文字优先 `body_small` 或 `label_medium`

如果某个控件需要表达的是“文本角色”，优先暴露 `TypographyRole` API；只有当角色仍不够表达时，再额外开放原始字体微调。

## Preset / Size / ColorVariant

这三个概念经常被混写，但它们负责的是不同维度。

### Preset

`preset` 或 `variant` 解决的是“组件长什么样”。

例如 Button 的：

- `filled`
- `tonal`
- `outlined`
- `ghost`
- `destructive`
- `link`

它决定的是背景、边框、文本颜色、hover/pressed 行为这些视觉组合，而不是尺寸。

### Size

`size` 解决的是“组件占多大、内部留白和文字多大”。

例如 Button 的：

- `xs`
- `sm`
- `md`
- `lg`
- `icon`

它通常映射到：

- height
- padding
- gap
- icon size
- font size

### ColorVariant

`colorVariant` 解决的是“相同 preset 下，用哪一类语义颜色族”。

它不应该和 preset 混为一谈。

例如未来 Button 可以表达：

- filled + primary
- filled + secondary
- ghost + destructive

也就是说：

- preset 决定样式结构
- colorVariant 决定颜色家族

如果没有 colorVariant，组件很容易把“destructive”“secondary”“outline”这些概念全部塞进一个大枚举里，后续扩展会越来越混乱。

## Spacing / Radius 选型规则

当前推荐的简单规则如下：

- 内部微间距优先 `spacing.xsmall` 或 `spacing.small`
- 普通控件 padding 优先 `spacing.medium` 或 `spacing.large`
- 卡片/面板内容区优先 `spacing.large`
- 页面级区块留白优先 `spacing.xlarge` 或更大

圆角选择规则：

- 常规输入框、按钮：优先 `radius.small`
- 卡片/面板：优先 `radius.small` 或 `radius.medium`
- pill / tag / badge：优先 `radius.full`

不要在单个控件内部直接新增 `7px`、`13px`、`19px` 这种孤立数值，除非已经证明现有 token 无法表达。

## 控件作者的使用规则

新增或修改控件时，建议按下面顺序思考：

1. 这个值是不是 spacing/radius/border/typography 维度，如果是，先找 primitive token。
2. 这个颜色是不是语义色，如果是，先找 palette role，而不是直接写 `NanRgb`。
3. 这个差异是 preset、size 还是 colorVariant，不要把三个维度折叠到一个枚举里。
4. 这个文本 API 是不是应该先暴露 `TypographyRole`，而不是只暴露裸字号。
5. 如果某个值未来会被第二个控件复用，就不要把它留在控件私有实现里。

## 当前已知空白

到 2026-05-31 为止，token 体系已经可用，但还没有完全收口：

- `colorVariant` 已在 Button、TextField、SidebarMenuButton 与 ProgressBar 中形成统一公开 API，但其他 widgets 仍未完全跟进。
- 不是所有 widgets 都已经完全通过 semantic palette 解析默认色。
- 未来仍需要 `style.cppm` 这类更高层 recipe/style layer，把 token、palette、primitive 和 control 进一步解耦。

因此，当前正确的方向不是回到局部常量，而是继续沿着：

```text
token -> semantic role -> resolved style -> widget behavior
```

这条链路收口。