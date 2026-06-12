# text

文本能力层。**依赖 foundation**；测量结果供 layout / render 使用。借鉴 EUI-NED 的文字渲染机制。

## 计划 API

- `Font`：字体句柄抽象（字体族、字重、字号）。
- `measureText(str, font) -> Size`：文本测量，供 layout 求 preferred size。
- text layout：换行、对齐、line height、overflow。

## 设计要点

- 只负责文本内容、度量与绘制；不含点击/状态机/容器边框等语义。
- widgets 层的 Text primitive 建立在本层之上；Label 是 Text 的语义包装。

## 状态

🚧 骨架。见 [组件 Primitives · Text](../../docs/development/widget-primitives.md)。
