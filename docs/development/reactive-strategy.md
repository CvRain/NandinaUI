# 响应式策略

> 状态：已校正（采用 Angular signal 命名）
> 来源：语义吸收自旧 C++ `archive/docs/reactive-strategy.md`，命名从 React 风格（State/Effect）切换为 Angular 风格（signal/computed/effect）。

## 目的

定义 reactive 子系统的设计与行为语义，作为 runtime / widgets / app 数据驱动更新的规范基础。
目标是保留旧版已验证的核心思想（依赖追踪、batch、scope 自动清理），同时把语义、生命周期与边界写清楚。

## 命名说明

本版本响应式原语统一对齐 Angular signal：

| 概念 | 本版本 | 旧版 |
|------|--------|------|
| 可写状态 | `Signal(T)` / `signal(v)` | `State` |
| 派生值 | `Computed(T)` / `computed(fn)` | `Computed` |
| 副作用 | `effect(fn)` / `Effect` | `Effect` |
| 副作用作用域 | `EffectScope` | `EffectScope` |
| 可写派生 | `linkedSignal` | — |
| 批处理 | `batch(fn)` | `batch` |

## 设计原则

1. **可预测**：任何 signal → effect 的传播都应可重现并有边界。
2. **最小 API**：只暴露满足常见 UI 场景的原语，复杂能力靠组合。
3. **故障安全**：effect 抛错不破坏全局追踪上下文，不泄漏资源。
4. **所有权清晰**：响应式对象生命周期明确，组件销毁时自动解绑。
5. **组合友好**：组件边界优先接收只读视图，而不是可写 signal。

## 核心原语

### Signal(T)
可读写的单值状态容器。
- `get()`：读取（在追踪上下文中会注册依赖）。
- `set(v)` / `update(fn)`：写入，触发依赖它的 computed / effect 的去重调度。
- `asReadonly()`：返回只读视图，用于组件输入，避免向子组件泄漏写权限。

### Computed(T)
由函数计算得出的派生值，自动追踪其内部读取的 signal。
- 求值策略：初期建议 **eager**（在 flush 时统一重算，实现简单、易测），性能需要时再切 lazy-on-get。

### effect / EffectScope
- `effect(fn)`：注册副作用，依赖变化后重新调度执行；执行期间建立依赖追踪上下文。
- `EffectScope`：管理一组 effect / 订阅的生命周期，scope 结束时自动断开并释放。

### linkedSignal
可写的派生 signal：默认值由源 computed 计算，但允许显式覆盖。用于「依赖输入但可临时改写」的场景。

### batch(fn)
- 块内所有 `set` 只做标记，不立即触发 effect。
- 块结束统一 flush：对所有失效节点去重并按依赖顺序执行一次。
- 用途：一次状态更新涉及多个 signal 时，避免短暂不一致或多次重绘。

## 依赖追踪与执行模型

- **追踪上下文**：effect / computed 执行期间设置 current tracker；`Signal.get()` 在该上下文中注册依赖。
- **变更传播**：`set()` 标记依赖节点为 dirty，本身不立即执行（除非未在 batch 中且实现选择即时 flush）。
- **执行顺序**：按依赖 DAG，先算被依赖节点，再算依赖节点；用 queue + visited 去重。
- **重入保护**：effect 执行期间若再次触发自身，用「正在执行」标记防止重复入列与无限递归。
- **异常安全**：effect 出错时先恢复并清理追踪上下文，再向上传播错误，内核结构保持一致。

## 线程模型

- 初期单线程（UI 线程）：所有 signal / effect / computed 的修改与 flush 在主线程进行。
- 设计上保持「追踪逻辑」与「调度执行」分层，为未来 worker offload 留口。

## 生命周期与所有权

- `Signal(T)` 的拥有者负责生命周期；订阅必须可断开。
- `EffectScope` 作为组件生命周期一部分：组件销毁时 `scope.clear()` 自动断开 effect / 订阅。
- 只读视图（`asReadonly()`）持有非 owning 引用时，调用方须保证源 signal 生命周期不短于接收方。

## 与 widgets / 组件层的集成

- 组件输入优先接收只读视图，而不是可写 signal。
- 组件内部状态用 `Signal(T)` 并绑定其 `EffectScope`，确保卸载时自动清理。
- 渲染路径：effect 或组件 build 在依赖变化时标记 widget dirty，render 在 flush 后重生成绘制命令。

## 最小测试与验收

- `Signal`：set/get、订阅触发、版本递增。
- 只读视图：只读、无写。
- `Computed`：依赖追踪正确、依赖变化重算、同 batch 内不重复算。
- `effect`：读取注册依赖、依赖变化重跑、同 batch 内单次执行。
- `EffectScope`：clear 时断开全部订阅。
- `batch`：连续 set 触发单次 flush。
- 异常安全：effect 抛错后追踪上下文恢复，其它 effect 仍执行。

## 落地顺序

见 [重写路线图](roadmap.md) M1：按 Signal → 追踪上下文 → effect/EffectScope → Computed → batch → 只读视图/linkedSignal 推进，每步独立可测。
