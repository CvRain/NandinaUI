# runtime

UI 运行时层。**依赖 foundation / reactive / render / layout**。借鉴 slint 的组件挂载机制。

## API

| 模块 | 内容 |
|------|------|
| `event` | 输入事件：`Event`（指针 / 键盘 / 文本 / 焦点 / 窗口）、`PointerButton`、`KeyModifiers` |
| `node` | `Node`（嵌入式 + vtable）：owning 子节点、几何 bounds、dirty 传播、命中测试；`VTable`、`EventResult` |
| `tree` | `Tree`：拥有根节点，驱动一帧 `relayout → repaint` 产出 `render.Scene`，分发事件 |

## 一帧闭环

```
signal 变化 → node.markLayoutDirty / markPaintDirty 冒泡到根
  → tree.frame()
      → relayout：measure（自底向上）+ layout（自顶向下，节点内用 layout 求解器）
      → repaint： 深度优先 paint，产出 Scene 命令
  → backend.submit(scene)
```

## 节点模式

具体节点把 `Node` 作为字段嵌入，通过 `@fieldParentPtr` 取回自身，用 vtable 多态分发
（与 reactive.graph.Node 一致）：

```zig
const Box = struct {
    node: runtime.Node,
    color: foundation.Color,
    const vtable = runtime.VTable{ .measure = measure, .paint = paint, .deinit = deinitImpl };
    fn measure(node: *runtime.Node, c: layout.Constraints) foundation.Size { ... }
    fn paint(node: *runtime.Node, scene: *render.Scene) anyerror!void { ... }
    fn deinitImpl(node: *runtime.Node, a: std.mem.Allocator) void {
        a.destroy(@as(*Box, @fieldParentPtr("node", node)));
    }
};
```

`VTable` 只有 `measure` 必填，其余（layout / paint / handle_event / deinit）有默认实现。
可运行 `zig build showcase -- runtime-loop` 查看布局 → 绘制一帧 + dirty 增量 + 命中测试。

## 设计要点

- **纯逻辑核心**：不持有平台窗口，绘制产物是 `render.Scene`，可交给任意 Backend
  （测试用 RecordingBackend）。平台窗口与事件源后续作为 backend 接入，平台库隐藏在实现
  内部，不污染上层接口（依赖规则第 7 条）。
- 不感知具体业务组件，不依赖 widgets / app。
- 几何计算委托给具体节点 + layout 求解器，runtime 不写死布局算法。
- dirty 分两级：`layout_dirty`（需重新 measure+layout）与 `paint_dirty`（仅重绘），
  都向上冒泡到根，`frame()` 据此决定增量出帧。

## 状态

✅ 已落地，含单元测试（树结构 / deinit / measure 约束 / dirty 冒泡 / 命中测试 /
frame 布局+绘制 / 增量出帧 / 视口重布局 / 事件冒泡 / setRoot 替换）。运行 `zig build test` 验证。
