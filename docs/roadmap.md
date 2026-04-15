# Roadmap

## M0：文档与规划（当前阶段）

**目标**
- 完成项目重启文档体系与仓库骨架
- 清理旧 QML 主线实现残留

**完成定义（DoD）**
- 核心规划文档齐备且互不冲突
- `main` 不再包含旧组件实现与示例应用

## M1：runtime + reactive

**目标**
- 建立基础 runtime 生命周期与事件调度
- 建立 State/Effect/Prop 等响应式最小闭环

**DoD**
- 能构建最小节点树并触发可追踪的状态更新

**依赖**
- M0

## M2：layout + theme

**目标**
- 提供基础布局模型（Row/Column/Stack）
- 建立 token/theme schema 与应用机制

**DoD**
- 可在 runtime 中稳定执行基础布局与主题切换

**依赖**
- M1

## M3：first widgets

**目标**
- 落地第一批基础组件（如 Label/Button/Input 的最小集合）
- 验证语义 API 与 theme 解耦

**DoD**
- 组件可在统一状态与主题体系下运行

**依赖**
- M2

## M4：app shell + router/page

**目标**
- 建立应用开发层，包含页面组织、路由与窗口控制协同
- 恢复 showcase 作为验证载体

**DoD**
- 具备最小多页面应用骨架，验证 app authoring 体验

**依赖**
- M3

## M5：render abstraction / text / scriptability

**目标**
- 完成渲染后端抽象强化
- 推进文本能力与脚本接入策略验证（TypeScript/Lua 等）

**DoD**
- 形成可评估的渲染与脚本扩展方案
- 明确下一阶段实现优先级

**依赖**
- M4

