# reactive

响应式核心层（Angular signal 风格）。纯逻辑，**仅依赖 foundation**。

## 计划 API

| 概念 | 形态 | 说明 |
|------|------|------|
| `Signal(T)` / `signal(v)` | 可写状态 | `get()` / `set(v)` / `update(fn)` / `asReadonly()` |
| `Computed(T)` / `computed(fn)` | 派生值 | 自动追踪依赖，依赖变化重算 |
| `effect(fn)` / `EffectScope` | 副作用 | 依赖变化重跑；scope 结束自动解绑 |
| `linkedSignal` | 可写派生 | 默认由源计算，允许覆盖 |
| `batch(fn)` | 批处理 | 块内多次 set 合并为一次 flush |

## 计划用法

```zig
const count = signal(i32, 0);
const doubled = computed(i32, fnReadingCount);
effect(fnUsingDoubled);
count.set(5); // effect 重跑，doubled = 10
```

## 设计要点

- 单线程（UI 线程）；追踪逻辑与调度分层，为未来 worker 留口。
- effect 抛错要恢复追踪上下文，不破坏内核一致性。
- 组件输入优先接收只读视图，内部状态用 `Signal` 并绑定 `EffectScope`。

## 状态

🚧 骨架。设计语义见 [响应式策略](../../docs/development/reactive-strategy.md)。
