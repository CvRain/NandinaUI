# 主题与设计令牌

NandinaUI 的外观由**设计令牌（design token）**驱动，而不是在每个组件里写死颜色和尺寸。
这让「换肤」「light/dark 切换」「统一调整圆角/间距」变得简单可控。

> 当前状态：`theme` 层为骨架。本文描述目标使用方式。

## 心智模型

```
primitive token（原子尺度值）
  → semantic palette（语义颜色角色）
  → resolved component style（组件解析样式）
  → 你的组件
```

你在写界面时，优先表达**语义意图**，而不是具体数值：

- 不写「这个按钮背景是 #3b82f6」，而写「这是 primary 按钮」。
- 不写「padding 16px」，而用 `spacing.large`。
- 不写「圆角 6px」，而用 `radius.small`。

这样切换主题时，所有引用同一语义的组件会一起更新。

## Primitive Token

提供可复用的原子尺度值：

| token 组 | 用途 |
|----------|------|
| `spacing` | padding、gap、留白（`xsmall` … `xlarge`） |
| `radius` | 圆角（`small` / `medium` / `full`） |
| `border` | 边框宽、分隔线宽、focus ring 宽 |
| `typography` | 字号、字重、行高与文本角色 |
| `elevation` | 层级深度 |
| `opacity` | disabled / 遮罩等透明度 |

## Semantic Palette

颜色按语义角色命名，而非具体组件：

`primary / onPrimary`、`secondary / onSecondary`、`error / onError`、
`surface / onSurface`、`surfaceVariant / onSurfaceVariant`、`outline / outlineVariant`。

`onX` 表示「画在 X 之上的前景色」，例如 `onPrimary` 是 primary 背景上的文字色。

## 文本角色（TypographyRole）

用角色而非裸字号选择文本样式：

| 场景 | 角色 |
|------|------|
| 页面主标题 | `headline_large` / `display_small` |
| 区块标题 | `title_large` / `title_medium` |
| 正文 | `body_medium` |
| 按钮文本 | `label_large` |
| 辅助文字 | `body_small` / `label_medium` |

## 组件的三个独立维度

定制组件外观时，区分三个不要混淆的维度：

- **preset / variant**：组件「长什么样」——`filled` / `tonal` / `outlined` / `ghost` / `destructive` / `link`。
- **size**：组件「占多大」——`xs` / `sm` / `md` / `lg` / `icon`。
- **colorVariant**：相同 preset 下「用哪一族语义颜色」——如 `filled + primary`、`ghost + destructive`。

例如：

```zig
button("Delete").variant(.destructive).size(.md)
button("Save").variant(.filled).color(.primary)
```

## 自定义主题

> 形态预览，API 待 theme 层落地后定稿。

```zig
const my_theme = Theme{
    .palette = .{
        .primary = Color.fromHexRgb(0x3B82F6),
        .surface = Color.fromHexRgb(0x0F172A),
        // ...
    },
    .radius = .{ .small = 6, .medium = 10, .full = 9999 },
};

app.setTheme(my_theme);
```

切换 light/dark 时，只需替换 palette，组件无需改动。

## 进一步阅读

token 选型规则与设计原理见开发文档 [设计令牌](../development/design-tokens.md)。
