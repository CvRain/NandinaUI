# 设计令牌（Design Tokens）

> 状态：已校正
> 来源：改写自旧 C++ `archive/docs/design-tokens.md`，token 选型规则在 Zig 重写中继续有效。

## 目的

theme/widgets 主线不是「给每个控件找一组默认值」，而是建立一套可被 primitives、controls
和未来 style 层共同消费的 token 体系。它要回答四个问题：

- 哪些值属于 primitive token，而不是组件私有常量。
- 哪些值应先通过 semantic palette 表达，再落到组件样式。
- `preset`、`size`、`colorVariant` 分别解决什么问题。
- 控件作者何时复用现有 token，何时才该新增。

## 分层模型

```
Primitive Tokens
  -> Palette / Typography / Border / Radius / Spacing
  -> Resolved Component Style
  -> Widgets / Controls
  -> Authoring DSL / Showcase
```

- primitive token：提供可复用的原子值（尺度），不决定组件行为。
- palette：提供 semantic color role，而不是直接告诉 Button 画什么色。
- resolved style：把 token 与语义状态翻译成具体组件默认样式。
- widgets/control：负责行为、布局与语义 API，不私有一套颜色/尺寸表。

## Primitive Tokens

| token 组 | 控制内容 |
|----------|----------|
| `spacing` | padding、gap、内容区留白 |
| `radius` | corner radius |
| `border` | border width、divider width、focus ring width |
| `typography` | 字号、字重、行高、字间距与 role 对应 |
| `elevation` | 层级深度 |
| `opacity` | disabled / scrim 等透明度语义 |

职责是「提供尺度」，不是「决定组件行为」。例如 `spacing.large` 可被 Button / Card / Field 同时复用。

## Semantic Palette

颜色按 semantic role 命名，而非具体组件：

`primary / onPrimary`、`secondary / onSecondary`、`error / onError`、
`surface / onSurface`、`surfaceVariant / onSurfaceVariant`、`outline / outlineVariant`。

控件应先表达语义意图（filled button 用 `primary` + `onPrimary`，错误态用 `error`），
这样 light/dark 切换才能统一生效。

## TypographyRole

控件优先使用 role 而非裸字号：`display_* / headline_* / title_* / body_* / label_*`。
- 页面主标题 → `headline_large` 或 `display_small`
- section 标题 → `title_large` / `title_medium`
- 正文 → `body_medium`
- 按钮文本 → `label_large`
- 辅助文字 → `body_small` / `label_medium`

## Preset / Size / ColorVariant（三个独立维度）

- **preset / variant**：组件「长什么样」——`filled / tonal / outlined / ghost / destructive / link`，决定背景、边框、文本色、hover/pressed 行为。
- **size**：组件「占多大」——`xs / sm / md / lg / icon`，映射到 height、padding、gap、icon size、font size。
- **colorVariant**：相同 preset 下「用哪一族语义颜色」——如 `filled + primary`、`ghost + destructive`。

三者不要折叠进一个大枚举，否则扩展会越来越混乱。

## 控件作者使用规则

1. 这个值是不是 spacing/radius/border/typography 维度？是 → 先找 primitive token。
2. 这个颜色是不是语义色？是 → 先找 palette role，不要直接写 RGB。
3. 这个差异是 preset、size 还是 colorVariant？不要折叠到一个枚举。
4. 文本 API 是不是该先暴露 `TypographyRole`，而不是裸字号？
5. 这个值未来会被第二个控件复用吗？会 → 不要留在控件私有实现里。

不要在单个控件内直接新增 `7px / 13px / 19px` 这类孤立数值，除非已证明现有 token 无法表达。

## 收口方向

```
token -> semantic role -> resolved style -> widget behavior
```
