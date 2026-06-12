# foundation

基础能力层：与具体 UI 无关的纯数据类型。**无上游依赖**，是整个依赖图的根。

## 模块

| 文件 | 内容 |
|------|------|
| `geometry.zig` | `Point` / `Size` / `Rect` / `Insets` |
| `color.zig` | `Color`（归一化 RGBA） |

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

// 颜色
const c = f.Color.fromHexRgb(0x3B82F6);
const faded = c.withAlpha(0.5);
const mid = f.Color.black.lerp(f.Color.white, 0.5);
```

## 约定

- 坐标统一用 `f32`，窗口像素空间。
- `Rect` 内部以 LTRB（left/top/right/bottom）存储，对外同时提供 `x/y/width/height`。
- `Color` 内部以 0..1 浮点分量存储，提供 8-bit / hex 互转。
- 所有类型是值语义 struct，可直接拷贝。

## 状态

✅ 已落地，含单元测试。运行 `zig build test` 验证。
