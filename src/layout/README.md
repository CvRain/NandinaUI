# layout

布局系统层。**仅依赖 foundation**。提供协议层 `Constraints` 与三套纯函数布局求解器。

## API

| 模块 | 用途 | 对标 |
|------|------|------|
| `constraints` | `Constraints`（min/max width/height）协议层核心类型 | Flutter BoxConstraints |
| `flex` | 盒子模型：`column` / `row` / `stack`，含 flex 权重 / min-max / align / padding / gap | Qt QVBox/QHBox/QStacked |
| `flow` | 流式折行：超宽自动换行，flex / grid 的基础 | 前端 wrap 布局 |
| `anchors` | 锚点定位：left/top/right/bottom/width/height 每轴独立解析 | QML anchors |

所有求解器都是**纯函数**：输入容器 bounds + 子节点规格，输出每个子节点的最终矩形
（`out_frames` 由调用方提供，不分配内存、不依赖 widget 树），便于独立测试。

## 用法

```zig
const layout = @import("NandinaUI").layout;

// 盒子模型：header(固定) / 内容(flex) / footer(固定)
const children = [_]layout.flex.ChildSpec{
    .{ .preferred = .{ .width = 0, .height = 40 } },
    .{ .flex = 1 },
    .{ .preferred = .{ .width = 0, .height = 30 } },
};
var frames: [3]foundation.Rect = undefined;
layout.flex.solve(
    .{ .axis = .column, .gap = 8, .cross_align = .stretch, .padding = .all(10) },
    container_bounds, &children, &frames,
);

// 流式折行
layout.flow.solve(.{ .inline_gap = 8, .line_gap = 8 }, bounds, &flow_children, &flow_frames);

// 锚点：顶部满宽条
const specs = [_]layout.anchors.AnchorSpec{
    .{ .horizontal = .{ .start = 0, .end = 0 }, .vertical = .{ .start = 0, .size = 36 } },
};
layout.anchors.solve(bounds, &specs, &anchor_frames);
```

可运行 `zig build showcase -- layout-box` 查看三套求解器输出与「布局 → 渲染」链路。

## 三层模型

1. **语义层**：未来由 widgets 封装的 `Row` / `Column` / `Stack` / `Flow` / `Positioned` 等原语。
2. **协议层**：`Constraints` / preferred size / 求解器输入输出边界（核心，必须稳定）。
3. **求解层**：当前的 `flex` / `flow` / `anchors` 自有轻量求解；未来复杂 flex / grid
   可接入 Yoga 作为可插拔后端，不改变上层语义。

## 设计要点

- 几何计算只发生在本层内部，**禁止泄漏到 widgets / app 业务代码**。
- 当前不 Yoga-first；先稳定协议层与自有求解器，为 Yoga 预留接入位。
- 锚点的动态值（如「兄弟节点底边 + 8」）由上层 / 响应式在调用前算成数值传入，
  本层只做纯几何解析。

## 状态

✅ 已落地，含单元测试（constraints / flex column·row·stack·flex 权重·对齐·padding /
flow 折行 / anchors 各轴组合）。运行 `zig build test` 验证。
详见 [布局策略](../../docs/development/layout-strategy.md)。
