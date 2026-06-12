# NandinaUI

> 一个追求「接近 Web App 的丝滑开发体验」的 Zig UI 库：上手简单、组件丰富、主题易变。

NandinaUI 把现代前端的成熟范式带到原生 UI 开发：

- **Flutter 式组件编写语法** —— 声明式、可链式组合，描述结构而非几何坐标。
- **Angular 式响应式（signal）** —— 以 `signal` / `computed` / `effect` 驱动数据更新；并吸收 Angular 的 page / router 机制。
- **shadcn 式设计体系** —— primitives 全局组件源语 + 通用设计 token，把功能拆成最小 node，新组件不断复用。
- **借鉴** EUI-NED 的文字渲染机制、slint 的组件挂载机制、Qt QML 的组件设计方式。

> 状态：Zig 重写早期。`foundation` 已落地并有测试，其余层为带路线图的骨架。
> 旧 C++/QML 主线已归档在 [`archive/`](archive/)，其设计文档作为语义参照保留。

## 快速开始

需要 Zig 0.16.0。

```sh
zig build run      # 运行 showcase 烟雾程序
zig build showcase # 运行组件 / 能力演示（zig build showcase -- list 查看全部）
zig build test     # 运行全部分层单元测试
```

更完整的入门见 [使用指南 · 快速开始](docs/guide/getting-started.md)。

## 分层结构

源码位于 `src/<layer>/<layer>.zig`，由 `src/root.zig` 聚合再导出，遵循单向依赖：

```
foundation → {reactive, render, layout, theme, text} → runtime → widgets → app
```

| 层 | 职责 | 状态 |
|------|------|------|
| `foundation` | 几何（Point/Size/Rect/Insets）、颜色 | ✅ 已落地 |
| `reactive` | signal / computed / effect / 依赖追踪 / batch | ✅ 已落地 |
| `render` | Scene / DrawCommand / Backend 接口 | 🚧 骨架 |
| `layout` | constraints / measure-layout / Row·Column·Stack | 🚧 骨架 |
| `theme` | design token / palette / Theme resolver | 🚧 骨架 |
| `text` | 字体、文本测量与布局 | 🚧 骨架 |
| `runtime` | Node 树、事件循环、调度边界 | 🚧 骨架 |
| `widgets` | primitives + controls | 🚧 骨架 |
| `app` | page / router / 挂载入口 | 🚧 骨架 |

## 文档

文档分为两条线，入口见 [`docs/README.md`](docs/README.md)：

- **开发文档** [`docs/development/`](docs/development/) —— 面向贡献者：架构、依赖规则、各子系统设计规范、路线图。
- **使用参考文档** [`docs/guide/`](docs/guide/) —— 面向库使用者：快速开始、核心概念、主题定制。
- **就近使用说明** —— 每个 `src/<layer>/README.md` 说明该层/组件如何使用，便于在代码旁查看。

## 许可证

见仓库 LICENSE（如有）。
