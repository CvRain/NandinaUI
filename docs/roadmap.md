# Roadmap

> 状态校正（2026-05）：主线代码已经显著超出纯规划阶段。runtime、reactive、layout、widgets、app、showcase 均已有实际实现；下面的 roadmap 更适合作为“当前收口重点 + 后续阶段顺序”理解，而不是“从零开始的待建清单”。

## M0：文档与规划 ✅ 已完成

**目标**
- 完成项目重启文档体系与仓库骨架
- 清理旧 QML 主线实现残留

**完成定义（DoD）**
- 核心规划文档齐备且互不冲突
- `main` 不再包含旧组件实现与示例应用

## M1：runtime + reactive ⚠️ 核心闭环已完成

**目标**
- 建立基础 runtime 生命周期与事件调度
- 建立 State/Effect/Prop 等响应式最小闭环

**DoD**
- 能构建最小节点树并触发可追踪的状态更新

**依赖**
- M0

**当前判断**
- runtime 生命周期、事件分发、Widget 树已落地
- reactive 主体能力已落地并有测试覆盖
- 仍未完成的主要是 Scene/DrawCommand 中间层，以及少量 reactive 边界测试补齐

## M2：layout 协议收口 + theme foundation ⚠️ layout 主线已完成，theme 继续推进

**目标**
- 完成 layout 主线从协议到底层消费面的收口
- 稳定 constraints / preferred size / 自动布局的核心协议
- 建立 token/theme schema 与应用机制

**DoD**
- layout 已接通 `constraints -> measure/layout -> root reflow -> widgets/showcase -> regression tests` 主线
- theme 至少具备 token / palette / manager 到主线控件的稳定消费路径

**依赖**
- M1

**说明**
- 当前阶段采用“自有布局协议优先，Yoga 接入位预留”的策略。
- Yoga 不作为当前 layout 核心，但应在后续复杂 flex 容器中作为可评估的求解后端进入主线。

**当前判断**
- `LayoutCore`、`LayoutContainer`、root reflow、widgets 自动布局收口与 showcase 主路径回归已经接通，layout 当前阶段的主线目标已完成
- 布局相关自动化回归面已覆盖 backend、helper widgets、PageHost、showcase shell 与主页面结构
- 仍未进入本阶段完成态的部分主要来自 theme：token / palette / manager 到 widgets 的统一消费尚未收口
- Yoga 仍不是当前核心任务；下一次评估应建立在复杂 flex / basis / wrap /更高频 min-max 约束需求真正出现之后

## M3：first widgets ⚠️ 已有可运行实现，继续收口

**目标**
- 落地第一批基础组件（如 Label/Button/Input 的最小集合）
- 验证语义 API 与 theme 解耦
- 让基础控件内部具备自动布局能力，而不是依赖页面层手工摆放

**DoD**
- 组件可在统一状态与主题体系下运行

**依赖**
- M2

**当前判断**
- Surface / Pressable / Label / Button / Panel / Card 等已存在实际实现
- 目前的主要问题已经从“控件内部手工 frame 计算”转向 theme 语义消费、widgets 专项测试与更完整的控件契约
- 因此 M3 的重点不再是等待 layout 主线成型，而是继续完成 primitive / control 收口，并在稳定测试面上扩展下一批控件

## M4：app shell + router/page ⚠️ 部分完成

**目标**
- 建立应用开发层，包含页面组织、路由与窗口控制协同
- 恢复 showcase 作为验证载体

**DoD**
- 具备最小多页面应用骨架，验证 app authoring 体验

**依赖**
- M3

**当前判断**
- app 层已有 `NanAppWindow`、`NanComponent`、`Node/Ref/mount` 第一版 authoring API
- `NanPage` / `NanRouter` / `NanPageHost` 已落地，showcase 已打通 sidebar 导航 + PageHost 内容区的最小多页面骨架
- 当前未完成项主要转为“收口型”工作：showcase registry 单一数据源、app 导航测试覆盖、更通用的 shell / route 能力与 authoring 体验验证

## M5：render abstraction / text / scriptability 🚧 早期

**目标**
- 完成渲染后端抽象强化
- 推进文本能力与脚本接入策略验证（TypeScript/Lua 等）

**DoD**
- 形成可评估的渲染与脚本扩展方案
- 明确下一阶段实现优先级

**依赖**
- M4

**当前判断**
- render 抽象仍未建立正式 Scene/DrawCommand 中间层
- text 已有基础测量/绘制能力，但完整 text layout 体系仍早期
- scriptability 仍主要停留在边界与 DSL 方向讨论

## 当前建议顺序

1. 以当前 layout 主线为稳定底座，优先推进 primitive / control 收口、widgets 专项测试与 theme 统一消费。
2. 继续把 showcase 和自动化测试当作结构与协议回归面，而不是把 showcase 重新退回成临时布局补丁层。
3. 只在复杂 flex / basis / wrap / 更高频 min-max 约束成为主线需求后，再正式评估 Yoga 作为复杂求解后端的接入点。

