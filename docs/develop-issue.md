# NandinaUI 细颗粒度开发 Issue 清单

> 说明：
> - 本清单面向正式项目主线开发，按模块拆分为可独立推进的细颗粒度 issue。
> - 默认假设当前阶段以 **C++ authoring + 文档驱动开发 + showcase 驱动验证** 为主。
> - 不要求一次性全部完成，建议按优先级和依赖顺序逐步创建。
> - 每个 issue 都尽量包含：目标、范围、产出、完成定义。
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

## Issue 016 — 设计 Scene/DrawCommand 中间层
**Labels:** `area:runtime`, `area:render`, `kind:architecture`, `priority:p0`

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

## Issue 018 — 定义 reactive 模块设计说明
**Labels:** `area:reactive`, `kind:docs`, `priority:p0`

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

---

## Issue 019 — 实现 State<T> 最小版本
**Labels:** `area:reactive`, `kind:implementation`, `priority:p0`

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

---

## Issue 020 — 实现 ReadState<T> 只读视图
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`

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

---

## Issue 021 — 实现 Effect 自动依赖追踪
**Labels:** `area:reactive`, `kind:implementation`, `priority:p0`

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

---

## Issue 022 — 实现 EffectScope 生命周期管理
**Labels:** `area:reactive`, `kind:implementation`, `priority:p0`

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

---

## Issue 023 — 实现 Computed 基础能力
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`

### 目标
支持派生值计算，减少手动同步逻辑。

### 范围
- lazily or eagerly recompute 的策略第一版
- 与 State/Effect 协作

### 产出
- `Computed`

### 完成定义
- 可以从多个状态派生出一个只读值

---

## Issue 024 — 为 reactive 增加异常安全与 tracking context 恢复
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`

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

## Issue 026 — 实现 StateList<T> 结构化集合响应式模型
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`

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

---

## Issue 027 — 实现 batch(...) 最小批处理能力
**Labels:** `area:reactive`, `kind:implementation`, `priority:p1`

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

---

## Issue 028 — 编写 reactive 模块单元测试集合
**Labels:** `area:reactive`, `area:tests`, `kind:test`, `priority:p0`

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

---

# Milestone M3 — Layout 模块

## Issue 029 — 定义 layout 协议与测量/布局职责边界
**Labels:** `area:layout`, `kind:architecture`, `priority:p0`

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

---

## Issue 030 — 实现 LayoutNode / LayoutRequest / LayoutResult 基础模型
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`

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

---

## Issue 031 — 实现 Row 容器
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`

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

---

## Issue 032 — 实现 Column 容器
**Labels:** `area:layout`, `kind:implementation`, `priority:p0`

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

---

## Issue 033 — 实现 Stack 容器
**Labels:** `area:layout`, `kind:implementation`, `priority:p1`

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

---

## Issue 034 — 实现 Spacer 与 SizedBox
**Labels:** `area:layout`, `kind:implementation`, `priority:p1`

### 目标
提供常见布局辅助元素。

### 范围
- 固定尺寸盒子
- 弹性 spacer

### 产出
- `layout/helpers`

### 完成定义
- 页面结构不必靠空 widget 硬编码

---

## Issue 035 — 实现轻量 Flex 能力（grow/shrink/basis）
**Labels:** `area:layout`, `kind:implementation`, `priority:p1`

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

---

## Issue 036 — 为布局系统编写单元测试
**Labels:** `area:layout`, `area:tests`, `kind:test`, `priority:p0`

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

---

# Milestone M4 — Theme 与 Design System Foundation

## Issue 037 — 定义主题系统核心语义枚举
**Labels:** `area:theme`, `kind:architecture`, `priority:p0`

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

---

## Issue 038 — 定义 spacing/radius/border 等 primitive tokens
**Labels:** `area:theme`, `kind:implementation`, `priority:p0`

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

---

## Issue 039 — 定义 palette 结构与语义色映射
**Labels:** `area:theme`, `kind:implementation`, `priority:p0`

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

---

## Issue 040 — 定义 typography token 与文字角色
**Labels:** `area:theme`, `area:text`, `kind:implementation`, `priority:p1`

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

---

## Issue 041 — 实现 ThemeManager 最小版本
**Labels:** `area:theme`, `kind:implementation`, `priority:p0`

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

---

## Issue 042 — 编写 design token 文档
**Labels:** `area:theme`, `area:docs`, `kind:docs`, `priority:p1`

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

