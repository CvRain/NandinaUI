# render

渲染抽象层。定义绘制中间表示与后端接口，**依赖 foundation**。

## 计划 API

- `DrawCommand`：绘制命令联合体（`FillRect` / `FillRoundedRect` / `DrawText` / `PushClip` / `PopClip`）。
- `Scene`：绘制命令缓冲，上层只往里追加命令。
- `Backend`：渲染后端接口（vtable），由具体后端（ThorVG / 自绘等）实现。

## 设计要点

- 上层只产出绘制命令，不直接调用平台绘制 API。
- `PushClip` / `PopClip` 服务于 Dialog / Popover / Card / ScrollView 等容器的子内容裁剪。
- 命令缓冲优先复用 buffer，避免每帧重建分配。
- 提供一个内存 backend（记录命令序列）用于单元测试断言。

## 状态

🚧 骨架。见 [架构](../../docs/development/architecture.md) 与 [重写路线图](../../docs/development/roadmap.md) M2。
