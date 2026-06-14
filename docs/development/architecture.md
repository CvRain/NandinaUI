# NandinaUI 架构（Zig 重写）

> 本文是 Zig 重写版的架构基线。旧 C++ 主线已归档在 `archive/`，其设计文档仍作为语义参照。

## 项目定位

NandinaUI 是一个「可扩展 UI runtime + 设计系统驱动的 widgets + 应用 authoring 层」。
项目初衷是创建一个**接近 Web App 的丝滑流畅开发体验**：上手简单、组件丰富、主题易变。
Zig 重写的目标是保留旧版已验证的核心思想（分层边界、响应式模型、design-system-first），
同时用 Zig 的显式分配器、comptime 与简洁所有权模型重建内核。

## 设计参照系

- **组件编写语法**：Flutter 式声明 + 链式组合。
- **响应式与应用结构**：Angular 的 signal / computed / effect、page、router。
- **组件与样式体系**：shadcn 的 primitives 全局源语 + 通用设计 token，功能拆成最小 node，新组件不断复用。
- **底层机制借鉴**：EUI-NED 的文字渲染、slint 的组件挂载、Qt QML 的组件设计方式。

## 设计哲学

- **Design-system-first**：先 token / theme / 语义 API，再具体 widget。
- **组合优于继承**：用原语拼装，避免深继承组件树。
- **语义 API 优先**：接口表达业务语义，不暴露实现细节。
- **现代响应式**：围绕 Angular 风格的 signal / computed / effect 建设数据驱动更新。
- **显式资源管理**：遵循 Zig 习惯，分配器显式传入，所有权清晰、可测。

## 分层结构

源码位于 `src/<layer>/<layer>.zig`，由 `src/root.zig` 聚合再导出。

```
foundation                         （几何 / 颜色 / 基础类型，无上游依赖）
   │
   ├── reactive   （signal / computed / effect / linkedSignal / batch）
   ├── render     （Scene / DrawCommand / Backend 接口）
   ├── layout     （Constraints / measure-layout / Row·Column·Stack）
   ├── theme      （token / palette / Theme resolver）
   ├── text       （Font / measureText / text layout）
   │
   └── runtime    （Node 树 / 事件 / 主循环；依赖 reactive·render·layout）
          │
          widgets （primitives + controls；依赖 reactive·layout·theme·text·runtime）
          │
          app     （Page / Router / 挂载入口；依赖 widgets）
```

### 依赖方向（单向，禁止反向）

`foundation → {reactive, render, layout, theme, text} → runtime → widgets → app`

- `app` 不得反向侵入 `runtime` 内核。
- 任何层都不得依赖比自己更高的层。

## 各层职责

| 层 | 职责 | 当前状态 |
|------|------|----------|
| `foundation` | 几何（Point/Size/Rect/Insets）、颜色 | ✅ 已落地，有测试 |
| `reactive` | signal / computed / effect 与依赖追踪、batch 调度 | ✅ 已落地，有测试 |
| `render` | Scene / DrawCommand 中间层与后端接口 | ✅ 已落地，有测试 |
| `layout` | constraints / measure-layout 协议与基础容器 | ✅ 已落地，有测试 |
| `theme` | token / palette / Theme resolver | ✅ 已落地，有测试 |
| `text` | 字体、文本测量与布局（溢出策略） | ✅ 已落地，有测试 |
| `runtime` | Node 树、事件循环、调度边界 | ✅ 纯逻辑核心已落地（平台后端待接） |
| `widgets` | primitives 与 controls | ✅ 首批已落地，有测试 |
| `app` | page / router / 挂载入口 | 🚧 骨架 |

## 稳定边界（优先建立）

1. runtime 与 reactive 的调度边界。
2. render 与具体后端的抽象边界（DrawCommand / Backend 接口）。
3. theme/token 与 widgets 的接口边界。
4. app authoring 与 widgets 的语义 API 边界。
5. layout 语义层、协议层与未来求解层（Yoga 等）的边界。

## 构建与测试

```sh
zig build run      # 运行 showcase 烟雾程序
zig build test     # 运行全部分层单元测试
```

`root.zig` 与每个层文件都用 `std.testing.refAllDecls(@This())` 逐层引用子模块，
确保 `zig build test` 能递归收集所有 `test` 块。
