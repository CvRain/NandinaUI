# NandinaUI 细颗粒度开发 Issue 清单

> 说明：
> - 本清单面向正式项目主线开发，按模块拆分为可独立推进的细颗粒度 issue。
> - 默认假设当前阶段以 **C++ authoring + 文档驱动开发 + showcase 驱动验证** 为主。
> - 不要求一次性全部完成，建议按优先级和依赖顺序逐步创建。
> - 每个 issue 都尽量包含：目标、范围、产出、完成定义。
> - 本文档的 Milestone 拆分比 `roadmap.md` 更细；`roadmap.md` 用于表达聚合阶段顺序，本清单用于表达可执行 issue 与模块收口顺序。
> - 推荐使用标签：
    >   - `area:foundation`
>   - `area:runtime`
>   - `area:reactive`
>   - `area:layout`
>   - `area:render`
>   - `area:text`
>   - `area:theme`
>   - `area:widgets`
>   - `area:app`
>   - 
>   - `area:showcase`
>   - `area:docs`
>   - `area:tests`
>   - `priority:p0`
>   - `priority:p1`
>   - `priority:p2`
>   - `kind:architecture`
>   - `kind:implementation`
>   - `kind:refactor`
>   - `kind:docs`
>   - `kind:test`
>
> 状态校正（2026-05）：本清单是持续更新的活文档，不再代表“从零创建项目”的原始开工顺序。历史 issue 与已完成记录保留用于追踪演进；当前实际优先级应以下文的收口型 issue 和源码/测试状态为准。

---

# Milestone M0 — 文档与工程基线

## Issue 001 — 建立正式项目目录骨架 ✅ 已完成
**Labels:** `area:docs`, `kind:architecture`, `priority:p0`

### 目标
建立正式项目的顶层目录结构，为后续模块落位提供统一规范。

### 范围
创建并约定以下目录：
- `foundation/`
- `runtime/`
- `reactive/`
- `layout/`
- `render/`
- `text/`
- `theme/`
- `widgets/`
- `app/`
- `showcase/`
- `tests/`
- `docs/`

### 产出
- 顶层目录骨架
- 必要的占位说明文件（如 `.gitkeep` 或 `README.md`）

### 完成定义
- 仓库顶层结构与文档中规划一致
- 后续代码模块可直接落位，不再临时决定目录

### 完成记录
- 所有顶层目录已创建
- 提交: `adbc185 feat(framework): complete the establishment of the basic structure of the project and the basic configuration.`

---

## Issue 002 — 编写模块依赖方向约束文档 ✅ 已完成
**Labels:** `area:docs`, `kind:docs`, `priority:p0`

### 目标
明确模块之间的依赖方向，避免后续开发出现循环依赖和层级污染。

### 范围
文档需说明：
- `foundation` 不依赖高层
- `runtime` 不依赖 widgets/app
- `widgets` 依赖 runtime/reactive/layout/theme
- `app` 依赖 widgets/runtime，不反向污染底层
- `showcase` 仅作为验证层，不成为 runtime 的前置依赖

### 产出
- `docs/module-dependency-rules.md` 或并入 architecture 文档

### 完成定义
- 所有后续 issue 可依据该文档判断“是否放错层”

### 完成记录
- 文档 `docs/module-dependency-rules.md` 已编写完成
- 提交: `ac9d31f`

---

## Issue 003 — 定义命名规范与 public/internal API 约定 ✅ 已完成
**Labels:** `area:docs`, `kind:docs`, `priority:p0`

### 目标
统一命名、目录、��名空间和 API 暴露策略。

### 范围
包括但不限于：
- 类型命名风格
- 文件命名风格
- 命名空间组织方式
- internal/private 边界
- 实验性 API 标记方式
- 示例代码是否视为 API 契约的一部分

### 产出
- `docs/coding-and-api-conventions.md`

### 完成定义
- 团队/个人后续实现不再靠临时习惯决定命名

### 完成记录
- 文档 `docs/coding-and-api-conventions.md` 已编写完成
- 提交: `c13c529`

---

## Issue 004 — 建立初始开发文档导航页 ✅ 已完成
**Labels:** `area:docs`, `kind:docs`, `priority:p1`

### 目标
建立统一的 docs 导航，让 README 不必承担所有说明职责。

### 范围
整理已有：
- 项目方向
- 架构规划
- 仓库结构
- 路线图
- QML 历史迁移说明

### 产出
- `docs/index.md`

### 完成定义
- README 能通过 docs 导航快速跳转到关键文档

### 完成记录
- `docs/index.md` 已创建，按主题分类整理所有技术文档
- 更新 `README.md` 添加导航页链接
- 更新 `develop-issue.md` 添加已完成 Issue 记录章节
- 提交: `abe263e`

---

# Milestone M1 — Foundation 与 Runtime MVP

## Issue 005 — 定义 foundation 层基础枚举与通用类型 ✅ 已完成
**Labels:** `area:foundation`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — 基础枚举已落地于 `foundation/src/nan_types.cppm`

### 目标
实现跨模块共享的基础类型与枚举，避免各层重复定义。

### 范围
建议先包含：
- `Axis`
- `Align`
- `Justify`
- `Visibility`
- `PointerButton`
- `KeyCode`
- `MouseCursor`
- `Direction`

### 产出
- `foundation/types` 下的基础头文件/模块

### 完成定义
- layout/runtime/widgets 不再自行定义重复枚举

### 完成记录
- 已实现 `Axis`、`Align`、`Justify`、`Visibility`、`PointerButton`、`KeyCode`、`MouseCursor`、`Direction`
- 落地文件：`foundation/src/nan_types.cppm`
- 当前 runtime / widgets 已通过导入该模块复用基础枚举

---

## Issue 006 — 实现基础几何类型 Point / Size / Rect / Insets ✅ 已完成
**Labels:** `area:foundation`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — 几何类型与测试均已落地

### 目标
建立 UI 系统统一的几何基础类型。

### 范围
实现：
- `Point`
- `Size`
- `Rect`
- `Insets`

建议支持：
- contains
- intersect/intersects
- inflate/deflate
- clamp basic helpers

### 产出
- `foundation/geometry`

### 完成定义
- Widget bounds 与 Layout request 均可基于该几何层表达

### 完成记录
- 已实现 `NanPoint`、`NanSize`、`NanRect`、`NanInsets`
- 支持 `contains`、`intersects`、`inflate/deflate` 等基础几何能力
- 落地文件：`foundation/src/nan_point.cppm`、`foundation/src/nan_size.cppm`、`foundation/src/nan_rect.cppm`、`foundation/src/nan_insets.cppm`
- 测试：`tests/foundation/test_geometry.cpp`

---

## Issue 007 — 增加 Constraints 类型并定义测量语义 ✅ 已完成
**Labels:** `area:foundation`, `area:layout`, `kind:architecture`, `priority:p0`
**Status:** ✅ 已完成 — `NanConstraints` 与测试已落地

### 目标
为后续布局与 widget 测量机制建立统一约束模型。

### 范围
定义：
- min/max width/height
- unbounded 语义
- tight/loose constraints

### 产出
- `Constraints` 类型
- 测量语义说明文档

### 完成定义
- 后续 layout/core 可直接依赖，无需返工抽象

### 完成记录
- 已实现 tight / loose / unbounded / constrain / intersect 等约束语义
- 落地文件：`foundation/src/nan_constraints.cppm`
- 测试：`tests/foundation/test_constraints.cpp`

---

## Issue 008 — 设计并实现 Application 生命周期最小接口 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `NanApplication` 与 `NanAppWindow` 已形成最小应用入口

### 目标
建立统一应用入口与生命周期管理。

### 范围
建议包括：
- 初始化
- 主循环
- 关闭流程
- 主线程任务投递接口占位

### 产出
- `runtime/app`

### 完成定义
- 可以创建 Application 并驱动窗口/帧循环

### 完成记录
- 已提供 `NanApplication`、`NanAppWindow`、`NanComponent`
- `NanApplication::run()` 可持有并驱动应用窗口
- `NanAppWindow::BridgeWindow` 已接通 ready / update / draw / resize 与基础指针事件桥接
- 落地文件：`app/src/nan_application.cppm`

---

## Issue 009 — 设计 Window 最小接口 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `NanWindow` 已提供平台无关窗口接口与 Builder

### 目标
建立平台无关的 Window 抽象基础。

### 范围
包括：
- create/destroy
- title
- width/height
- resize event
- close event
- dpi/scale 基础能力占位

### 产出
- `runtime/window` 基础接口

### 完成定义
- 上层不直接耦合底层平台窗口实现细节

### 完成记录
- 已提供 `NanWindow::Config`、`NanWindow::Builder`、PIMPL 隔离的窗口接口
- 已覆盖 title / width / height / dpi_scale / resize / close 生命周期钩子
- SDL 类型已隐藏在 `runtime/src/nan_window.cpp` 实现单元中
- 导出接口：`runtime/src/nan_window.cppm`

---

## Issue 010 — 打通窗口事件循环到 runtime 事件队列 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — SDL 事件已翻译并进入 runtime 窗口事件回调

### 目标
让平台事件可以统一进入 runtime 内部事件系统。

### 范围
- 平台事件 -> runtime event translation
- 输入事件排队或直接分发策略的第一版
- window close / resize 接入

### 产出
- event translation 基础实现

### 完成定义
- 可以从窗口收到用户输入和关闭事件

### 完成记录
- `NanWindow::poll_events()` 已翻译 SDL 的 close / resize / pointer / wheel / key / text input 事件
- 关闭与 resize 已接入 `should_close`、`on_close_requested()`、`on_resize()`
- 落地文件：`runtime/src/nan_window.cpp`

---

## Issue 011 — 定义统一 Event 类型体系 ✅
**Labels:** `area:runtime`, `kind:architecture`, `priority:p0`
**Status:** ✅ 已完成 — 提交 `a0e78c7`

### 目标
统一描述鼠标、键盘、文本、窗口等事件。

### 范围
覆盖：
- PointerMove / PointerDown / PointerUp / Click / Wheel
- KeyDown / KeyUp / TextInput
- FocusIn / FocusOut
- WindowResize / WindowClose

### 产出
- `runtime/src/nan_event.cppm` — 统一 Event 类型体系（EventType 枚举、事件结构体、Event 变体、event_type() 提取函数、to_string()）
- `runtime/src/nan_widget.cppm` — 增加 Widget 级事件处理接口（dispatch_event/bubble_event + on_xxx 虚方法）
- `runtime/src/nan_window.cppm` — 迁移至新 Event 类型，移除重复定义
- `docs/event-system.md` — 事件系统设计文档
- `tests/runtime/test_event.cpp` — 单元测试（枚举/结构体/变体/round-trip）

### 完成定义
- ✅ 事件类型统一，Widget 可用同一套事件类型
- ✅ Widget 支持事件分发（dispatch → handle → on_xxx）
- ✅ Window 消费同一套事件类型
- ✅ 文档与单元测试完备

---

## Issue 012 — 实现 Widget 树基础结构 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `NanWidget` 已具备树结构、遍历与基础生命周期语义

### 目标
实现最基础的 widget tree 结构。

### 范围
包括：
- parent / child
- add/remove child
- 遍历
- 生命周期基础约束
- visible / enabled 基础属性

### 产出
- `runtime/widget`

### 完成定义
- 可以构建一棵 UI 树并遍历

### 完成记录
- 已实现 parent / child / `add_child()` / `clear_children()` / `for_each_child()` / `child_count()`
- `draw()` 已递归遍历子树，`preferred_size()` / `set_bounds()` 已提供基础布局协议
- 落地文件：`runtime/src/nan_widget.cppm`

---

## Issue 013 — 为 Widget 增加 bounds、hit-test-visible、dirty 状态 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p0`

