# text

文本能力层。**仅依赖 foundation**；测量结果供 layout / render 使用。借鉴 EUI-NED 的文字渲染机制。

## API

| 模块 | 内容 |
|------|------|
| `font` | `TextStyle`（字号/字重/行高/字间距）、`FontMetrics` 度量接口（vtable）、`MonospaceMetrics` 等宽估算占位后端 |
| `layout` | `Overflow` 溢出策略、`Constraints`（max_width / max_lines）、`measure(...) -> TextLayout` |

## 溢出策略（一等公民）

`measure` 把**约束宽度 + 换行 + 行数上限 + 溢出策略**作为输入，从根上保证布局结果不超过
约束 —— 杜绝「无界换行导致文字溢出组件」（archive 教训）。

| 策略 | 行为 | 开销 |
|------|------|------|
| `clip` | 单行，超宽硬截断（无省略号） | 低 |
| `ellipsis` **默认** | 单行，超宽末尾放 "…" | 低 |
| `wrap` | 按宽度折行，受 `max_lines` 限制，末行超出仍以 "…" 收尾 | 中 |
| `scale` | 按比例缩小字号直到放下 | 较高 |

## 用法

```zig
const text = @import("NandinaUI").text;

var backend = text.MonospaceMetrics{};        // 占位后端（真实后端后续接 HarfBuzz）
const metrics = backend.interface();

var layout = try text.measure(
    allocator,
    "一段可能很长的文本……",
    .{ .font_size = 14 },
    .ellipsis,                                 // 默认策略
    .{ .max_width = 200, .max_lines = 2 },
    metrics,
);
defer layout.deinit();
// layout.size  → 文本实际占用尺寸（上报给 layout 层求 preferred size）
// layout.lines → 每行文本切片 + 宽度 + 是否省略
// layout.truncated → 是否被截断
```

可运行 `zig build showcase -- text-overflow` 对比四种策略与 text→render 裁剪链路。

## 设计要点

- **只负责测量与布局**，不含点击 / 状态机 / 容器边框等语义。
- **不做裁剪**：text 只保证「布局结果不超过约束」；像素级裁剪由 render 的 push_clip/pop_clip
  负责，未来 runtime/widgets 提供统一 `ClipNode` 复用 —— 文本组件不必各自造裁剪轮子
  （archive 教训：曾为每个组件单独设 clip，应改为统一组件）。
- 度量接口与具体字体引擎解耦（依赖规则第 7 条）：现用等宽估算占位后端，真实 HarfBuzz /
  FreeType 后端作为 `FontMetrics` 的另一实现后续接入，不影响上层。
- widgets 层的 Text primitive 建立在本层之上；Label 是 Text 的语义包装。

## 状态

✅ 已落地，含单元测试（clip / ellipsis / wrap / max_lines / scale / 显式换行 / CJK 双宽 /
无约束）。运行 `zig build test` 验证。见 [组件 Primitives · Text](../../docs/development/widget-primitives.md)。
