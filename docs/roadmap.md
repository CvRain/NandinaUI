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

## M2：layout + theme 🚧 进行中

**目标**
- 提供基础布局模型（Row/Column/Stack）
- 稳定 constraints / preferred size / 自动布局的核心协议
- 建立 token/theme schema 与应用机制

**DoD**
- 可在 runtime 中稳定执行基础布局与主题切换
- 上层装配代码不再大量手工计算几何结果

**依赖**
- M1

**说明**
- 当前阶段采用“自有布局协议优先，Yoga 接入位预留”的策略。
- Yoga 不作为当前 layout 核心，但应在后续复杂 flex 容器中作为可评估的求解后端进入主线。

**当前判断**
- Row / Column / Stack / Spacer / SizedBox 等基础能力已落地
- `NanWidget` 已具备 measure/layout 协议雏形，但 layout 主线仍未完全收口
- 当前最优先工作不是新增更高层抽象，而是先完成 Layout 主线收口：
	- `LayoutCore` 接入 `NanConstraints`
	- `LayoutContainer` 切到两阶段驱动
	- 建立 root reflow 闭环
	- 清理 widgets / showcase 中的手工布局
- theme 仍处于早期，建议在 layout 收口稳定后继续推进 token 到 widgets 的统一消费

## M3：first widgets ⚠️ 已有可运行实现，待收口

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
- 目前的主要问题不是“有没有控件”，而是部分控件内部仍保留手工 frame 计算
- 因此 M3 的前置重点是消费 M2 的 layout 收口成果，而不是继续铺更多新控件

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

1. 先完成 layout 主线收口，再继续扩大 widgets、router/page、theme 的表面积。
2. 以 showcase 和自动化测试作为 layout 重构的回归面，而不是继续把 showcase 当作临时布局补丁承载层。
3. 等 constraints / measure / reflow / widgets 收口稳定后，再评估 Yoga 作为复杂求解后端的正式接入点。