### 目标
让 Widget 初步具备布局、命中测试和重绘请求能力。

### 范围
包括：
- `bounds`
- `hit_test_visible`
- `mark_dirty`
- dirty upward propagation

### 产出
- Widget 核心状态字段和最小行为

### 完成定义
- ✅ 修改 widget 状态可触发脏标记传播
- ✅ 测试: `test_bounds_hit_test.cpp` — 24 个测试全部通过

---

## Issue 014 — 实现基础 hit test 机制 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p1`

### 目标
支持根据坐标查找命中的 widget。

### 范围
- children 遍历顺序约定
- basic rect hit testing
- 忽略 invisible / disabled / hit-test-hidden 节点策略

### 产出
- runtime hit test 基础逻辑

### 完成定义
- ✅ 点击时能够定位一个目标 widget
- ✅ 测试: `test_bounds_hit_test.cpp` — 9 个 HitTest 测试全部通过

---

## Issue 015 — 建立 Widget 事件分发最小通路 ✅ 已完成
**Labels:** `area:runtime`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — Widget 命中测试与事件投递已形成最小闭环

### 目标
让命中测试结果可以驱动 widget 收到事件。

### 范围
- 目标节点事件投递
- bubbling/capturing 第一版策略选择
- stop propagation 是否先留占位

### 产出
- 事件分发最小闭环

### 完成定义
- 某个 widget 能响应点击等基础事件

### 完成记录
- `NanWidget` 已实现 `dispatch_event()` 与 `bubble_event()`，并暴露 `on_pointer_xxx` / `on_key_xxx` 等虚方法
- `NanAppWindow::BridgeWindow` 已通过 `hit_test()` 将指针事件投递到命中节点
- `Pressable`、`Button`、`SidebarMenuButton` 等组件已消费该事件通路

---

## Issue 016 — 设计 Scene/DrawCommand 中间层 ❌ 未完成
**Labels:** `area:runtime`, `area:render`, `kind:architecture`, `priority:p0`
**Status:** ❌ 未完成 — 主线仍未建立 Scene/DrawCommand 中间层

### 目标
建立 widget 到 renderer 之间的中间表示，避免 widget 直接耦合具体后端。

### 范围
建议定义：
- `DrawRect`
- `DrawRoundedRect`
- `DrawText`
- `PushClip`
- `PopClip`

### 产出
- `runtime/scene` 或 `render/scene`

### 完成定义
- render backend 只消费 draw commands，不理解 widget 高层语义

### 当前判断
- `NanWidget` 仍直接面向 `tvg::SwCanvas` 绘制
- `NanWindow` 的绘制回调也仍直接暴露 ThorVG canvas
- `render/` 模块尚未落地 commands / adapter 的真实实现

---

## Issue 017 — 实现首个固定节点渲染闭环 ✅ 已完成
**Labels:** `area:runtime`, `area:render`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — 已打通 window -> widget draw -> ThorVG/SDL 呈现闭环

### 目标
从 window -> widget tree -> scene -> render backend 打通第一条可视化路径。

### 范围
- 固定背景/矩形节点
- 每帧提交 draw commands
- window present

### 产出
- 首个可运行最小 UI 画面

### 完成定义
- 可以显示基础图形，不只是窗口空白

### 完成记录
- `NanWindow::present_frame()` 已完成 ThorVG `SwCanvas` 绘制、光栅化、SDL 纹理上传与 present
- `NanWindow::run()` 已将事件轮询、update 与 present 串为主循环
- showcase 已作为真实运行环境验证该闭环

---

# Milestone M2 — Reactive 模块

## Issue 018 — 定义 reactive 模块设计说明 ✅ 已完成
**Labels:** `area:reactive`, `kind:docs`, `priority:p0`
**Status:** ✅ 已完成 — reactive 模块设计说明已落地

### 目标
在实现前明确 reactive 设计边界，避免后续反复改概念。

### 范围
说明：
- `State<T>`
- `ReadState<T>`
- `Computed`
- `Effect`
- `EffectScope`
- `Prop<T>`
- `StateList<T>`
- batch 的角色边界

### 产出
- `docs/reactive-strategy.md`

### 完成定义
- reactive 实现时有统一设计依据

### 完成记录
- `docs/reactive-strategy.md` 已覆盖 State / ReadState / Computed / Effect / EffectScope / Prop / StateList / batch 的整体语义
- 文档已纳入 docs 导航，可作为主线设计说明入口

---

## Issue 019 — 实现 State<T> 最小版本 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `State<T>` 与基础订阅能力已落地

### 目标
实现可读写的单值响应式状态容器。

### 范围
- get/read
- set/write
- 变化通知
- 基础观察者机制

### 产出
- `reactive/state`

### 完成定义
- 可通过 State 保存并更新单值状态

### 完成记录
- `reactive/src/state.cppm` 已实现 `State<T>`
- 已支持 `get()`、`set()`、`on_change()` 与只读视图导出

---

## Issue 020 — 实现 ReadState<T> 只读视图 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — `ReadState<T>` 已形成只读订阅视图

### 目标
区分拥有可写状态与对外暴露只读状态。

### 范围
- 从 State 导出 ReadState
- 只读接口
- 生命周期边界说明

### 产出
- `ReadState<T>`

### 完成定义
- 父组件可向子组件传递只读状态

### 完成记录
- `reactive/src/state.cppm` 已实现 `ReadState<T>`
- `tests/reactive/test_reactive.cpp` 已覆盖只读视图复制与追踪行为

---

## Issue 021 — 实现 Effect 自动依赖追踪 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — Effect 自动依赖追踪已落地

### 目标
实现“读取哪些 State，就对哪些 State 建依赖”的 effect 机制。

### 范围
- effect 注册
- 自动追踪上下文
- 状态变更后触发重运行

### 产出
- `reactive/effect`

### 完成定义
- UI 属性可通过 effect 自动同步状态变化

### 完成记录
- `reactive/src/effect.cppm` 已实现 `Effect`
- `reactive/src/tracking.cppm` 已提供依赖追踪上下文与自动恢复
- `tests/reactive/test_reactive.cpp` 已覆盖自动依赖追踪

---

## Issue 022 — 实现 EffectScope 生命周期管理 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `EffectScope` 已具备生命周期管理能力

### 目标
解决组件销毁时 reactive 订阅自动清理的问题。

### 范围
- scope add
- clear/dispose
- 与 component/widget 生命周期的未来集成点

### 产出
- `EffectScope`

### 完成定义
- effect 生命周期可被宿主对象安全管理

### 完成记录
- `reactive/src/effect.cppm` 已实现 `EffectScope`
- 已提供作用域清理与 effect 挂载能力
- `docs/reactive-strategy.md` 已说明其与组件生命周期的集成方向

---

## Issue 023 — 实现 Computed 基础能力 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — `Computed` 已支持依赖追踪与惰性重算

### 目标
支持派生值计算，减少手动同步逻辑。

### 范围
- lazily or eagerly recompute 的策略第一版
- 与 State/Effect 协作

### 产出
- `Computed`

### 完成定义
- 可以从多个状态派生出一个只读值

### 完成记录
- `reactive/src/computed.cppm` 已实现 `Computed`
- `tests/reactive/test_reactive.cpp` 已覆盖派生值与惰性重算

---

## Issue 024 — 为 reactive 增加异常安全与 tracking context 恢复 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — tracking context 恢复与异常安全已补齐

### 目标
防止 effect/computed 中出现异常后破坏线程局部追踪状态。

### 范围
- tracking guard
- 恢复上下文
- basic exception safety

### 产出
- reactive tracking guard 机制

### 完成定义
- 出错后不会让后续 effect 全部异常行为

### 完成记录
- `reactive/src/tracking.cppm` 已通过 RAII 恢复 tracking context
- 已实现异常路径下的 pending observer 恢复
- `tests/reactive/test_reactive.cpp` 已覆盖异常后继续通知的行为

---

## Issue 025 — 实现 Prop<T> 统一属性输入模型 ✅ 已完成
**Labels:** `area:reactive`, `area:widgets`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `Prop<T>` 已实现并被 widgets 广泛消费

### 目标
统一静态值与响应式输入值，减少组件 API 分裂。

### 范围
- static value
- reactive read-only source
- `get()`
- `on_change()`
- `is_reactive()`

### 产出
- `reactive/prop`

### 完成定义
- 组件输入可以优先围绕 Prop 建模

### 完成记录
- 已实现静态值、`ReadState<T>` 绑定、`get()`、`set()`、`on_change()`、`is_reactive()`
- 落地文件：`reactive/src/prop.cppm`
- `Surface`、`Panel`、`Label`、`Card` 等组件已使用 `Prop<T>` 管理输入属性

---

## Issue 026 — 实现 StateList<T> 结构化集合响应式模型 ⚠️ 部分完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — `StateList<T>` 已存在，但细粒度变化对外语义仍不完整

### 目标
支持列表、树等结构化数据的细粒度变更通知。

### 范围
- Inserted
- Removed
- Updated
- Moved
- Reset

### 产出
- `reactive/state_list`

### 完成定义
- 未来 list/tree/router registry 有统一数据基础

### 当前进展
- `reactive/src/state_list.cppm` 已实现 `StateList<T>` 与 `ListChangeKind`
- 已支持集合存取与变更通知

### 未完成部分
- 公开 `on_change` 仍主要暴露整体列表视角，细粒度 `ListChange` 语义没有完整对外开放
- 还不足以证明后续 list/tree/router registry 会直接复用这套结构化差量事件

---

## Issue 027 — 实现 batch(...) 最小批处理能力 ✅ 已完成
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — `batch(...)` 与批量失效刷新已落地

### 目标
合并多个连续状态更新，避免重复触发 effect。

### 范围
- batch scope
- invalidation coalescing
- 第一版不要求复杂事务语义

### 产出
- `reactive/batch`

### 完成定义
- 连续 set 多个 state 时不会产生多次无意义刷新

### 完成记录
- `reactive/src/batch.cppm` 已实现 `batch(...)`
- `reactive/src/tracking.cppm` 已支持 pending invalidation flush
- `tests/reactive/test_reactive.cpp` 已覆盖批处理去重刷新

---

## Issue 028 — 编写 reactive 模块单元测试集合 ⚠️ 部分完成
**Labels:** `area:reactive`, `area:tests`, `kind:test`, `priority:p0`
**Status:** ⚠️ 部分完成 — reactive 主体能力已有测试，但部分边界场景仍未覆盖

### 目标
为 reactive 核心打下稳定测试基础。

### 范围
至少覆盖：
- State set/get
- Effect tracking
- Computed recompute
- Scope clear
- Prop change
- StateList changes
- batch

### 产出
- `tests/reactive/*`

### 完成定义
- reactive 成为最先有较完整测试覆盖的模块

### 当前进展
- `tests/reactive/test_reactive.cpp` 已覆盖 effect / computed / prop / batch / StateList 主体能力
- reactive 测试已接入主测试构建

### 未完成部分
- `EffectScope::clear()` 这类生命周期清理边界仍缺少直接测试
- `StateList` 细粒度事件顺序与语义也未见专门用例

---

# Milestone M3 — Layout 模块

## Issue 029 — 定义 layout 协议与测量/布局职责边界 ⚠️ 部分完成
**Labels:** `area:layout`, `kind:architecture`, `priority:p0`
**Status:** ⚠️ 部分完成 — 文档与基础协议均已存在，但尚未成为全项目唯一布局入口

### 目标
在实现前明确 layout 的输入输出模型。

### 范围
说明：
- measure vs layout
- preferred size
- constraints
- padding/gap/align/justify
- 容器负责什么，子节点负责什么

### 产出
- `docs/layout-strategy.md`

### 完成定义
- 基础容器实现时不再反复改协议

