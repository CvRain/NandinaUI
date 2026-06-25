# showcase

NandinaUI 的可视化画廊，作为绑定边界的验收用例。两种前端各跑一份等价页面，证明 Zig 与 C++ 两条绑定能力对齐。

## 运行

```sh
zig build run      # Zig 前端可视化画廊（SDL3 窗口，消费 frontend/zig/nandina.zig）
zig build run-cpp  # C++ 前端可视化画廊（SDL3 窗口，消费 frontend/cpp/nandina.hpp）
zig build test     # 运行全部单元测试
```

## 结构

```
showcase/
├── zig/
│   ├── main.zig            # Zig 前端入口（App + PageHost + 主循环）
│   └── gui_pages.zig       # 页面构建（Overview/Widgets/Layout/Reactive/Theme）
├── cpp/
│   └── main.cpp            # C++ 前端入口（同一组页面，C ABI 之上）
└── README.md
```

## 页面

5 个页面覆盖主要能力：

| 页面     | 内容                                                |
| -------- | --------------------------------------------------- |
| Overview | 简介与迁移进度                                      |
| Widgets  | 常见组件展示（Button/Icon/TextField/Checkbox/Switch）|
| Layout   | 布局组件展示（Column/Row/Stack/Panel/Card）         |
| Reactive | 响应式信号展示                                      |
| Theme    | 主题系统展示                                        |

Zig 版与 C++ 版展示同一组页面，便于对照验证。

## 设计原则

- **不走绑定边界就不算数**。showcase 代码只消费前端层（`@import("nandina")` 或 `#include <nandina/nandina.hpp>`），
  不直接 `@import("NandinaUI")` 调用 Core 内部 API。
- **双前端对齐**。Zig 与 C++ 两个版本的页面结构、内容尽量保持一致。
- **逻辑验证交给单元测试**。不再保留命令行文本 demo，所有功能覆盖通过 `zig build test` 保障。
