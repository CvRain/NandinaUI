# Reactive Strategy

Status: Draft — intended as the canonical design for the project's reactive core.

Purpose
-------
定义正式项目的响应式子系统设计与行为语义，作为实现与后续扩展（components / widgets / bindings / tests）的规范基础。目标是保留 nandina_experiment 已验证的关键思想，同时把语义、生命周期与边界写清楚，避免“实验性实现语义模糊”带来的后续成本。

Scope
-----
本文件覆盖：
- 基本设计原则与约束
- 核心原语：State, ReadState, Computed, Effect, EffectScope, Prop, StateList, EventSignal/Connection
- batch（批处理）语义
- 依赖追踪与执行模型（单线程首要，线程扩展边界）
- 异常处理与追踪恢复
- 生命周期 / 所有权 / 自动清理
- 与 widget/component 层的集成约束
- 最小测试与验收条件

不在本文件范围：
- 跨线程分布式状态同步（作为后续扩展）
- 脚本层细节（TS/Lua）绑定协议（另文讨论）
- 持久化/undo/redo 策略

Principles
----------
1. Predictability — 语义必须可理解、可测、可测试。任何 state->effect 传播都应可重现并有边界。
2. Minimalist API — 只暴露满足常见 UI 场景的原语；复杂能力通过组合构建。
3. Fail-safe — 异常不会破坏全局追踪上下文或导致内存泄漏。
4. Ownership clarity — Reactive 对象的生命周期必须明确（谁持有/谁观测），组件销毁时自动解绑。
5. Composition-friendly — Prop 与 ReadState 用于组件边界，组件优先接收只读输入。

Core primitives (概览)
--------------------

- State<T>
    - 可读写的单值状态容器。
    - API 要点：get(), set(new), on_change(conn), as_read_only() -> ReadState<T>, version()
    - set() 会触发依赖它的 Computed / Effect 的无重复调度（由 batch 决定合并行为）。

- ReadState<T>
    - 只读视图，暴露 get() 与订阅能力（on_change）。
    - 用于组件输入，避免向子组件泄漏可写权限。

- Computed<R> (派生值)
    - 由一个函数计算得出，自动追踪其内部读取的 State/ReadState。
    - 懒惰/及时策略可配置（实现初期建议采用 eager recompute 在变更 flush 时更新，或 lazy-on-get 的策略）；文档中要统一选择并实现。

- Effect (副作用)
    - 可注册副作用函数，该函数在其依赖的 State 发生变化后被重新调度执行。
    - Effect 的执行期间会建立“依赖追踪上下文”，读取的 State 自动建立依赖。
    - Effect 应支持一次性连接、取消、以及在异常后恢复追踪上下文。

- EffectScope
    - 管理一组 Effect/Connection 的生命周期。
    - 当组件销毁或 scope 结束时，自动断开订阅并释放资源。

- Prop<T>
    - 统一的组件输入封装。可以包装静态值或 ReadState<T>。
    - API：get(), is_reactive(), on_change(callback)（若为静态则返回 no-op 连接）
    - 组件应默认使用 Prop<T>/ReadState<T> 作输入参数，而不是直接接受 mutable State<T>。

- StateList<T>
    - 对 vector-like 集合的可观测包装，提供细粒度变更事件（Inserted, Removed, Updated, Moved, Reset）。
    - 事件模型允许控件只处理必要的增量更新，而不是整体重建。

- EventSignal<Args...> / Connection / ScopedConnection
    - 事件广播机制（用于纯事件，如 click、keydown），与响应式数据流分离。
    - Connection 支持 disconnect; ScopedConnection 在析构时自动断开。