### 当前进展
- `docs/layout-strategy.md` 与 `docs/layout-refactor-plan.md` 已落位
- `runtime/src/nan_widget.cppm` 已提供 `measure()` / `layout()` / `measured_size()` / `mark_layout_dirty()` 的基础协议
- `layout/src/layout_core.cppm` 与 `layout/src/layout_container.cppm` 已固化 padding/gap/align/justify 与基础容器协议

### 未完成部分
- `LayoutContainer` 仍主要走 `preferred_size() + set_bounds() -> layout()` 的即时路径
- widgets 与 showcase 中仍存在较多手工 `set_bounds()` 分发布局，协议尚未真正收口到全链路

---

## Issue 030 — 实现 LayoutNode / LayoutRequest / LayoutResult 基础模型 ⚠️ 部分完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — 已有 `LayoutRequest` 与 backend，但目标模型未完全齐备

### 目标
建立布局系统的统一数据结构。

### 范围
- request
- result
- child specs
- content bounds

### 产出
- `layout/core`

### 完成定义
- Row/Column/Stack 可以共享同一协议

### 当前进展
- `layout/src/layout_core.cppm` 已提供 `LayoutRequest`、`LayoutChildSpec` 与 `BasicLayoutBackend`
- `LayoutRequest` 已显式携带 `NanConstraints`
- `LayoutChildSpec` 已覆盖 `preferred_size`、`min_size`、`max_size`、`flex_factor`、`can_shrink`
- Row / Column / Stack 已共享这套基础布局协议

### 未完成部分
- backend 当前主要返回 frames 向量，缺少统一 `LayoutResult` / 调试信息表达

---

## Issue 031 — 实现 Row 容器 ✅ 已完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — Row 容器已落地并有测试覆盖

### 目标
实现最基础的水平容器布局。

### 范围
支持：
- gap
- padding
- align_items
- justify_content
- preferred size 基本处理

### 产出
- `layout/row`

### 完成定义
- 可用于排列按钮、标题栏内容等

### 完成记录
- `layout/src/layout_container.cppm` 已实现 Row
- `tests/layout/test_layout_core.cpp` 已覆盖 Row 布局行为

---

## Issue 032 — 实现 Column 容器 ✅ 已完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — Column 容器已落地并有测试覆盖

### 目标
实现最基础的垂直容器布局。

### 范围
与 Row 对应：
- gap
- padding
- align_items
- justify_content

### 产出
- `layout/column`

### 完成定义
- 可用于页面主结构、表单结构

### 完成记录
- `layout/src/layout_container.cppm` 已实现 Column
- `tests/layout/test_layout_core.cpp` 已覆盖 Column 基础布局与 flex 分配行为

---

## Issue 033 — 实现 Stack 容器 ✅ 已完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — Stack 容器已落地并有测试覆盖

### 目标
支持 overlay/重叠子节点布局。

### 范围
- children overlay
- alignment
- simplest stack semantics

### 产出
- `layout/stack`

### 完成定义
- 能承载背景层 + 内容层 + 覆盖层等结构

### 完成记录
- `layout/src/layout_container.cppm` 已实现 Stack
- `tests/layout/test_layout_core.cpp` 已覆盖 overlay / alignment 语义

---

## Issue 034 — 实现 Spacer 与 SizedBox ✅ 已完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — 布局辅助组件已落地

### 目标
提供常见布局辅助元素。

### 范围
- 固定尺寸盒子
- 弹性 spacer

### 产出
- `layout/helpers`

### 完成定义
- 页面结构不必靠空 widget 硬编码

### 完成记录
- `layout/src/flex_widgets.cppm` 已实现 `Spacer` 与 `SizedBox`
- `layout/src/nan_layout.cppm` 已作为公共入口导出

---

## Issue 035 — 实现轻量 Flex 能力（grow/shrink/basis） ⚠️ 部分完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — 轻量 grow 已有，但 shrink / basis 语义仍未补齐

### 目标
补足基础容器在常见自适应布局中的能力。

### 范围
- flex factor
- basis
- shrink/grow 基础语义

### 产出
- `layout/flex`

### 完成定义
- 基础页面不必过早引入 Yoga 才能自适应

### 当前进展
- 当前 backend 已支持基于 flex factor 的轻量空间分配
- `Expanded` / `Spacer` 已能表达基础 grow 行为

### 未完成部分
- shrink / basis 尚未形成明确语义与公开模型
- 当前能力还不能完全覆盖 issue 定义中的 flex 三元组

---

## Issue 036 — 为布局系统编写单元测试 ⚠️ 部分完成
**Labels:** `area:layout`, `area:tests`, `kind:test`, `priority:p0`
**Status:** ⚠️ 部分完成 — 基础容器测试已存在，但边界场景覆盖仍不完整

### 目标
验证基础布局算法在边界条件下的稳定性。

### 范围
至少覆盖：
- Row basic
- Column basic
- gap/padding
- align/justify
- Stack overlay
- zero-size / constrained-size cases

### 产出
- `tests/layout/*`

### 完成定义
- 布局成为可安全迭代的模块，而非全靠手工跑 UI 看

### 当前进展
- `tests/layout/test_layout_core.cpp` 已覆盖 Row / Column / Stack、gap / padding、align / justify 等核心行为
- 布局测试已接入主测试构建

### 未完成部分
- zero-size / constrained-size 这类边界场景仍缺少专门测试
- `measure -> layout -> reflow` 两阶段链路、layout dirty 传播、resize 后重排等场景仍缺少测试
- 还不能算达到“边界条件下稳定性”这一完整目标

---

## Layout 主线收口 Lane（2026-05 建议按此顺序推进）

> 说明：Issue 029-036 记录的是 Layout 基础能力的建设情况；下面这一组 issue 面向下一轮主线收口，目标是让 measure/layout 协议真正接管 widgets 与 showcase，而不是继续停留在“协议存在、调用分散”的状态。

## Issue 082 — 让 LayoutCore 接入 NanConstraints 并扩展 LayoutChildSpec ✅ 已完成
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `LayoutRequest`、`LayoutChildSpec` 与 backend 已接入 `NanConstraints`、`min/max` 与 `can_shrink` 语义

### 目标
让 `layout/core` 的输入模型显式感知约束、最小/最大尺寸与 shrink 能力，为后续两阶段布局提供统一底座。

### 范围
- 为 `LayoutRequest` 增加 `NanConstraints`
- 为 `LayoutChildSpec` 增加 `min_size` / `max_size` / `can_shrink`
- 更新 row/column backend 的 cross/main-axis 计算，使其对约束与 clamp 语义敏感

### 涉及文件
- `layout/src/layout_core.cppm`
- `foundation/src/nan_constraints.cppm`
- `tests/layout/test_layout_core.cpp`

### 完成定义
- layout backend 不再只依赖 `preferred_size`
- row/column 的 frames 分配能正确响应约束、min/max 与 shrink 语义

### 完成记录
- `layout/src/layout_core.cppm` 已为 `LayoutRequest` 增加 `NanConstraints`
- `LayoutChildSpec` 已包含 `min_size` / `max_size` / `can_shrink`
- `tests/layout/test_layout_core.cpp` 已覆盖 stretch + max 限制、flex child min/max clamp 等回归面

---

## Issue 083 — 让 LayoutContainer 切换到 measure/layout 两阶段驱动 ✅ 已完成
**Labels:** `area:layout`, `area:runtime`, `kind:refactor`, `priority:p0`
**Status:** ✅ 已完成 — `LayoutContainer` 主路径已明确为 `measure() -> set_bounds() -> layout()`，`set_bounds()` 不再隐式求解容器布局

### 目标
把容器从“赋 bounds 即立即分发布局”的旧路径，切到“measure 收集、layout 应用”的两阶段协议。

### 范围
- 为 `LayoutContainer` 实现 `measure()`，缓存 child specs
- 让 `layout()` 消费 measured child specs，而不是重新临时读取 `preferred_size()`
- 精简 `set_bounds()`，去掉对 `layout()` 的隐式调用

### 涉及文件
- `layout/src/layout_container.cppm`
- `runtime/src/nan_widget.cppm`
- `tests/layout/test_layout_core.cpp`

### 完成定义
- `LayoutContainer` 的主路径明确为 `measure() -> set_bounds() -> layout()`
- `set_bounds()` 不再承担容器级布局求解职责

### 完成记录
- `layout/src/layout_container.cppm` 已在 `measure()` 中缓存 child specs，并让 `layout()` 消费测量结果
- `LayoutContainer::set_bounds()` 现仅负责 bounds 分配，不再隐式调用 `layout()`
- backend 应用阶段会在 child `set_bounds(...)` 后显式递归 `child.layout()`，保证嵌套容器仍沿两阶段主链传播
- `tests/layout/test_layout_core.cpp` 已新增“set_bounds 不自动 layout children”与“显式 layout 传播到嵌套容器”回归测试

---

## Issue 084 — 为根节点建立统一 reflow 入口与 layout dirty 闭环 ✅ 已完成
**Labels:** `area:app`, `area:runtime`, `area:layout`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `NanAppWindow` 已作为当前架构下的根宿主，统一收口 set_root、resize 节流、draw 前消费与 hit-test 前消费的 root reflow 语义

### 目标
建立从 window/app 根节点发起的统一重排入口，使 layout dirty 能真正驱动全树 measure/layout，而不是仅靠局部 `set_bounds()` 副作用维持。

### 范围
- 在 app/window 根组件尺寸变化时触发 measure + layout
- 明确 `mark_layout_dirty()` 的上行传播与根级消费时机
- 补齐 resize 后 root bounds 与真实 reflow 的协同语义
- 让当前架构下的 root reflow 责任明确收敛在 `app` 层窗口宿主，而不是继续散落在局部 `set_bounds()` 副作用中

### 涉及文件
- `app/src/nan_application.cppm`
- `tests/app/test_app_authoring.cpp`
- `tests/showcase/test_showcase_layout.cpp`

### 完成定义
- root resize、child 结构变化、布局属性变化都能通过统一入口触发 reflow
- 不再依赖局部容器的即时 layout 副作用来维持整棵树一致性

### 当前结果
- `NanAppWindow` 现已通过统一的 request / consume root reflow 入口收口 `set_root()`、启动初始同步、resize 节流、draw 前消费与 root hit-test 前消费
- root bounds 同步与 measure / layout 消费已进入同一主链，不再区分旧的 immediate / deferred 分叉语义
- `tests/app/test_app_authoring.cpp` 已覆盖 draw 前消费与 first draw 之前 hit-test 消费 root layout dirty 的回归场景
- `tests/showcase/test_showcase_layout.cpp` 已验证 showcase 页面在该主链下保持正确布局与可绘制状态

---

## Issue 085 — 适配 FlexWidgets 到新布局协议 ❌ 未完成
**Labels:** `area:layout`, `kind:refactor`, `priority:p1`
**Status:** ❌ 未完成 — `Spacer` / `Expanded` / `Padding` / `Center` / `SizedBox` 仍偏向旧式 `set_bounds()` 行为

### 目标
让布局辅助组件全面兼容两阶段布局协议，成为 widgets 和 authoring API 的稳定基础设施。

### 范围
- 让 `Padding` / `Center` / `SizedBox` / `Expanded` 在 `measure()` 中表达真实尺寸语义
- 减少这些辅助组件对即时 `set_bounds()` 传播的依赖
- 明确各类 helper 的 `preferred_size()` 与 `measured_size()` 关系

### 涉及文件
- `layout/src/flex_widgets.cppm`
- `tests/layout/test_layout_core.cpp`
- `tests/showcase/test_showcase_layout.cpp`

### 完成定义
- 布局辅助组件在新协议下行为稳定
- authoring API 可以继续直接复用这批 helper，而不引入额外手算逻辑

---