---

# Milestone M5 — Render 与 Text 基础能力

## Issue 043 — 定义 RenderBackend 抽象接口
**Labels:** `area:render`, `kind:architecture`, `priority:p0`

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

---

## Issue 044 — 定义 DrawCommand 到 RenderBackend 的适配层
**Labels:** `area:render`, `kind:implementation`, `priority:p0`

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

---

## Issue 045 — 实现 ThorVG 软件后端适配器
**Labels:** `area:render`, `kind:implementation`, `priority:p1`

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

---

## Issue 046 — 定义 text 模块的最小接口
**Labels:** `area:text`, `kind:architecture`, `priority:p0`

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

---

## Issue 047 — 实现基础文本测量能力
**Labels:** `area:text`, `kind:implementation`, `priority:p0`

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

---

## Issue 048 — 实现基础文本绘制能力
**Labels:** `area:text`, `kind:implementation`, `priority:p0`

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

---

## Issue 049 — 编写渲染策略文档
**Labels:** `area:render`, `area:docs`, `kind:docs`, `priority:p1`

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
- `Button` 与 showcase 的 `StatsSection` 已复用 `Pressable`
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

### 完成定��
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
**Status:** ⚠️ 部分完成 — 基础 Label 已可用，但状态语义与 typography role 仍未补齐

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
- 已实现文本、字体大小、颜色、对齐、preferred size 与真实字体绘制
- 落地文件：`widgets/src/nan_label.cppm`

### 未完成部分
- `disabled` / `error` / `required` 语义状态尚未建模
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
**Status:** ⚠️ 部分完成 — Button 已可点击使用，但 preset / size / colorVariant 仍未成型

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
- 已实现 text / on_click / disabled / hover / press 基础状态
- 通过 `Surface + Pressable + Label` 组合完成最小 Button
- 落地文件：`widgets/src/nan_button.cppm`、`widgets/src/nan_button.cpp`

### 未完成部分
- `preset`、`size`、`colorVariant` 仍未形成明确公开 API

---

## Issue 058 — 为 Button 加入 icon slot / 左右图标支持 ❌ 未完成
**Labels:** `area:widgets`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — Button 尚未暴露 icon slot 或左右图标 API

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

### 当前判断
- `Button` 当前只组合了 `Pressable + Label`
- widgets 层虽已有 `Icon` 组件，但未接入 `Button` 的公开接口与布局逻辑
- 未见 `left icon` / `right icon` / icon-text 排布 API

---

## Issue 059 — 实现 Button 的 preset 视觉映射 ❌ 未完成
**Labels:** `area:widgets`, `area:theme`, `kind:implementation`, `priority:p1`
**Status:** ❌ 未完成 — Button 目前只有一组直接颜色配置，未形成 preset 视觉语义

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

### 当前判断
- 目前 `ButtonColors` 只是单一颜色集，未见 `filled / tonal / outlined / ghost / link` 等 preset API
- 也未见从 theme 或 preset 枚举解析到样式的 resolver

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
- 落地文件：`widgets/src/nan_panel.cppm`

### 未完成部分
- optional header action 仍未实现
- 目前未见主线 showcase 或测试对 `Panel` 的实际消费，完成定义里的“内容分区验证”证据还不够

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

## Issue 063 — 定义 Page 抽象与页面元数据模型
**Labels:** `area:app`, `kind:architecture`, `priority:p0`

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

---

## Issue 064 — 定义 Router 核心接口
**Labels:** `area:app`, `kind:architecture`, `priority:p0`

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

---

## Issue 065 — 实现 Router 最小版本
**Labels:** `area:app`, `kind:implementation`, `priority:p1`

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

---

## Issue 066 — 实现 PageHost / 内容区域承载容器
**Labels:** `area:app`, `kind:implementation`, `priority:p1`

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

---

## Issue 067 — 实现基础应用壳结构（导航 + 内容区）
**Labels:** `area:app`, `kind:implementation`, `priority:p1`

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

---

## Issue 068 — 设计 WindowShell 抽象（标题栏/边框/区域保留）
**Labels:** `area:app`, `area:runtime`, `kind:architecture`, `priority:p2`

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

## Issue 070 — 实现 showcase 页面注册机制
**Labels:** `area:showcase`, `area:app`, `kind:implementation`, `priority:p1`

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

---

