# runtime

UI 运行时层。**依赖 foundation / reactive / render / layout**。借鉴 slint 的组件挂载机制。

## 计划 API

- `Node` 树：parent / children，owning 子节点 + handle 访问。
- `Event`：pointer / key 事件类型与分发。
- 主循环骨架：`markDirty → reflow → repaint`。
- `Window` / 平台后端边界（平台库隐藏在实现内部）。

## 设计要点

- 是 widgets 与具体平台窗口之间的运行时基座。
- 不感知具体业务组件，不依赖 widgets / app。
- 平台/绘制库通过接口解耦，不污染上层。

## 状态

🚧 骨架。见 [架构](../../docs/development/architecture.md) 与 [重写路线图](../../docs/development/roadmap.md) M4。