## Issue 086 — 清理 widgets 中的手工布局分发 ✅ 已完成
**Labels:** `area:widgets`, `area:layout`, `kind:refactor`, `priority:p0`
**Status:** ✅ 已完成 — `Pressable` / `Button` / `Card` / `Panel` / `SplitRow` / `Sidebar` 系列已回到两阶段布局主链；其中 `SidebarGroup` 已从手工 item frame 计算收口到 `Surface + Column + Padding/SizedBox` 组合，现阶段后续工作转入 widgets 测试、primitive/theme 收口与更高层 Sidebar 结构化设计

### 目标
把 widgets 收口到“组合布局原语或极少量叶子级几何”的模式，减少控件内部重复实现布局逻辑。

### 范围
- 优先重构 `Pressable` / `Button` / `Card` / `Panel`
- 继续收敛 `SplitRow`、`SidebarGroup`、`SidebarMenuButton` 等仍含手算布局的控件
- 修正 widgets 运行时 setter 的 `mark_dirty()` / `mark_layout_dirty()` 语义，避免属性更新绕过根布局回流
- 形成一套可复用的 widgets 收口模板

### 涉及文件
- `widgets/src/nan_pressable.cppm`
- `widgets/src/nan_button.cppm`
- `widgets/src/nan_card.cppm`
- `widgets/src/nan_panel.cppm`
- `widgets/src/nan_split_row.cppm`
- `widgets/src/nan_sidebar_group.cppm`
- `widgets/src/nan_sidebar_menu_button.cppm`
- `tests/app/test_app_authoring.cpp`
- `tests/widgets/*`（后续由 Issue 078 独立补齐）

### 完成定义
- widgets 内部的大多数布局不再依赖手工 frame 计算
- 基础控件可作为 showcase 与后续页面 authoring 的稳定积木

### 当前结果
- `Surface` / `Button` / `Card` / `Panel` / `Sidebar` 系列已统一回到 measure/layout 主链
- `SidebarGroup` 不再手工写死 label/item 的 y 偏移和 item 高度，改为内部组合布局
- `tests/app/test_app_authoring.cpp` 已补回归，覆盖 `SidebarGroup` 的动态 label slot 与“按子项首选高度布局”行为

---

## Issue 087 — 清理 showcase 中的手工 frame 计算并回归 authoring/layout 原语 ✅ 已完成
**Labels:** `area:showcase`, `area:layout`, `area:app`, `kind:refactor`, `priority:p0`
**Status:** ✅ 已完成 — 当前 showcase 入口已回到 `router + create_shell + page host` 主链；页面内容不再自带嵌套导航布局，showcase 测试也改为验证结构与路由语义而不是补丁式几何计算

### 目标
让 showcase 真正成为 layout 协议与 widgets 组合模式的验证场，而不是继续承载补丁式布局样例。

### 范围
- 优先清理 cards / sections 中仍保留的手工 `set_bounds()` 逻辑
- 让页面结构更多回归 `row/column/stack/padding/expanded/sized_box`
- 保持现有 showcase 布局测试可读且稳定

### 涉及文件
- `showcase/nan_showcase.cppm`
- `showcase/main_page.cppm`
- `showcase/pages/sandbox_page.cppm`
- `showcase/pages/button.cppm`
- `tests/showcase/test_showcase_layout.cpp`

### 完成定义
- showcase 主页面中的手工几何计算显著减少
- showcase 布局测试主要验证结构和协议，而不是为补丁式手算背书

### 当前结果
- `MainWindow` 不再直接 mount 单页，而是通过 `create_showcase_shell()` 统一注册页面并交给 `create_shell()` 构建标准 Sidebar + PageHost 壳
- `MainPage` 已收口为纯内容页，不再嵌入自己的 Sidebar，避免 showcase 可执行路径与 page/router/shell 主线分叉
- `tests/showcase/test_showcase_layout.cpp` 已新增真实 showcase shell 回归，验证多页面注册后 Sidebar 按页面元数据生成导航按钮并保持初始激活态

---

## Issue 088 — 补齐 layout 回归测试矩阵 ❌ 未完成
**Labels:** `area:layout`, `area:tests`, `kind:test`, `priority:p0`
**Status:** ❌ 未完成 — 现有测试覆盖基础容器，但尚不足以保护下一轮收口重构

### 目标
为 layout 主线收口建立可回归的测试矩阵，避免后续每次改布局都只能靠肉眼检查 showcase。

### 范围
- 增加 constraints/min/max/shrink 的 backend 测试
- 增加 `measure/layout/reflow` 链路测试
- 增加 resize / dirty propagation / helper widgets / showcase 结构测试

### 涉及文件
- `tests/layout/test_layout_core.cpp`
- `tests/showcase/test_showcase_layout.cpp`
- `tests/widgets/*`
- `tests/runtime/*`

### 完成定义
- layout 主线收口涉及的核心路径均有自动化测试保护
- 后续继续演进 Yoga 接入位或 widgets 组合方式时，能快速识别回归

---

## Issue 089 — 以当前实现为准重写 Layout 里程碑验收标准 ❌ 未完成
**Labels:** `area:docs`, `area:layout`, `kind:docs`, `priority:p1`
**Status:** ❌ 未完成 — 当前 DoD 仍偏向基础能力建设，未体现“主线收口”这一实际目标

### 目标
把 Layout 里程碑从“是否已有 Row/Column/Stack”升级为“是否已接管 widgets/showcase 布局”的验收口径。

### 范围
- 同步 `develop-issue.md` 中 M3 的完成定义
- 同步 `roadmap.md` 与 `index.md` 的里程碑状态
- 明确 Yoga 接入位的前置完成条件

### 涉及文件
- `docs/develop-issue.md`
- `docs/roadmap.md`
- `docs/index.md`
- `docs/layout-strategy.md`

### 完成定义
- 文档中的 layout 里程碑表述与代码现实一致
- 后续开发能直接按“协议收口 -> widgets 收口 -> showcase 回归 -> Yoga 评估”的顺序推进

---

# Milestone M4 — Theme 与 Design System Foundation

## Issue 037 — 定义主题系统核心语义枚举 ⚠️ 部分完成
**Labels:** `area:theme`, `kind:architecture`, `priority:p0`
**Status:** ⚠️ 部分完成 — theme 类型基础已存在，但控件层语义枚举仍未统一消费

### 目标
统一 design system 的语义基础。

### 范围
建议包括：
- `ColorVariant`
- `Preset`
- `SizeVariant`
- `TypographyRole`

### 产出
- `theme/types`

### 完成定义
- widgets 不再各自发明 variant/preset/size 概念

### 当前进展
- `theme/types` 已提供颜色、组件状态、字重等主题基础枚举与类型

### 未完成部分
- `ColorVariant` / `Preset` / `SizeVariant` / `TypographyRole` 这类面向 widgets API 的核心语义仍未完整成型
- widgets 侧仍普遍直接使用局部颜色和尺寸配置，没有统一收敛到语义枚举

---

## Issue 038 — 定义 spacing/radius/border 等 primitive tokens ⚠️ 部分完成
**Labels:** `area:theme`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — spacing/radius 等 token 已有，但 border/divider/focus ring 仍不完整

### 目标
建立基础视觉 token。

### 范围
- spacing scale
- radius scale
- border width
- divider width
- focus ring width

### 产出
- `theme/tokens/primitives`

### 完成定义
- widgets 可以共享结构性视觉参数

### 当前进展
- `theme/tokens/primitives` 已聚合 spacing / radius / elevation / opacity / typography
- `tests/theme/test_theme_types.cpp` 已覆盖这些默认 token 值

### 未完成部分
- issue 范围中的 border width、divider width、focus ring width 尚未形成独立 primitive token

---

## Issue 039 — 定义 palette 结构与语义色映射 ⚠️ 部分完成
**Labels:** `area:theme`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — palette 与 light/dark 映射已存在，但控件消费侧仍大量硬编码颜色

### 目标
建立统一颜色系统，而不是零散 RGB 常量。

### 范围
- semantic family
- shades/steps
- light/dark mode 映射策略

### 产出
- `theme/palette`

### 完成定义
- Surface/Button/Label 等可通过语义色而非硬编码色工作

### 当前进展
- `theme/palette` 已提供语义色与 light/dark 映射策略
- `tests/theme/test_theme_manager.cpp` 已覆盖 scheme 切换取色

### 未完成部分
- `Surface` / `Button` / `Label` / `Panel` 等控件尚未系统接入语义色解析，主线仍有大量 `NanRgb` 常量写死

---

## Issue 040 — 定义 typography token 与文字角色 ⚠️ 部分完成
**Labels:** `area:theme`, `area:text`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — typography tokens 已存在，但文字角色尚未接入控件 API

### 目标
建立统一文字层级与字体语义。

### 范围
- base text
- heading
- mono
- size scale
- font weight
- line height basic

### 产出
- `theme/typography`

### 完成定义
- 文本控件不再各自硬编码字体大小/粗细

### 当前进展
- `theme/typography` 已提供默认字号、字重等排版 token

### 未完成部分
- `Label` 等文本控件仍直接使用 raw font size，而不是 `TypographyRole`
- 文字角色尚未形成 widgets 层公开语义 API

---

## Issue 041 — 实现 ThemeManager 最小版本 ⚠️ 部分完成
**Labels:** `area:theme`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — ThemeManager 已实现，但主题入口尚未真正驱动主线控件

### 目标
建立统一主题切换入口。

### 范围
- current theme
- dark/light mode
- resolved primitives
- resolved palette

### 产出
- `theme/manager`

### 完成定义
- widgets 可以从统一主题入口读取当前主题信息

### 当前进展
- `theme/manager` 已支持当前主题、light/dark 切换、主题注册与监听
- `tests/theme/test_theme_manager.cpp` 已覆盖注册、激活、切换与监听

### 未完成部分
- 主线 widgets 仍大量直接使用局部样式常量，尚未系统从统一主题入口解析当前主题

---

## Issue 042 — 编写 design token 文档 ⚠️ 部分完成
**Labels:** `area:theme`, `area:docs`, `kind:docs`, `priority:p1`
**Status:** ⚠️ 部分完成 — token 体系已有代码与测试，但独立文档尚未落位

### 目标
把 token 体系写清楚，避免后续扩展控件时语义漂移。

### 范围
说明：
- colorVariant 的用途
- preset 的用途
- size 的用途
- spacing/radius 的级别选择原则

### 产出
- `docs/design-tokens.md`

### 完成定义
- 未来控件作者知道应该复用哪些 token，而不是临时创造属性

### 当前进展
- token 结构与默认值已存在于 theme 模块实现与测试中

### 未完成部分
- `docs/design-tokens.md` 尚未落位
- 当前缺少面向控件作者的 token 选型说明文档

---

# Milestone M5 — Render 与 Text 基础能力

## Issue 043 — 定义 RenderBackend 抽象接口 ❌ 未完成
**Labels:** `area:render`, `kind:architecture`, `priority:p0`
**Status:** ❌ 未完成 — RenderBackend 抽象接口尚未落地

### 目标
抽象渲染后端，避免未来具体实现反向污染 runtime/widgets。

### 范围
- begin frame
- submit scene/draw commands
- present
- capabilities 查询

### 产出
- `render/backend`

### 完成定义
- runtime/scene 与具体 renderer 解耦

### 当前判断
- `render/` 当前仍主要是占位结构，未见真实 backend 接口实现
- runtime 仍直接暴露 ThorVG/SDL 绘制路径，未与渲染后端抽象解耦

---

## Issue 044 — 定义 DrawCommand 到 RenderBackend 的适配层 ❌ 未完成
**Labels:** `area:render`, `kind:implementation`, `priority:p0`
**Status:** ❌ 未完成 — 主线未见 DrawCommand encoder / adapter

