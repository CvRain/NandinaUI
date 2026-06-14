# render

渲染抽象层。定义后端无关的绘制中间表示与后端接口，**仅依赖 foundation**。

## API

| 类型 | 说明 |
|------|------|
| `DrawCommand` | 绘制命令联合体：`fill_rect` / `fill_rounded_rect` / `draw_text` / `push_clip` / `pop_clip` |
| `Scene` | 绘制命令缓冲。`init` / `deinit` / `clear`（保留容量复用）/ 便捷构造方法 |
| `Backend` | 渲染后端接口（vtable）：`beginFrame` / `submit` / `endFrame` |
| `RenderTarget` | 渲染目标视图（像素缓冲 + 尺寸 + 格式） |
| `RecordingBackend` | 内置内存后端：录制命令序列，供测试断言 / 无头展示 |
| `SoftwareBackend` | 纯 Zig 软件光栅后端：把 Scene 画成 ARGB8888 像素（无外部依赖） |

## 用法

```zig
const render = @import("NandinaUI").render;

// 1) 产出一帧的绘制命令
var scene = render.Scene.init(allocator);
defer scene.deinit();
try scene.fillRect(bg_rect, surface_color);
try scene.fillRoundedRect(card_rect, 14, card_color);
try scene.pushClip(card_rect, 14);
try scene.drawText(.{ .text = "Hello", .x = 44, .y = 64, .font_size = 26, .color = ink });
try scene.popClip();

// 2) 提交给某个后端执行
var rec = render.RecordingBackend.init(allocator);
defer rec.deinit();
const backend = rec.interface();

try backend.beginFrame(.{ .width = 800, .height = 480 });
try backend.submit(&scene);
try backend.endFrame();
// rec.commands.items 即录制到的命令序列
```

可运行 `zig build showcase -- render-scene` 查看一帧绘制命令，或
`zig build showcase -- software-render` 看 SoftwareBackend 把 widget 树光栅化为像素（ASCII 预览）。

## 后端

- **RecordingBackend**：把提交命令拷进缓冲，供测试断言 / 无头展示。
- **SoftwareBackend**：纯 Zig 光栅器，把 `DrawCommand` 画成 ARGB8888 像素（填充矩形 /
  抗锯齿圆角 / 矩形裁剪栈 / source-over 混合）。无外部依赖，可独立测试；与未来 GPU
  后端（Vulkan / wgpu）是平级的 `Backend` 实现，可长期留作 fallback 与测试基准。
  文字渲染 v1 为占位（画基线条），等真实字体后端接入后替换。

## 设计要点

- 上层只产出绘制命令，不直接调用平台绘制 API —— render 与后端的抽象边界是
  架构优先稳定的边界之一。
- `Scene` 跨帧复用：`clear()` 保留底层容量，避免每帧重建分配（编码规范第 7 节）。
- `push_clip` / `pop_clip` 以裁剪栈语义服务于 Dialog / Popover / Card / ScrollView
  等容器的子内容裁剪。
- `DrawText.text` 是借用切片，调用方须保证其生命周期覆盖到 scene 被后端消费。
- 真实后端（软件光栅 ✅ / GPU 🚧）作为 `Backend` 的其它实现接入，不影响上层。

## 状态

✅ 已落地，含单元测试（Scene 构造 / clear 复用 / clip 配对 / RecordingBackend 录制 /
SoftwareBackend 填充·混合·裁剪·圆角）。运行 `zig build test` 验证。