Dependency tracking & execution model
------------------------------------
- Tracking Context: 在 Effect/Computed 的执行期间，设置 thread-local（或执行上下文）current_tracker。State::get() 在该上下文中会注册依赖。
- When state changes:
    - set() 标记依赖的 invalidators（Effect/Computed）为 dirty。它本身**不立即执行**（除非未在 batch 中且实现选择即时 flush）。
    - batch 会将一批 set() 合并，最后 flush 时对每个 invalidator 执行一次（合并同一 invalidator 的多次触发）。
- Order:
    - 保证“读依赖”形成的 DAG 中，先计算被依赖节点，再计算依赖节点（topological 或稳定重算顺序）；实现上可用简单 queue + visited 去重。
- Reentrancy:
    - Effect 在执行期间若再次触发相同的 effect，需避免无限递归（可通过标记当前正在执行来防止再次入列）。
- Exception safety:
    - Effect 执行出现异常时，应：
        1. 恢复并清理 tracking context（以免破坏后续追踪）
        2. 将异常向上抛出或以可配置方式记录，但不得让内部数据结构处于不一致状态。
    - EventSignal::emit 的语义建议为：“继续调用其他回调并在最后重抛第一个异常（若需要）”，以保证订阅者不会因为某个回调异常而阻止其它订阅者接收事件（参考实验仓库中实践）。

Batch semantics
---------------
- batch(f):
    - 在 batch 块中，所有 set() 操作只做标记，不立即触发 effect。
    - batch 结束后统一 flush：对所有 invalidators ���重并按依赖顺序执行。
    - 设计原则：保持最低惊讶原则（single flush -> consistent final state seen by effects）。
- 用例：
    - 界面状态更新时将相关状态写在同一 batch 内，避免短暂不一致或多次重绘。

Lifecycle & Ownership
---------------------
- State<T> 的拥有者负责生命周期。State 不应在被观察者销毁后继续存在逻辑依赖（订阅必须可断开）。
- EffectScope 作为组件生命周期的一部分：组件销毁时调用 scope.clear()，自动断掉 effect/subscription。
- Prop<T> 持有非 owning 引用时（绑定到 ReadState 或 State），必须文档化“caller 必须保证源 State 的生命周期不短于接收组件”，通常父组件管理该 State 的生命周期。
- 为避免常见悬空/所有权错误，推荐：
    - 组件 add_child(...) 返回稳定非 owning 指针/handle，组件持有子组件的 owning 指针（如 unique_ptr）。
    - 不允许直接把临时 State 或组件指针传入长期订阅（除非使用 ScopedConnection 并和宿主 scope 绑定）。

Threading model
---------------
- 初期实现：single-threaded（UI thread）语义。所有 State/Effect/Computed 的修改与 flush 在主线程进行。
- 扩展策略（后续）：
    - 若需跨线程 set/read，应定义明确的同步策略（atomic snapshots 或将跨线程操作转到主线程队列执行）。
    - Reactive core 需要在设计上保留将追踪上下文与调度器分离的可能性（即：追踪逻辑与调度执行分层），以便未来做线程调度或 worker offload。
- Implementation note: 即使当前单线程，也要用线程安全的数据结构或明确注释扩展边界，防止日后误用。

API sketches (pseudo-C++)
-------------------------
下面为常见原语的示意 API（非最终实现签名，仅为便于实现和讨论）：

```cpp
// State
template<typename T>
class State {
public:
    State() = default;
    State(T initial);

    const T& get() const;           // read
    void set(const T& v);           // write, triggers invalidation

    ReadState<T> as_read_only() const;

    // connect returns Connection which can be disconnected
    Connection on_change(std::function<void(const T&)> cb) const;

    std::size_t version() const;
};

// ReadState
template<typename T>
class ReadState {
public:
    const T& get() const;
    Connection on_change(std::function<void(const T&)> cb) const;
    bool is_reactive() const;
};

// Prop wrapper
template<typename T>
class Prop {
public:
    Prop(T value);                       // static
    Prop(ReadState<T> source);           // reactive source

    const T& get() const;                // unified access
    bool is_reactive() const;
    Connection on_change(std::function<void(const T&)>) const;
};

// Effect + EffectScope
class EffectScope {
public:
    void add(std::function<void()> cleanup_or_effect);
    void clear();      // disconnect all in scope
};

class Effect {
public:
    Effect(std::function<void()> fn, EffectScope* scope = nullptr);
    void run();             // internal
    void dispose();
};

// batch
void batch(std::function<void()> fn);
```