### 目标
将 Scene/DrawCommand 层稳定映射到渲染后端。

### 范围
- rect
- rounded rect
- text
- clip

### 产出
- command encoder / adapter

### 完成定义
- widget 不再需要知道具体绘制 API

### 当前判断
- `NanWidget` 仍直接绘制到 `SwCanvas`
- `NanWindow::present_frame()` 也仍直接调用 widget 的 ThorVG 绘制路径

---

## Issue 045 — 实现 ThorVG 软件后端适配器 ⚠️ 部分完成
**Labels:** `area:render`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — ThorVG 软件渲染闭环已存在，但尚未以独立 render 适配器形态落地

### 目标
使用当前最现实的后端打通第一版绘制。

### 范围
- basic shapes
- software canvas
- frame submit

### 产出
- `render/thorvg`

### 完成定义
- 第一版运行时可稳定绘制基础 UI

### 当前进展
- runtime 已通过 ThorVG `SwCanvas` + SDL 打通稳定的软件渲染闭环
- `runtime/docs/runtime-design.md` 已描述当前渲染管线

### 未完成部分
- `render/thorvg` 仍未作为独立 adapter 模块落位
- 当前实现仍耦合在 runtime 窗口实现中

---

## Issue 046 — 定义 text 模块的最小接口 ⚠️ 部分完成
**Labels:** `area:text`, `kind:architecture`, `priority:p0`
**Status:** ⚠️ 部分完成 — 已有 NanFont 与文本布局能力，但 text/core 分层接口仍不完整

### 目标
建立文字渲染与测量的统一入口，避免文本逻辑散落在 widgets/render 中。

### 范围
- text style
- text layout input
- measure text
- render text

### 产出
- `text/core`

### 完成定义
- Label/Button 等可以依赖统一 text 接口

### 当前进展
- `NanFont` 已提供文本布局、测量与绘制相关能力

### 未完成部分
- 主线仍缺少独立的 text style / layout input / render 分层接口
- `text/core` 目前更像 `NanFont` 聚合入口，而不是清晰的最小接口层

---

## Issue 047 — 实现基础文本测量能力 ✅ 已完成
**Labels:** `area:text`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — 文本测量能力已落地并有测试覆盖

### 目标
支持组件进行最基本的文本尺寸计算。

### 范围
- plain string
- font size
- single-line measure
- basic multi-line 预留接口

### 产出
- `text/measure`

### 完成定义
- Label/Button 的隐式尺寸可被文本驱动

### 完成记录
- `text/measure` 已支持文本测量与 shaping
- `tests/text/test_nan_font.cpp` 已覆盖测量与基础 shaping 行为

---

## Issue 048 — 实现基础文本绘制能力 ⚠️ 部分完成
**Labels:** `area:text`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — 文本已能稳定绘制到 UI，但仍未进入目标中的抽象渲染路径

### 目标
让文本可以进入 draw command/render backend 路径。

### 范围
- single-line render
- alignment basic
- text color

### 产出
- `text/render`

### 完成定义
- 可以在 UI 中稳定显示文本节点

### 当前进展
- `NanFont` 已支持基础文本绘制
- `Label` 已能在真实 UI 中显示文本节点

### 未完成部分
- 当前文本仍直接走 ThorVG 绘制路径，没有接入 DrawCommand / RenderBackend 抽象链路

---

## Issue 049 — 编写渲染策略文档 ⚠️ 部分完成
**Labels:** `area:render`, `area:docs`, `kind:docs`, `priority:p1`
**Status:** ⚠️ 部分完成 — 已有渲染路线说明，但独立策略文档尚未落位

### 目标
明确当前不自研渲染器、先做抽象和适配器的路线。

### 范围
说明：
- 为什么先抽象 backend
- 为什么不马上做 Vulkan 自绘库
- ThorVG 在当前阶段的角色
- 未来 backend 扩展预留

### 产出
- `docs/rendering-strategy.md`

### 完成定义
- 后续讨论 render 方向时有明确依据

### 当前进展
- `runtime/docs/runtime-design.md` 已说明当前 ThorVG/SDL 渲染管线与阶段性路线
- 仓库中也已有对 backend 抽象方向的若干说明

### 未完成部分
- `docs/rendering-strategy.md` 尚未落位
- 当前缺少一份独立、面向项目决策的渲染策略文档

---

# Milestone M6 — Widgets: Primitives

## Issue 050 — 实现 Surface primitive ✅ 已完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `Surface` 已作为基础视觉容器供多个控件复用

### 目标
建立最基础的视觉表面组件。

### 范围
支持：
- background
- border
- radius
- theme-aware semantic colors

### 产出
- `widgets/primitives/surface`

### 完成定义
- 可作为 Panel/Card/Button 等的底层视觉积木

### 完成记录
- 已支持 background / border / radius / padding
- `Panel`、`Card`、`Button`、`Sidebar`、`SidebarMenuButton` 已直接复用 `Surface`
- 落地文件：`widgets/src/nan_surface.cppm`

---

## Issue 051 — 实现 Pressable primitive ✅ 已完成
**Labels:** `area:widgets`, `area:runtime`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — `Pressable` 已提供统一交互状态机

### 目标
建立纯交互 primitive，不直接绑定视觉表现。

### 范围
支持：
- hover
- press
- click
- disabled
- keyboard activation 基础预留

### 产出
- `widgets/primitives/pressable`

### 完成定义
- Button/Card 等交互组件可复用统一行为逻辑

### 完成记录
- 已支持 hover / press / click / disabled / focus 状态
- 已提供 `on_click` / `on_press` / `on_release` / `on_hover` / `on_leave` 与 signal 接口
- 已有 app / widgets 测试覆盖其禁用态与状态清理语义
- 落地文件：`widgets/src/nan_pressable.cppm`

---

## Issue 052 — 实现 Text primitive ⚠️ 部分完成
**Labels:** `area:widgets`, `area:text`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — 文本能力已被 `Label` 吸收，但尚未形成独立 Text primitive

### 目标
建立 widgets 层统一的文本节点封装。

### 范围
- text content
- color
- style role
- alignment basic

### 产出
- `widgets/primitives/text`

### 完成定义
- Label/Button 不必各自重新封装底层 text 逻辑

### 当前进展
- `Label` 已封装文本内容、颜色、基础对齐、字体加载、shaping 与绘制能力
- 文本底层能力由 `text/nan_font` 提供，已可支撑 widgets 层文本显示
- 落地文件：`widgets/src/nan_label.cppm`、`text/src/nan_font.cppm`

### 未完成部分
- 当前没有独立的 `widgets/primitives/text` 模块或控件
- `style role` 尚未形成独立语义 API，Button 也仍然通过嵌入 `Label` 间接复用文本能力

---

## Issue 053 — 实现 FocusRing primitive ❌ 未完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 未见独立 FocusRing primitive 或统一焦点可视化实现

### 目标
为交互组件提供统一焦点可视化 primitive。

### 范围
- focus visible state
- ring color from theme
- border/outline rendering

### 产出
- `widgets/primitives/focus_ring`

### 完成定义
- Button/Input 等未来可复用统一焦点反馈

### 当前判断
- 当前 `Pressable` 虽有 `focused` 状态，但 widgets 层没有独立 `FocusRing` 组件或 outline 渲染积木
- 仓库中也未见 `focus visible state` 到 `ring color / border / outline` 的复用实现
- theme 规划里能看到 focus ring token 需求，但主线 widgets 尚未消费它

---

## Issue 054 — 编写 primitive 设计文档 ❌ 未完成
**Labels:** `area:widgets`, `area:docs`, `kind:docs`, `priority:p1`
**Status:** ❌ 未完成 — `docs/widget-primitives.md` 尚未落位

### 目标
明确 primitive 与 control 的边界，避免组件体系重新混乱。

### 范围
说明：
- Surface 是什么
- Pressable 是什么
- Text primitive 是什么
- FocusRing 是什么
- 何时应复用 primitive 而不是直接在控件里硬写

### 产出
- `docs/widget-primitives.md`

### 完成定义
- 后续控件开发有统一积木思维

### 当前判断
- 主线 docs 中不存在 `docs/widget-primitives.md`
- 目前只有 `develop-issue.md` 中的问题拆分，没有一份专门解释 Surface / Pressable / Text / FocusRing 边界的文档

---

# Milestone M7 — Widgets: First Controls

## Issue 055 — 实现 Label 控件 ⚠️ 部分完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — 基础 Label 已可用，并已具备 `disabled`；`error` / `required` / typography role 仍未补齐

### 目标
实现第一个主题感知的基础文本控件。

### 范围
支持：
- text / Prop<string>
- disabled
- error
- required
- typography role
- preferred size 由 text 驱动

### 产出
- `widgets/controls/label`

### 完成定义
- Label 可在页面中作为真实控件使用

### 当前进展
- 已实现文本、字体大小、颜色、对齐、`disabled`、preferred size 与真实字体绘制
- 落地文件：`widgets/src/nan_label.cppm`

### 未完成部分
- `error` / `required` 语义状态尚未建模
- typography role 尚未形成独立语义 API

---

## Issue 056 — 为 Label 实现状态驱动样式映射 ❌ 未完成
**Labels:** `area:widgets`, `area:theme`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 未见独立的 Label 语义状态到样式解析层

### 目标
让 Label 的视觉状态来自语义状态而不是散乱条件判断。

### 范围
- normal
- disabled
- error
- required indicator

### 产出
- Label style resolution

### 完成定义
- Label 的状态表现统一并可扩展

### 当前判断
- 当前 `Label` 只暴露文本、颜色、字号和对齐，没有 `normal / disabled / error / required` 的语义状态输入
- 也未见从 theme 解析状态样式的 resolver 或 mapping 层

---

## Issue 057 — 实现 Button 控件最小版本 ⚠️ 部分完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p0`
**Status:** ⚠️ 部分完成 — Button 已具备 text / variant / size / disabled / loading / icon 基础能力，但 theme preset 与上层收口仍未完成

### 目标
实现第一个完整的交互式核心控件。

### 范围
支持：
- text
- on_click
- disabled
- hover/press state
- preset
- size
- colorVariant

### 产出
- `widgets/controls/button`

### 完成定义
- 可稳定用于 showcase 与页面导航

### 当前进展
- 已实现 text / on_click / disabled / hover / press / loading 基础状态
- 已提供 `variant(...)`、`size(...)`、`icon(...)` / `icon_left(...)` / `icon_right(...)` 等公开 API
- 当前实现采用 `Surface + Row + Label + optional Icon` 组合，并直接在 Button 内处理交互事件
- 落地文件：`widgets/src/nan_button.cppm`、`widgets/src/nan_button.cpp`

### 未完成部分
- `colorVariant` 尚未形成独立语义 API
- authoring DSL、showcase 与测试对 icon / loading 等能力的消费仍不充分
- 更完整的 preset / resolver 文档化与 theme 层统一消费仍待收口

---

## Issue 058 — 为 Button 加入 icon slot / 左右图标支持 ⚠️ 部分完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — widgets 层已有 `icon / icon_left / icon_right` API，但 authoring / 测试 / 语义收口仍未完成

### 目标
让 Button API 更接近真实应用需求。

### 范围
- left icon
- right icon
- icon 与 text 的排布关系

### 产出
- Button icon support

### 完成定义
- Button 可以表达更真实的 UI 场景

### 当前进展
- `Button` 已提供 `icon(...)`、`icon_left(...)`、`icon_right(...)` 公开接口
- widgets 层已将 `Icon` 接入 `Button` 的内部内容排布
- 模块接口注释已包含 `button("Save").icon_left(...).on_click(...)` 这类使用示例

### 未完成部分
- app authoring 层尚未暴露对应的链式 icon API
- tests / showcase 尚未把 icon button 作为稳定回归面
- `icon_right` 的更完整独立 slot 语义仍值得继续收口

