# 项目演进与当前形态

现在这套 API 不是一次设计出来的。设计令牌、`primitives` + `token` 的组件方式、signal 响应式模型、分层约束和 authoring 风格，都是前两代验证过之后再带进当前主线的；而 QML 绑定、Zig Core、C ABI 这些试验则被有意放弃。了解这段来源，可以避免把已经验证过的约定重新推翻，也能分清哪些是“验证过的共识”、哪些是当前主线的新选择。

## 四段形态

| 阶段 | 形态 | 归档位置 |
| --- | --- | --- |
| v1 | Qt/QML 组件库（CMake + `qmldir` + `Nandina/` + `public/`） | 分支 `archive-0.0.1-qml` |
| v1.5 | C++ 分层重写：`foundation` / `layout` / `reactive` / `theme` / `app` / `bindings` | 分支 `archive-0.0.2` |
| v2 | Zig Core + C ABI（`build.zig` + `src` / `frontend` / `showcase`） | 分支 `archive-0.0.3` |
| v3 | C++26 + raylib，Godot 式 SceneTree 内核 | 当前 `main` |

这四个位置都是历史存档，不再维护；正史只存在于 `main`。

## 每次转向的原因

**v1（QML）** 验证了主题、语义命名和组件组合方式确实好用，问题在于它把框架绑死在 QML 的绑定机制与控件生态上，运行时抽象和跨语言能力都施展不开，继续在旧结构上迭代只会放大技术债。

**v1.5 / v2（C++ 分层 → Zig + C ABI）** 想把“组件库”提升为「Core + Bindings」：核心只写一次，通过 C ABI 导出，各语言绑定层再做惯用封装。方向是对的，Zig 的显式所有权也确实让边界更清楚，但工具链与 ABI 边界引入的复杂度超过了当时的收益。

**v3（当前）** 回到 C++，把底层做成命令式场景树内核（SceneTree / Node / 生命周期 / 事件 / hit-test），再在它之上重建声明式、响应式的开发体验。目标是底层像游戏引擎一样可控，写起来像前端框架一样自然。

## 迁移到正式仓库

- 实验仓库在 `47daa63` 完成双设备 Linux 手工验收，完整测试 45/45。
- 正式仓库用 `0edd24e` 把该实验树接入 `main`，旧 `main` 固定到 `archive-0.0.3`。
- 随后 `31ecc81` 收口官方项目标识，`ed31258` 移除归档示例测试，`1aab75e` 接入本地 playground 开发流。
- v3 的开发参考文档在此之后逐步补齐：组件公共契约、组件路线图、浮层架构、Tooltip 迁移契约、模块依赖规则等。

## 三代之间没有变的东西

- **Design-system-first**：先定 token / theme / 语义 API，再写具体组件。
- **组合优于继承**：用 primitives 拼装，而不是拉深继承树。
- **语义 API 优先**：接口表达业务意图，不把实现细节暴露给应用。
- **现代响应式**：Angular 风格的 `signal` / `computed` / `effect`，而不是手写刷新。
- **分层与单向依赖**：见 [模块依赖规则](module_dependency.md)。

## 当前形态

当前 `main` 由 12 个模块组成（自底向上）：`foundation`、`reactive`、`resource`、`physics2d`、`theme`、`render`、`text`、`scene`、`animation`、`semantics`、`widget`、`app`。模块职责与依赖方向记录在 [模块依赖规则](module_dependency.md)，各模块的分层示意见 [项目介绍](../getting_started/1_overview.md)。

文档按读者目的分三条路线：`getting_started/` 是应用开发者的学习路径，`components/` 是单个组件的公开使用参考，`references/` 是维护者的设计与流程规则。

## 读旧文档时的注意事项

- `dev-docs-v1` 属于 C++/QML 时代，里面有 C++20 modules、SDL3/ThorVG、`runtime` / `layout` / `showcase` 等已经不存在或改名的模块——只作**语义参照**。
- `dev-docs-v2` 属于 Zig + C ABI 时代，分配器、error union、`pub const` 等写法与当前无关，只作**分层与模型参照**。
- `dev-docs-v3` 是当前主线的阶段记录，仍然可读；但其中 `readme.md` 是对话记录，不是设计文档。
- 判断旧结论是否仍成立，一律以当前 `nandina/` 源码为准：命名、枚举、模块边界都可能已经变了。

实验仓库 `nandina_experiment` 的主线已清空、转型为示例展示平台，历史保留在 `dev-docs-*` 与归档分支里。
