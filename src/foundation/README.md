# foundation

基础能力层：与具体 UI 无关的纯数据类型。**无上游依赖**，是整个依赖图的根。

## 模块

| 文件 | 内容 |
|------|------|
| `geometry.zig` | `Point` / `Size` / `Rect` / `Insets` |
| `color.zig` | `Color`（渲染就绪的归一化 RGBA） |
| `color_space.zig` | `Oklab` / `Oklch` / `Lab` / `Lch` / `Rgb` / `HexRgb` 多色彩空间与统一 `convert` |

## 使用

```zig
const f = @import("NandinaUI").foundation;

// 几何
const r = f.Rect.fromXywh(10, 20, 100, 80);
_ = r.width();        // 100
_ = r.center();       // Point{ .x = 60, .y = 60 }
_ = r.intersected(other);

const pad = f.Insets.all(8);
const inner = pad.applyToRect(r);  // 向内收缩

// 颜色（渲染就绪的 RGBA）
const c = f.Color.fromHexRgb(0x3B82F6);
const faded = c.withAlpha(0.5);
const mid = f.Color.black.lerp(f.Color.white, 0.5);

// 色彩空间转换（以 OKLab 为中转枢纽）
const cs = f.color_space;
const blue = cs.HexRgb.fromRgb(0x3B82F6);
var oklch = cs.convert(cs.Oklch, blue); // 转到感知均匀空间
oklch.l += 0.1;                          // 提亮（hover / dark 变体）
const lighter = cs.toColor(oklch);       // 转回渲染就绪 Color
```

## 约定

- 坐标统一用 `f32`，窗口像素空间。
- `Rect` 内部以 LTRB（left/top/right/bottom）存储，对外同时提供 `x/y/width/height`。
- `Color` 是渲染就绪类型（render Backend 直接消费），内部 0..1 浮点分量。
- 调色运算（亮度 / 色相变体）在 `color_space` 的感知均匀空间（OKLab/OKLCH）里做，
  算完用 `toColor` 转回 `Color`；库使用者可定义自有格式（实现 `toOklab`/`fromOklab`）直接参与 `convert`。
- 所有类型是值语义 struct，可直接拷贝。

## 状态

✅ 已落地，含单元测试。运行 `zig build test` 验证。