---

## Issue 059 — 实现 Button 的 preset 视觉映射 ⚠️ 部分完成
**Labels:** `area:widgets`, `area:theme`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — Button 已有 `ButtonVariant` 与基础视觉切换，但 preset 体系与 theme resolver 仍未完全收口

### 目标
建立统一按钮视觉语义。

### 范围
建议覆盖：
- filled
- tonal
- outlined
- ghost
- link

### 产出
- Button preset style resolver

### 完成定义
- preset 不是仅有属性名，而有真实一致的样式差异

### 当前进展
- `Button` 已提供 `ButtonVariant` 枚举与 `variant(...)` API
- app 测试已覆盖 `default_variant`、`outline`、`ghost`、`destructive` 等基础视觉切换

### 未完成部分
- preset 体系仍未以组件规格文档与 theme resolver 的形式单独收口
- `tonal` / `link` 等更完整语义集合仍未稳定
- 变体到 token/resolver 的统一映射仍未脱离组件内部实现细节

---

## Issue 060 — 实现 Panel 容器控件 ⚠️ 部分完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ⚠️ 部分完成 — Panel 基础容器已可用，但 header action 尚未支持

### 目标
提供带标题与内边距的基础容器。

### 范围
- title
- content slot
- optional header action
- padding
- surface composition

### 产出
- `widgets/containers/panel`

### 完成定义
- 可用于 showcase 页面中的内容分区

### 当前进展
- 已支持 title / header_color / header_height / padding / 内容区布局
- app authoring 测试已覆盖 `panel(...).title(...).bg_color(...).corner_radius(...).bind(...)` 基本链路
- 落地文件：`widgets/src/nan_panel.cppm`

### 未完成部分
- optional header action 仍未实现
- showcase 中作为内容分区的系统性验证仍不足，完成定义里的“页面级内容分区验证”证据还不够

---

## Issue 061 — 实现 Card 容器控件 ⚠️ 部分完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p2`
**Status:** ⚠️ 部分完成 — Card 已具备标题与内容容器能力，但结构化 slot 仍不完整

### 目标
提供结构化内容展示容器。

### 范围
- title
- description
- content
- footer
- optional media/image 先可占位

### 产出
- `widgets/containers/card`

### 完成定义
- Card 可承载业务展示块

### 当前进展
- 已支持 title / accent / content / elevation，且已在 showcase 统计卡片中使用
- 落地文件：`widgets/src/nan_card.cppm`

### 未完成部分
- `description` / `footer` / media/image slot 仍未形成组件级 API
- 目前 `Card` 的真实消费集中在标题 + 内容 + accent 用例，尚未覆盖更完整的结构化展示块语义

---

## Issue 062 — 编写 Label/Button/Panel 组件规格文档 ❌ 未完成
**Labels:** `area:widgets`, `area:docs`, `kind:docs`, `priority:p1`
**Status:** ❌ 未完成 — 目标文档尚未落位

### 目标
让基础控件从一开始就有文档，不靠未来补票。

### 范围
每个组件至少包含：
- 目标
- API
- states
- variants
- examples
- 不支持的内容

### 产出
- `docs/components/label.md`
- `docs/components/button.md`
- `docs/components/panel.md`

### 完成定义
- showcase 与组件文档能互相对应

### 当前判断
- 仓库中不存在 `docs/components/label.md`、`docs/components/button.md`、`docs/components/panel.md`
- 当前只有代码实现与 showcase 验证，尚未建立组件规格文档面

---

# Milestone M8 — App Shell 与 Navigation

## Issue 063 — 定义 Page 抽象与页面元数据模型 ✅ 已完成
**Labels:** `area:app`, `kind:architecture`, `priority:p0`
**Status:** ✅ 已完成 — `NanPage` 抽象已落地于 `app/src/nan_page.cppm`

### 目标
建立页面层抽象，而不是直接用普通 widget 拼页面。

### 范围
建议包括：
- route key
- title
- summary
- build root
- optional lifecycle hooks

### 产出
- `app/page`

### 完成定义
- 页面在框架中成为第一等公民

### 当前判断
- `app/src` 当前只有 `nan_application.cppm`，未见 `Page` 基类、route key、title、summary 或 lifecycle hook 约定
- showcase 仍直接挂载单个 `MainComponent`，不是围绕 page object 组织

---

## Issue 064 — 定义 Router 核心接口 ✅ 已完成
**Labels:** `area:app`, `kind:architecture`, `priority:p0`
**Status:** ✅ 已完成 — `NanRouter` 接口与实现均已落地于 `app/src/nan_router.cppm`

### 目标
建立应用级导航系统的基础模型。

### 范围
- route registry
- current route
- replace
- push
- back
- params

### 产出
- `app/router` 设计草案

### 完成定义
- 可以围绕它实现页面切换，而不是临时写 showcase 切页逻辑

### 当前判断
- `app/src` 未见 `Router`、route registry、history stack 或 params 相关接口
- 当前 showcase 也没有多页面切换入口，无法证明路由模型已经建立

---

## Issue 065 — 实现 Router 最小版本 ✅ 已完成
**Labels:** `area:app`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — `NanRouter`（register_page / navigate_to / on_navigate / build_current）已与 showcase 路由打通

### 目标
打通从 route 到 page 实例的切换能力。

### 范围
- route lookup
- route stack/history
- page host integration

### 产出
- router implementation MVP

### 完成定义
- showcase 可以在多个页面间切换

### 当前判断
- 主线没有 route lookup、route stack/history 或 page host integration 的实现文件
- 现有 showcase 仍是单个 dashboard 页面，未形成页面切换能力

---

## Issue 066 — 实现 PageHost / 内容区域承载容器 ✅ 已完成
**Labels:** `area:app`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — `NanPageHost` 已落地于 `app/src/nan_page_host.cppm`，支持延迟换页、bounds 传播与 layout 刷新

### 目标
提供页面显示区域与页面生命周期承载对象。

### 范围
- current page root
- route replacement
- page mount/unmount basic behavior

### 产出
- `app/page_host`

### 完成定义
- Router 可以驱动实际页面显示

### 当前判断
- `app/src` 未见 `page_host` 或等价承载容器
- 当前内容区域由 showcase 自己直接布局，不是由可替换 page root 承载

---

## Issue 067 — 实现基础应用壳结构（导航 + 内容区） ✅ 已完成
**Labels:** `area:app`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — `nan_showcase.cppm` 中 `MainShell` 形成 sidebar（含路由导航）+ `NanPageHost`（内容区）的完整应用壳结构

### 目标
形成一个能支撑 showcase 的最小应用壳。

### 范围
建议包含：
- sidebar/nav area
- content area
- optional top bar/title region

### 产出
- `app/shell`

### 完成定义
- showcase 不再只是“一个页面显示一个组件”，而是真正的应用结构

### 当前进展
- `showcase/application.cppm` 已形成 sidebar / header / stats / middle / bottom / footer / dock 的完整应用壳结构
- 现有 showcase 已不是单一控件裸展示，而是有导航区和内容区的 dashboard 形态

### 未完成部分
- 主线仍未提供独立 `app/shell` 抽象，当前壳层逻辑主要固化在 showcase 层
- 还没有与 `Page` / `Router` / `PageHost` 形成可复用的应用级装配关系

---

## Issue 068 — 设计 WindowShell 抽象（标题栏/边框/区域保留） ❌ 未完成
**Labels:** `area:app`, `area:runtime`, `kind:architecture`, `priority:p2`
**Status:** ❌ 未完成 — `docs/window-shell-strategy.md` 尚未落位

### 目标
为未来桌面应用体验预留窗口壳层。

### 范围
- title bar role
- window control slots
- draggable region
- resize edge integration points

### 产出
- `docs/window-shell-strategy.md`

### 完成定义
- 后续自定义桌面窗口能力有规划可循

### 当前判断
- 主线 docs 中不存在 `docs/window-shell-strategy.md`
- runtime 当前只提供基础窗口封装，未见 title bar role、draggable region、window control slots 的设计说明

---

# Milestone M9 — Showcase 驱动开发

## Issue 069 — 建立 showcase 应用基础入口 ✅ 已完成
**Labels:** `area:showcase`, `kind:implementation`, `priority:p0`
**Status:** ✅ 已完成 — 提交 `abe263e`, `45e18d2`

### 目标
让组件开发从第一批控件起就有真实运行环境。

### 范围
- showcase app entry
- application + window + app shell 接线
- theme switch placeholder

### 产出
- `showcase/app`

### 完成定义
- 可以启动一个组件展示应用

### 完成记录
- `showcase/main.cpp` 与 `showcase/application.cppm` 已形成可启动的 showcase 应用入口与主窗口装配
- 当前 showcase 目录已具备应用壳层与页面骨架拆分：header / stats / middle / bottom / footer / sidebar / dock
- 提交: `abe263e`, `45e18d2`

---

## Issue 070 — 实现 showcase 页面注册机制 ✅ 已完成
**Labels:** `area:showcase`, `area:app`, `kind:implementation`, `priority:p1`
**Status:** ✅ 已完成 — showcase 已通过 `NanRouter::register_page()` 实现集中式页面注册；`nan_showcase.cppm` 注册 Overview/Layout/Widgets/Authoring 四个页面

### 目标
提供集中式页面注册，便于组件文档化展示。

### 范围
- page registry
- grouping/category
- route binding

### 产出
- `showcase/registry`

### 完成定义
- 新增一个组件展示页不需要改很多散乱代码

### 当前判断
- `showcase/` 目录当前只有单个 dashboard 入口与若干 section/card 组件，未见 `registry` 模块
- 也未见页面分组、类别管理或 route binding 相关实现

---

## Issue 071 — 创建 Label showcase 页面 ❌ 未完成
**Labels:** `area:showcase`, `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 尚未存在独立 LabelPage 或状态矩阵展示页

### 目标
用页面形式验证 Label 的真实 API 与状态矩阵。

### 范围
展示：
- basic
- disabled
- error
- required
- typography variants（如果已有）

### 产出
- LabelPage

### 完成定义
- Label 的使用方式清晰可见

### 当前判断
- 当前 showcase 仍是综合 dashboard，没有单独的 Label showcase page
- `disabled / error / required / typography variants` 也没有成页展示面

---

## Issue 072 — 创建 Button showcase 页面 ❌ 未完成
**Labels:** `area:showcase`, `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 尚未存在独立 ButtonPage 或按钮变体展示页

### 目标
系统展示 Button 的状态和变体。

### 范围
展示：
- presets
- color variants
- size variants
- disabled
- icon buttons

### 产出
- ButtonPage

### 完成定义
- Button 的 API 和视觉系统有统一展示面

### 当前判断
- 当前 showcase 没有 `ButtonPage` 或专门的按钮状态/变体页
- 现有代码也尚未具备 issue 范围里要求的 presets / color variants / size variants / icon buttons 完整矩阵

---

## Issue 073 — 创建 Panel/Card showcase 页面 ❌ 未完成
**Labels:** `area:showcase`, `area:widgets`, `kind:implementation`, `priority:p2`
**Status:** ❌ 未完成 — 尚未存在独立 Panel/Card 展示页

### 目标
验证容器型组件的结构表达能力。

### 范围
- panel with title
- panel with actions
- card with content
- card with footer

### 产出
- PanelCardPage

### 完成定义
- 容器组件的用途和限制清晰可见

### 当前判断
- 当前 showcase 只有 dashboard 内部对 `Card` 的局部消费，没有 `PanelCardPage`
- `panel with actions`、`card with footer` 这些目标场景也尚未具体验证

---

## Issue 074 — 在 showcase 中加入主题切换能力 ❌ 未完成
**Labels:** `area:showcase`, `area:theme`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — showcase 未见 theme selector、light/dark toggle 或 live rerender 控件

