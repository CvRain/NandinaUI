# theme

主题与设计令牌层。**依赖 foundation**（颜色等）。遵循 design-system-first。

## 计划 API

- 颜色 token / semantic palette：`primary` / `surface` / `outline` / `error` 等角色（含 `onX` 前景色）。
- 尺度 token：`spacing` / `radius` / `border` / `elevation` / `opacity`。
- `typography`：字号/字重/行高 + `TypographyRole`（`display_* / headline_* / title_* / body_* / label_*`）。
- `Theme` 结构与 resolver：把 token + 语义状态解析为组件默认样式。

## 设计要点

- token 只提供值，不与具体 widget 耦合。
- 控件优先消费语义 role，而不是写死颜色/尺寸；light/dark 切换只换 palette。
- `preset` / `size` / `colorVariant` 三个维度分开，不折叠进一个枚举。

## 状态

🚧 骨架。使用方式见 [主题指南](../../docs/guide/theming.md)，选型规则见 [设计令牌](../../docs/development/design-tokens.md)。