Integration with widgets / components
-------------------------------------
- Components accept inputs as Prop<T> / ReadState<T> (只读优先原则)。
- Internal per-component state uses State<T>，并绑定其 EffectScope，确保组件卸载时自动清理。
- Rendering path:
    - Effect 或 component build() 在 state 改变时标记 widget dirty（mark_dirty）。
    - Render 系统定期（或在 flush 后）重新生成 scene draw commands。
- Avoid: 子组件直接接受可写 State<T>，避免跨组件写入导致 ownership/同步复杂。

Testing & validation
--------------------
最低测试用例（单元测试）：
- State: set/get, on_change fired, version increments
- ReadState: only read API, no write
- Prop: static vs reactive behavior, on_change semantics
- Effect: dependency tracking (reads register), re-run on dependency change, no double-run for same invalidator in single batch
- EffectScope: cleanup on clear, subscriptions disconnected
- StateList: Inserted/Removed/Updated/Move/Reset events in expected order
- batch: consecutive set() inside batch triggers single flush
- Exception safety: effect throws -> tracking context restored, other effects still run
- EventSignal: multiple subscribers, exception handling as designed

Deliverables / Definition of Done (DoD) for reactive core MVP
-------------------------------------------------------------
- Implementations for State, ReadState, Prop, Effect, EffectScope, StateList, batch available and unit-tested.
- Tracking context infrastructure with automatic dependency registration on State::get().
- batch semantics implemented and covered by tests.
- EffectScope integration with a mock component demonstrating automatic cleanup.
- Example small app: counter component using State + Effect + Prop demonstrating consistent behavior.

Migration notes (from nandina_experiment)
-----------------------------------------
- nandina_experiment 的实现已经验证了大量核心思想（State/Effect/Prop/StateList/batch）。正式项目应吸收成熟语义（尤其是 batch 与 EffectScope 的行为边界），但把 API 签名与 ownership 约定写清楚并纳入测试体系。
- 将实验性文件（分散的 .ixx 草稿）整合为单一、被测试并导出的模块是关键；历史副本可保留在 docs/archive，但不作为主实现依据。

Next steps (implementer guidance)
---------------------------------
1. 在 `reactive/` 下建立模块骨架与最小 public header/module（State/ReadState/Effect/Prop/StateList/Connection）。
2. 实现 tracking context（thread-local）及 State::get 注册逻辑。
3. 实现 batch 与 flush 调度（queue + visited 去重策略）。
4. 实现 Effect + EffectScope 与 basic Connection/ScopedConnection。
5. 编写单元测试覆盖前述行为（包括异常安全与 batch 场景）。
6. 编写一个小的 example（counter）集成验证：State -> Effect -> mark_dirty -> render loop。

Notes / open questions
----------------------
- Computed 的评估策略（lazy vs eager）需要在实现前做明确决策；建议初期用 eager 在 flush 时统一 recompute（实现简单且测试容易），后期若性能需要再切换为 lazy。
- 跨线程写入与读取的安全策略需在项目扩展阶段再定（最好在 reactive 模块设计时留下可插调度器/同步钩子）。
- Prop 的引用语义需在 API 文档里强调（非 owning pointer 的责任说明）。

References
----------
- 源自 nandina_experiment 的设计讨论与实现验证（参考 docs/reactive-design.md、docs/reactive-primitives.md、docs/report-1.txt、docs/meeting-notes-2026-04-10.md）。
- 本文档旨在把实验结果转化为适合长期维护的规范。