### 目标
让组件在真实环境中验证 light/dark/theme 切换。

### 范围
- theme selector
- dark/light toggle
- live rerender

### 产出
- showcase theme switch controls

### 完成定义
- 所有基础组件可在不同主题下观察效果

### 当前判断
- `showcase/application.cppm` 当前未接入主题切换控制逻辑
- 现有运行面仍是固定配色 dashboard，不能验证跨主题效果

---

## Issue 075 — 编写 showcase 开发策略文档 ❌ 未完成
**Labels:** `area:showcase`, `area:docs`, `kind:docs`, `priority:p1`
**Status:** ❌ 未完成 — `docs/showcase-strategy.md` 尚未落位

### 目标
明确 showcase 不是 demo 附属品，而是开发期验证基础设施。

### 范围
说明：
- 每个组件都应配 showcase page
- showcase 页面承担 API 验证职责
- showcase 页面结构如何组织

### 产出
- `docs/showcase-strategy.md`

### 完成定义
- 后续不会把 showcase 当作“有空再做”的部分

### 当前判断
- 主线 docs 中不存在 `docs/showcase-strategy.md`
- 当前只有 showcase 代码与 issue 拆分，没有专门沉淀“showcase 作为开发验证基础设施”的策略文档

---

# Milestone M10 — 稳定性、测试与后续扩展准备

## Issue 076 — 为 runtime/widget/event 增加单元测试 ✅ 已完成
**Labels:** `area:runtime`, `area:tests`, `kind:test`, `priority:p1`
**Status:** ✅ 已完成 — `tests/runtime/*` 已覆盖 widget/event 基础行为

### 目标
验证 Widget tree 和事件分发基础行为。

### 范围
- add/remove child
- hit test
- dirty propagation
- basic dispatch

### 产出
- `tests/runtime/*`

### 完成定义
- runtime 基础行为有回归保护

### 完成记录
- `tests/runtime/test_bounds_hit_test.cpp` 已覆盖 bounds、dirty propagation、child/hit test 等 widget tree 基础行为
- `tests/runtime/test_event.cpp` 已覆盖 EventType、事件结构体与基础 variant/event_type 行为
- `tests/runtime/CMakeLists.txt` 已接入主测试构建

---

## Issue 077 — 为 theme 模块增加测试 ✅ 已完成
**Labels:** `area:theme`, `area:tests`, `kind:test`, `priority:p1`
**Status:** ✅ 已完成 — `tests/theme/*` 已覆盖 ThemeManager 与 token/type 基础行为

### 目标
验证 token/palette/theme 切换逻辑。

### 范围
- color resolution
- dark/light switching
- token lookup
- primitive resolution

### 产出
- `tests/theme/*`

### 完成定义
- theme 逻辑不再只能靠肉眼看 UI 判断

### 完成记录
- `tests/theme/test_theme_manager.cpp` 已覆盖主题注册、激活、light/dark 切换、颜色取值、token 访问与变更监听
- `tests/theme/test_theme_types.cpp` 已覆盖 color role、scheme、component state、primitive tokens 等基础类型与默认值
- `tests/theme/CMakeLists.txt` 已接入主测试构建

---

## Issue 078 — 为 widgets 基础控件增加测试 ❌ 未完成
**Labels:** `area:widgets`, `area:tests`, `kind:test`, `priority:p1`
**Status:** ❌ 未完成 — widgets 测试子目录尚未接入，基础控件缺少独立回归测试

### 目标
验证基础控件的状态与交互逻辑。

### 范围
- button click/disabled
- label states
- panel structure rules

### 产出
- `tests/widgets/*`

### 完成定义
- 基础控件可以稳定迭代而不轻易退化

### 当前判断
- `tests/CMakeLists.txt` 中 `add_subdirectory(widgets)` 仍处于注释状态
- 虽然 `tests/app/test_app_authoring.cpp` 已覆盖 Label / Button / Card / Panel 的部分工厂与状态语义，但主线仍未见 `tests/widgets/*` 目录与更聚焦的控件专门测试文件

---

## Issue 079 — 建立 API 冻结策略文档 ❌ 未完成
**Labels:** `area:docs`, `kind:docs`, `priority:p2`
**Status:** ❌ 未完成 — `docs/api-stability-policy.md` 尚未落位

### 目标
明确哪些模块在什么阶段可以视为“相对稳定”。

### 范围
建议说明：
- foundation/types
- reactive core
- layout core
- theme tokens
- first widget APIs

### 产出
- `docs/api-stability-policy.md`

### 完成定义
- 项目不会长期处于“所有东西都不稳定”的状态

### 当前判断
- 主线 docs 中不存在 `docs/api-stability-policy.md`
- 现有文档虽有 roadmap 与模块规划，但没有一份明确定义 API 冻结阶段与稳定边界的策略文档

---

## Issue 080 — 调研脚本/DSL authoring 边界需求 ⚠️ 部分完成
**Labels:** `area:app`, `area:docs`, `kind:architecture`, `priority:p2`
**Status:** ⚠️ 部分完成 — 已有 authoring 草案，但脚本宿主边界尚未单独整理

### 目标
为未来 TypeScript/Lua 等 authoring 方向做边界准备，但不立即实现。

### 范围
调研并总结：
- 哪些 API 需要稳定宿主边界
- 哪些对象不能直接暴露给脚本
- `Prop` / `State` / Widget tree 如何映射到脚本友好模型

### 产出
- `docs/bindings-and-authoring-notes.md`

### 完成定义
- 后续做脚本层时有文档基础，不会从零重新想

### 当前进展
- `docs/godot-like-authoring-draft.md` 已保留为应用层宿主边界的历史参考
- 多份架构文档也已将 app 视为 authoring layer 的承载层

### 未完成部分
- 主线 docs 中不存在 `docs/bindings-and-authoring-notes.md`
- 目前还没有专门回答“哪些对象不能直接暴露给脚本”以及 `Prop` / `State` / Widget tree 如何映射为脚本友好模型

---

## Issue 081 — 调研 render backend 扩展路线（OpenGL/WebGPU/Vulkan） ⚠️ 部分完成
**Labels:** `area:render`, `area:docs`, `kind:architecture`, `priority:p2`
**Status:** ⚠️ 部分完成 — 已有后端比较与方向讨论，但独立路线文档尚未落位

### 目标
为未来渲染扩展建立理性路线，而不是立刻自研。

### 范围
说明：
- 当前 ThorVG adapter 的价值
- scene/render 抽象如何支持新 backend
- 何时才值得评估自绘库

### 产出
- `docs/render-backend-roadmap.md`

### 完成定义
- 渲染方向讨论不再每次回到“要不要马上写 Vulkan”

### 当前进展
- `runtime/docs/runtime-design.md` 已记录当前 ThorVG/SDL 路线以及与其他框架的后端比较
- 主线与 archive 文档中已有对 OpenGL / WebGPU / Vulkan 的零散方向讨论

### 未完成部分
- 主线 docs 中不存在 `docs/render-backend-roadmap.md`
- 目前还没有一份专门梳理“何时值得扩展新 backend、scene/render 抽象如何承载新后端”的路线文档

---

# 2026-05 下一阶段推荐 Issue 序列

> 本节不是从零规划全仓库，而是基于当前主线真实完成度，对“接下来最值得做什么”给出的排序结果。
> 排序原则：优先收口 layout 主线，其次收口 primitive / control 契约，最后再扩大 showcase 与新增控件表面积。

## Phase A — Layout 主线收口（P0，必须先做）

1. **Issue 082 — 让 LayoutCore 接入 NanConstraints 并扩展 LayoutChildSpec**  
    依赖：无  
    原因：这是约束语义的基础，不先固定 child spec 和 constraints，后续 container / widgets / showcase 都会继续补丁化。

2. **Issue 083 — 让 LayoutContainer 切换到 measure/layout 两阶段驱动**  
    依赖：Issue 082  
    原因：只有容器主线切到两阶段，Row / Column / Stack 才能真正成为上层 authoring 的稳定基础。

3. **Issue 084 — 为根节点建立统一 reflow 入口与 layout dirty 闭环**  
    依赖：Issue 083  
    原因：这是把 layout 从“局部可用”变成“整棵树稳定可推理”的关键一步。

4. **Issue 085 — 适配 FlexWidgets 到新布局协议**  
    依赖：Issue 083, Issue 084  
    原因：Expanded / SizedBox / Center / Padding 是当前 authoring DSL 的高频积木，不尽快适配会长期拖住上层 API。

5. **Issue 086 — 清理 widgets 中的手工布局分发**  
    依赖：Issue 085  
    原因：Button / Panel / Card / Sidebar 等控件要真正消费 layout 主线，而不是继续在控件内部保留特殊布局分支。

6. **Issue 088 — 补齐 layout 回归测试矩阵**  
    依赖：Issue 082, Issue 083, Issue 085  
    原因：layout 收口如果没有回归矩阵，后面每做一个控件都会反复踩回归。

7. **Issue 087 — 清理 showcase 中的手工 frame 计算并回归 authoring/layout 原语**  
    依赖：Issue 085, Issue 086  
    原因：showcase 应该是 layout 的回归面，而不是布局补丁层。

8. **Issue 089 — 以当前实现为准重写 Layout 里程碑验收标准**  
    依赖：Issue 082-088 基本完成  
    原因：当前 layout 已经不处于“从零设计”阶段，需要把 DoD 改写成收口后的真实验收标准。

## Phase B — Primitive / Control 收口（P1，依赖 Phase A）

9. **Issue 053 — 实现 FocusRing primitive**  
    依赖：Issue 086，建议与 Issue 038 / Issue 041 联动  
    原因：不把 focus ring 做成 primitive，后续 Input、Button、可聚焦控件都会各写各的焦点表现。

10. **Issue 054 — 编写 primitive 设计文档**  
    依赖：Issue 053 启动后即可推进  
    原因：Surface / Pressable / Text / FocusRing 的边界需要在新增更多控件前先文档化。

11. **Issue 056 — 为 Label 实现状态驱动样式映射**  
    依赖：Issue 054，建议与 Issue 040 / Issue 041 联动  
    原因：Label 现在能显示文本，但还没有真正形成语义状态到样式的映射层。

12. **Issue 059 — 实现 Button 的 preset 视觉映射**  
    依赖：Issue 054，建议与 Issue 037-041 联动  
    原因：Button 已能工作，但 variant / size / 视觉语义还需要进一步从 theme 层收口。

13. **Issue 078 — 为 widgets 基础控件增加测试**  
    依赖：Issue 056, Issue 059 至少完成一部分  
    原因：当前 widgets 缺独立测试目录，继续做控件而没有专门回归面，风险会快速累积。

## Phase C — 表单垂直切片（P1，依赖 Phase A；建议在 Phase B 启动后推进）

14. **Issue 090 — 定义 Input / Field 的 authoring 与状态契约**  
    依赖：Issue 054，建议参考 Issue 053 方向  
    原因：Input 是下一批最有价值的控件，但必须先把 authoring API、受控/非受控边界、状态语义固定下来。

15. **Issue 091 — 实现 TextField 最小版本**  
    依赖：Issue 090  
    原因：单行文本输入会同时压测 focus、键盘事件、文本渲染、光标、placeholder、disabled/read_only 等多个底层能力。

16. **Issue 095 — 为 TextField 接入 authoring 工厂与 signals 绑定模式**  
    依赖：Issue 091  
    原因：项目的核心竞争力之一就是 authoring 体验，Input 不能只在 widgets 层存在，必须尽快接入 app DSL。

17. **Issue 092 — 实现 Field 容器与 label/helper/error 组合**  
    依赖：Issue 090, Issue 091  
    原因：Field 是把 shadcn / primitives 思路转化成 NandinaUI 语义 API 的关键容器，能显著减少表单页面装配成本。

