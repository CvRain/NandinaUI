# widgets

组件库层。**依赖 foundation / reactive / layout / theme / text / runtime**。
对齐 shadcn 的 primitives / composition：先源语，再控件，组合优于继承。

## 计划 API

### primitives（底层积木，单一维度能力）
- `Surface`：结构与视觉容器（背景/圆角/描边/padding）。
- `Pressable`：纯交互状态机（hover/pressed/focused/disabled）。
- `Text`：文本能力（建立在 text 层之上）。
- `FocusRing`：焦点可视化。

### controls（面向页面作者的真实控件）
- `Label` / `Button` / `Panel` / `Card` …，由 primitives 组合而成。

## 设计要点

```
Control = 语义 API + primitive 组合 + 语义状态到样式的映射
```

- 控件接收只读输入（`Prop` / 只读 signal），内部状态用 `Signal` 并绑定 `EffectScope`。
- 不在控件内部硬写本可复用的 primitive 能力。
- 视觉默认值来自 theme token，而非控件私有常量。

## 状态

🚧 骨架。边界规则见 [组件 Primitives](../../docs/development/widget-primitives.md)。
