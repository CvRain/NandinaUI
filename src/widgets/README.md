# widgets

组件库层。**依赖 foundation / reactive / layout / theme / text / runtime**。
对齐 shadcn 的 primitives / composition：先源语，再控件，组合优于继承。

## API

### primitives（底层积木，单一维度能力）
- ✅ `Surface`：结构与视觉容器（背景 / 圆角 / 描边 / padding）。
- ✅ `Pressable`：纯交互状态机（hover / pressed / focused / disabled）+ 点击回调。
- ✅ `ClipNode`：通用子树裁剪容器（不可见，声明裁剪区域，裁剪由 runtime 统一执行）。
- 🚧 `FocusRing`：焦点可视化（待落地）。

### controls（面向页面作者的真实控件）
- ✅ `Label`：响应式文本标签（建立在 text 层之上，含测量缓存 / 失效）。
- ✅ `Button`：可点击按钮（背景随 hover / pressed / disabled 状态变化 + 文本）。
- ✅ `Panel`：带圆角 / 边框 / padding 的内容面板。
- ✅ `Card`：带 title / description header 的结构化容器。

每个组件通过 `XxxProps`（一组 `ReadSignal`）构造，`create(allocator, graph, props)` 返回
`*Xxx`，用 `node.deinitTree(allocator)` 释放。

## 设计要点

```
Control = 语义 API + primitive 组合 + 语义状态到样式的映射
```

- 控件接收只读输入（`Prop` / 只读 signal），内部状态用 `Signal` 并绑定 `EffectScope`。
- 不在控件内部硬写本可复用的 primitive 能力。
- 视觉默认值来自 theme token，而非控件私有常量。
- **裁剪不重复造轮子**：组件不自己做像素裁剪。需要裁剪子内容时用 `ClipNode` 包裹，
  或节点通过 `runtime.Node.VTable.child_clip` 声明裁剪区域，runtime 的绘制遍历统一 push/pop
  （吸收 archive 教训：曾为每个组件单独设 clip）。

## 状态

✅ M5 primitives（Surface / Pressable / ClipNode）与首批 controls（Label / Button / Panel / Card）
已落地，含单元测试。运行 `zig build test` 验证，或 `zig build showcase -- widgets-gallery`
查看全链路演示。边界规则见 [组件 Primitives](../../docs/development/widget-primitives.md)。