18. **Issue 093 — 为表单控件补齐交互与回归测试**  
    依赖：Issue 091, Issue 092, Issue 095  
    原因：输入控件是最容易积累边界 bug 的一类，没有测试很难继续往 Select、Checkbox、Dialog 走。

19. **Issue 094 — 创建 Forms showcase 页面并纳入导航验证**  
    依赖：Issue 091, Issue 092, Issue 095  
    原因：forms 页面既是展示面，也是对 page/router/component authoring 组合能力的真实验收。

## Phase D — Showcase 扩面与系统验证（P2，依赖 Phase A / B / C 的关键项）

20. **Issue 071 — 创建 Label showcase 页面**  
    依赖：Issue 056  
    原因：Label 页面更适合作为 typography / semantic text role 的验证面，而不是单独优先于 layout 收口。

21. **Issue 072 — 创建 Button showcase 页面**  
    依赖：Issue 059  
    原因：Button 页面应在 variant / preset 基本稳定后再做，否则容易变成临时样式陈列。

22. **Issue 073 — 创建 Panel/Card showcase 页面**  
    依赖：Issue 086, Issue 087  
    原因：容器类 showcase 应建立在 layout 收口后，才能真正验证组合与插槽能力。

23. **Issue 074 — 在 showcase 中加入主题切换能力**  
    依赖：Issue 041，建议在 Issue 071-073 至少完成两页后推进  
    原因：主题切换需要有足够多的组件消费面，否则很难看出 token / resolver 的真实问题。

## Phase E — Sidebar / Navigation 体系（P2，延后到 Primitive/Theme 初步稳定后）

24. **Issue 096 — 定义 Sidebar 结构组件与插槽协议**  
    依赖：Issue 054, Issue 059, Issue 086  
    原因：Sidebar 需要先从“专用成品控件”收口成结构组件（Header/Content/Footer/Group/Menu/...）与默认 stock widgets，避免继续把扩展性压在 Button 继承链上。

25. **Issue 097 — 重构 Sidebar 为可组合结构件**  
    依赖：Issue 096, Issue 087  
    原因：在 layout/showcase 主线稳定后，再把现有 Sidebar 拆成 `SidebarHeader` / `SidebarContent` / `SidebarFooter` / `SidebarGroup` / `SidebarMenu` 等结构件，风险更可控。

26. **Issue 098 — 为 Sidebar 增加 submenu / hierarchical menu**  
    依赖：Issue 096, Issue 097  
    原因：当前更适合先做 Sidebar 专用分层菜单，而不是提前抽象完整 TreeView；先验证导航层级、激活态传播与缩进规则，再决定是否上提通用树模型。

27. **Issue 099 — 为 Sidebar 体系补齐测试与 showcase 验证页**  
    依赖：Issue 097, Issue 098, Issue 078  
    原因：Sidebar 属于复杂复合控件，没有独立回归面和展示页，很难稳定推进折叠、submenu、badge、action 等后续能力。

28. **Issue 100 — 评估是否从 Sidebar submenu 上提 ListView / TreeView primitive**  
    依赖：Issue 098, Issue 099  
    原因：只有在 Sidebar 的层级导航语义先稳定后，才适合判断仓库是否真的需要抽出通用 ListView / TreeView，而不是过早做大抽象。

---

# Milestone M11 — Forms / Authoring 垂直切片

## Issue 090 — 定义 Input / Field 的 authoring 与状态契约 ❌ 未完成
**Labels:** `area:widgets`, `area:app`, `area:docs`, `kind:architecture`, `priority:p1`
**Status:** ❌ 未完成 — 当前主线尚无输入控件契约，缺少下一批 primitives / controls 的共同落点

### 目标
为输入类控件建立统一的状态模型、authoring 语法和组合边界。

### 范围
建议明确：
- `value` / `placeholder`
- `disabled` / `read_only`
- `focused` / `invalid` / `required`
- `on_change` / `on_submit`
- `Ref<TextField>` 与 signals/Var 的推荐绑定方式
- Input 与 Field 的职责分层

### 产出
- `docs/input-and-field-api.md`

### 完成定义
- TextField / Field 的实现不再需要一边写一边临时定 API
- app 层可以围绕统一 authoring 模式继续扩表单类控件

### 当前判断
- runtime 已有 pointer / key / text input 基础通道
- widgets / app 层尚未形成输入控件的公开契约
- 若直接开始写 Input，实现很容易反向塑形 authoring API

---

## Issue 091 — 实现 TextField 最小版本 ❌ 未完成
**Labels:** `area:widgets`, `area:text`, `area:runtime`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 当前主线没有可编辑的文本输入控件

### 目标
实现第一版单行文本输入控件，验证输入交互链路。

### 范围
建议先覆盖：
- 单行文本显示与编辑
- focus / blur
- caret 可视化
- placeholder
- backspace / delete / text input
- disabled / read_only

### 产出
- `widgets/src/nan_text_field.cppm`
- 对应导出与最小工厂接口

### 完成定义
- 可在 showcase 页面中实际输入、编辑和读取文本
- 输入行为不依赖页面层手工处理键盘事件

### 当前判断
- text 渲染与基础键盘事件已经具备落地条件
- 还缺一个真正把 runtime 事件、text 能力和 widgets 状态机串起来的控件

---

## Issue 092 — 实现 Field 容器与 label/helper/error 组合 ❌ 未完成
**Labels:** `area:widgets`, `area:theme`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 当前主线缺少表单语义容器，label / helper / error 仍需页面层手工拼装

### 目标
提供围绕输入控件的语义容器，承接 shadcn / primitives 风格的组合思路。

### 范围
建议覆盖：
- label slot
- control slot
- helper text
- error text
- required / invalid 状态展示

### 产出
- `widgets/src/nan_field.cppm`

### 完成定义
- 页面层不再为最常见的表单行手工拼装 label + input + helper + error
- 表单语义状态可以通过统一容器消费 theme

### 当前判断
- Button / Label 已经证明“组合优于继承”的方向可行
- Field 是把这一套思路扩到表单系统的最佳下一步

---

## Issue 093 — 为表单控件补齐交互与回归测试 ❌ 未完成
**Labels:** `area:widgets`, `area:app`, `area:tests`, `kind:test`, `priority:p1`
**Status:** ❌ 未完成 — 输入控件一旦落地，需要独立测试面锁住焦点、键盘和绑定语义

### 目标
为 TextField / Field 建立稳定回归测试面。

### 范围
建议覆盖：
- focus / blur / caret
- text input / backspace / delete
- disabled / read_only
- value 变更与 signals 绑定
- helper / error 状态展示

### 产出
- `tests/widgets/test_text_field.cpp`
- `tests/app/test_form_authoring.cpp`

### 完成定义
- 输入相关迭代不再主要依赖人工点击验证

### 当前判断
- 表单控件的边界比 Button 更复杂，没有独立测试很难稳定扩展

---

## Issue 094 — 创建 Forms showcase 页面并纳入导航验证 ❌ 未完成
**Labels:** `area:showcase`, `area:widgets`, `area:app`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — 当前 showcase 只有 sandbox，尚不足以承担表单类控件的验证职责

### 目标
建立第一个围绕表单 authoring 的 showcase 页面。

### 范围
建议展示：
- 基础 TextField
- Field + helper/error
- disabled / invalid / required
- signals 驱动的实时表单反馈

### 产出
- `showcase/pages/forms_page.cppm`

### 完成定义
- forms 页面可作为后续更多控件的验证样板
- showcase 导航与 page 元数据可承载真实业务形态页面，而不只是 sandbox

### 当前判断
- 仅靠 sandbox 难以覆盖输入控件的真实组合场景

---

## Issue 095 — 为 TextField 接入 authoring 工厂与 signals 绑定模式 ❌ 未完成
**Labels:** `area:app`, `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — Input 若不接入 app authoring，将无法验证项目当前最重要的开发体验方向

### 目标
让 TextField 与现有 `label()` / `button()` 一样进入 authoring DSL，并验证与 signals 的协作方式。

### 范围
建议覆盖：
- `input()` 或 `text_field()` 工厂
- `bind(ref)`
- `value(...)` / `placeholder(...)`
- `on_change(...)`
- `Var<std::string>` 或等价模式的推荐接法

### 产出
- `app/src/nan_authoring.cppm` 扩展
- 对应 app 层测试

### 完成定义
- 页面层可以用与现有 button/label 一致的风格声明输入控件
- 输入类控件不会退回到“只有底层 widget API 可用”的双轨状态

### 当前判断
- 当前项目的 authoring 体验已经开始形成特色，Input 不应成为例外

---

# 已完成 Issue 记录

> 本章节记录已完成的 issue，作为项目进度的快速参考。

## M0 — 文档与工程基线

| Issue | 标题 | 完成日期 | 提交记录 |
|-------|------|----------|----------|
| 001 | 建立正式项目目录骨架 | 2026-04-22 | `adbc185` |
| 002 | 编写模块依赖方向约束文档 | 2026-04-22 | `ac9d31f` |
| 003 | 定义命名规范与 public/internal API 约定 | 2026-04-22 | `c13c529` |
| 004 | 建立初始开发文档导航页 | 2026-04-23 | `abe263e` |

## M1 — Foundation 与 Runtime MVP

| Issue | 标题 | 完成日期 | 提交记录 |
|-------|------|----------|----------|
| 006 | 实现基础几何类型 Point / Size / Rect / Insets | 2026-04-22 | `3b26ebf`, `1ad8630` |
| 007 | 增加 Constraints 类型并定义测量语义 | 2026-04-24 | 当前 HEAD |
| 008 | 设计并实现 Application 生命周期最小接口 | 2026-04-23 | `8b1fcb9`, `abe263e` |
| 009 | 设计 Window 最小接口 | 2026-04-22 | `c8742aa`（后续迭代） |
| 010 | 打通窗口事件循环到 runtime 事件队列 | 2026-04-22 | `c8742aa`（后续迭代） |
| 011 | 定义统一 Event 类型体系 | 2026-04-26 | `a0e78c7` |

## 额外完成（未列入 Issue 清单）

| 功能 | 描述 | 完成日期 | 提交记录 |
|------|------|----------|----------|
| Log 服务 | 基于 spdlog 的日志框架 | 2026-04-22 | `ef0a191` |
| Color 模块 | NanColor 及颜色转换 | 2026-04-22 | `f282373`, `81b6adb` |
| Widget 树基础 | NanWidget 基类（parent/child/draw） | 2026-04-22 | `8b1fcb9`（与 Issue 012 对应） |
| Godot-like Authoring | 继承式生命周期与输入钩子 | 2026-04-22 | `8b1fcb9` |
| App 层抽象 | NanAppWindow / NanComponent 应用层封装 | 2026-04-23 | `8b1fcb9` |

## M9 — Showcase 驱动开发

| Issue | 标题 | 完成日期 | 提交记录 |
|-------|------|----------|----------|
| 069 | 建立 showcase 应用基础入口 | 2026-05-02 | `abe263e`, `45e18d2` |
| 063 | 定义 Page 抽象与页面元数据模型 | 2026-05-13 | 当前工作区未提交 |
| 064 | 定义 Router 核心接口 | 2026-05-13 | 当前工作区未提交 |
| 065 | 实现 Router 最小版本 | 2026-05-13 | 当前工作区未提交 |
| 066 | 实现 PageHost 内容区域承载容器 | 2026-05-13 | 当前工作区未提交 |
| 067 | 实现基础应用壳结构（sidebar + PageHost） | 2026-05-13 | 当前工作区未提交 |
| 070 | 实现 showcase 页面注册机制 | 2026-05-13 | 当前工作区未提交 |
