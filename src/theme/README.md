# theme

主题与设计令牌层。**仅依赖 foundation**。遵循 design-system-first：先 token / palette /
语义 API，再具体 widget。

## API

| 模块 | 内容 |
|------|------|
| `tokens` | primitive token：`Spacing` / `Radius` / `Border` / `Elevation` / `Opacity` / `Typography`，聚合为 `Tokens` |
| `palette` | 语义颜色：`ColorRole`（primary/surface/outline/error… 含 onX 前景色）+ `Scheme`（light/dark）+ `Palette` |
| `theme.Theme` | 聚合 name + palette + tokens + scheme，提供按角色解析的 resolver |

## 用法

```zig
const theme = @import("NandinaUI").theme;

// 库默认主题
var t = theme.Theme.default();

// 按语义角色取色（用当前 scheme）
const bg = t.color(.surface);
const fg = t.color(.on_surface);

// light / dark 切换
t.toggleScheme();
const dark_bg = t.color(.surface);

// token 解析
const r = t.tokens.radius.md;          // 12
const ts = t.typeStyle(.headline_large); // 字号/字重/行高/字间距
```

## 「类似 global.css」的自定义

库提供完整默认主题，开发者可宽松重写整套样式与配色：

```zig
var brand = theme.Theme{
    .name = "brand",
    .palette = theme.Palette.default(),
    .tokens = .{ .radius = .{ .md = 6 } }, // 只覆盖要改的字段，其余取默认
};
const blue = foundation.Color.fromHexRgb(0x3B82F6);
brand.palette.set(.primary, blue, blue);  // 逐角色覆盖品牌色
```

可运行 `zig build showcase -- theme` 查看默认主题、light/dark 切换、自定义与 theme→render。

## 设计要点

- token 只提供值，不与具体 widget 耦合；控件优先消费语义 role，而不是写死颜色 / 尺寸。
- `Theme` 是值语义，可整体拷贝 / 替换 —— 这是上层「多主题快速切换」的基础（用 reactive
  的 `Signal(Theme)` 持有当前主题即可，theme 层不内建全局 manager，保持只依赖 foundation）。
- 调色变体（hover / 提亮 / dark 派生）在 `foundation.color_space` 的 OKLCH 空间运算，
  palette 只存终值。
- `preset` / `size` / `colorVariant` 三个维度待 widgets 落地时再分开建模，不折叠进一个枚举。

## 状态

✅ 已落地，含单元测试（token 默认 / 覆盖、palette light·dark、scheme 切换、resolver、
自定义主题）。运行 `zig build test` 验证。
使用方式见 [主题指南](../../docs/guide/theming.md)，选型规则见 [设计令牌](../../docs/development/design-tokens.md)。