## Issue 071 — 创建 Label showcase 页面
**Labels:** `area:showcase`, `area:widgets`, `kind:implementation`, `priority:p1`

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

---

## Issue 072 — 创建 Button showcase 页面
**Labels:** `area:showcase`, `area:widgets`, `kind:implementation`, `priority:p1`

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

---

## Issue 073 — 创建 Panel/Card showcase 页面
**Labels:** `area:showcase`, `area:widgets`, `kind:implementation`, `priority:p2`

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

---

## Issue 074 — 在 showcase 中加入主题切换能力
**Labels:** `area:showcase`, `area:theme`, `kind:implementation`, `priority:p1`

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

---

## Issue 075 — 编写 showcase 开发策略文档
**Labels:** `area:showcase`, `area:docs`, `kind:docs`, `priority:p1`

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

---

# Milestone M10 — 稳定性、测试与后续扩展准备

## Issue 076 — 为 runtime/widget/event 增加单元测试
**Labels:** `area:runtime`, `area:tests`, `kind:test`, `priority:p1`

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

---

## Issue 077 — 为 theme 模块增加测试
**Labels:** `area:theme`, `area:tests`, `kind:test`, `priority:p1`

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

---

## Issue 078 — 为 widgets 基础控件增加测试
**Labels:** `area:widgets`, `area:tests`, `kind:test`, `priority:p1`

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

---

## Issue 079 — 建立 API 冻结策略文档
**Labels:** `area:docs`, `kind:docs`, `priority:p2`

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

---

## Issue 080 — 调研脚本/DSL authoring 边界需求
**Labels:** `area:app`, `area:docs`, `kind:architecture`, `priority:p2`

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

---

## Issue 081 — 调研 render backend 扩展路线（OpenGL/WebGPU/Vulkan）
**Labels:** `area:render`, `area:docs`, `kind:architecture`, `priority:p2`

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

---

# 推荐创建顺序

## 第一批（必须先开）
1. Issue 001 — 建立正式项目目录骨架
2. Issue 005 — 定义 foundation 层基础枚举与通用类型
3. Issue 006 — 实现基础几何类型
4. Issue 008 — 设计并实现 Application 生命周期最小接口
5. Issue 009 — 设计 Window 最小接口
6. Issue 011 — 定义统一 Event 类型体系
7. Issue 012 — 实现 Widget 树基础结构
8. Issue 016 — 设计 Scene/DrawCommand 中间层
9. Issue 018 — 定义 reactive 模块设计说明
10. Issue 019 — 实现 State<T> 最小版本

## 第二批（尽快跟进）
11. Issue 021 — 实现 Effect 自动依赖追踪
12. Issue 022 — 实现 EffectScope 生命周期管理
13. Issue 025 — 实现 Prop<T>
14. Issue 029 — 定义 layout 协议
15. Issue 031 — 实现 Row 容器
16. Issue 032 — 实现 Column 容器
17. Issue 037 — 定义主题系统核心语义枚举
18. Issue 038 — 定义 primitive tokens
19. Issue 041 — 实现 ThemeManager 最小版本
20. Issue 046 — 定义 text 模块最小接口

## 第三批（形成最小可用 UI）
21. Issue 047 — 实现基础文本测量能力
22. Issue 048 — 实现基础文本绘制能力
23. Issue 050 — 实现 Surface primitive
24. Issue 051 — 实现 Pressable primitive
25. Issue 052 — 实现 Text primitive
26. Issue 055 — 实现 Label 控件
27. Issue 057 — 实现 Button 控件最小版本
28. Issue 069 — 建立 showcase 应用基础入口
29. Issue 071 — 创建 Label showcase 页面
30. Issue 072 — 创建 Button showcase 页面

---

# 后续建议

如果需要，我下一步还可以继续帮你生成两种内容中的任意一种：

1. **每个 issue 的标准 GitHub issue 模板版**  
   也就是每条都改写成：
    - Title
    - Summary
    - Motivation
    - Scope
    - Out of Scope
    - Definition of Done

2. **按"可并行开发"重新分组的 issue board 版本**  
   比如分成：
    - Runtime Lane
    - Reactive Lane
    - Layout Lane
    - Theme Lane
    - Widgets Lane
    - Showcase Lane

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
| 006 | 实现基础几何类型 Point / Size / Rect（Insets 待完成） | 2026-04-22 | `3b26ebf`, `1ad8630` |
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
