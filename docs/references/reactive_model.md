# 响应式内部模型

本文面向维护者，说明 NandinaUI 响应式子系统的内部结构：依赖图由谁持有、依赖边何时建立与拆除、值变化如何在图中传播、effect 在什么时机执行，以及页面与组件作用域销毁时这些资源如何释放。面向应用开发者的读写用法与绑定教学见 Getting Started 的《用信号驱动界面更新》；本文不重复那部分内容，而是解释它背后的机制。

阅读代码时请以 `nandina/reactive/` 为准：`graph.hpp` / `graph.cpp` 是唯一的调度内核，`signal`、`computed`、`effect`、`scope`、`event`、`property` 都是围绕它组合出的门面。

## 为什么是一个显式 Graph

响应式系统最容易失控的地方是隐式全局状态：一个进程级的追踪上下文或调度队列会让测试相互污染，也让多窗口、多页面同时存在时难以推理。当前实现把全部调度状态收进一个显式的 `Graph` 对象——当前读取者、待执行队列、批处理深度、延迟域深度，以及它持有的 computed / effect 列表都挂在实例上。因此每个 Graph 彼此独立，单元测试只要各自建一个 Graph 就天然隔离。

这也决定了所有权的基本分工：`Signal` 由调用方持有（栈上局部变量或上层结构体的成员），`Computed` 与 `Effect` 的所有权交给 `Graph`（`adopt()` 之后返回裸指针作为稳定句柄）。由于 `Signal` 析构时要向 Graph 解绑依赖边，它必须比 Graph 先销毁；这是使用该系统时最主要的生命周期约束。

## 节点、边与两种角色

图里只有两类节点。`Source` 是可被订阅的数据源，带一个单调递增的 `version` 和非拥有的 `subs` 订阅者列表；`Reactor` 是能在依赖变化时被通知的一方，带 `state`（`clean` / `dirty`）、非拥有的 `sources` 列表，以及 `in_queue`、`disposed` 两个调度标志。

`Signal` 内嵌一个 `Source`。`Computed` 则同时是两者：作为 `Reactor` 它订阅上游，作为 `Source` 它被下游订阅。这种「一个对象两个身份」是理解派生值传播的关键。

依赖边是双向的：`track()` 同时把 source 记进 reactor 的 `sources`、把 reactor 记进 source 的 `subs`，并在同一 reactor 单次执行内对重复读取去重（同一个 source 只建一条边）。解绑也必须双向进行：`clear_reactor_deps()` 在 reactor 重新执行前清空旧边，`detach_source()` 在 source 析构时摘掉所有订阅者。维护这套对称性是改动这块代码时最容易出错的地方。

## 读写语义：追踪、peek 与等值写入

追踪上下文不是线程局部变量，而是 Graph 上的 `current_reader_`。`enter_read()` / `ReadScope` 以 RAII 方式在进入时保存先前的 reader、退出时恢复，因此嵌套读取能正确回到外层。`Signal::get()` 与 `Computed::get()` 都会调用 `track()`：只有 `current_reader_` 非空时才建立依赖，因此在 effect / computed 之外读取不会留下任何订阅关系。

`peek()` 是明确的不追踪读口。Signal 的 `peek()` 直接返回引用；Computed 的 `peek()` 虽然不建立依赖，但若当前是 dirty 仍会重算，以保证返回最新值。

写入方面，`set()` 先做变化检测：当 `T` 满足 `equality_comparable` 时，值相等就整体返回，既不递增 version 也不通知订阅者；不满足该概念的类型则保守地每次都通知。`update(fn)` 就地修改后总是通知，因为它无法判断 `fn` 是否真的改变了值——需要精确控制通知次数时应优先用 `set()`。`ReadSignal` 只是持有源 `Signal*` 的非拥有视图，把读权限传出去而不泄漏写权限。

## 推入失效 + 拉取求值

系统采用「推入失效、拉取取值」的混合模型，而不是在写入时急式计算。`notify_source()` 递增 version，然后对订阅者快照逐个调用 `invalidate_reactor()`：把节点标为 dirty，并执行该节点自己的 `on_invalidate()`。该函数在节点已是 dirty 时直接返回，这个幂等性正是菱形依赖不会重复传播、effect 不会重复入队的根据。

两个身份的失效行为不同。`Effect::on_invalidate()` 只把自己排进 pending 队列；`Computed::on_invalidate()` 把自己标 dirty 后继续向它的订阅者传播，但本身不重算。Computed 的重算只发生在 `get()` / `peek()` 读到 dirty 时：先清空旧依赖边，把状态置回 clean，再进入追踪上下文调用计算函数，结果缓存起来。由此有两个直接后果：创建 computed 不会执行计算函数，首次读取才计算；未被读取的 computed 在依赖变化后根本不会重算。

惰性求值还顺带解决了拓扑顺序与中间态问题。读取派生值必然先读取它本次执行所需的依赖，因此「先算被依赖节点、再算依赖节点」不靠显式排序，而由调用顺序保证；菱形结构下 effect 只会看到一致的最终值，不会观察到 glitch。

## effect 的调度：flush、batch 与 tick 提交点

`Effect` 是 eager 的一侧：`make_effect()` 创建后立即执行一次以建立初始依赖，此后由队列调度。`enqueue()` 用 `in_queue` 去重，保证同一节点在一波待执行队列里只出现一次。

`notify_source()` 在传播结束后才判断是否立即 flush：只有不在 batch 中、不在延迟域中、当前也没有正在 flush 时才调用 `flush()`。因此单独一次 `set()` 是同步的——写入返回时 effect 已经跑完。这正是测试里「写入后立刻断言」能够成立的原因。

`batch()` 用 RAII guard 递增 / 递减 `batch_depth_`，支持嵌套，只在最外层退出且延迟域也为空时 flush 一次；即使 fn 抛异常，guard 析构也保证退出批量模式。`flush()` 按索引遍历 pending 队列（执行过程中可能继续入队），同一波内每个 effect 最多执行一次：如果 effect 在执行中再次使自己失效，`enqueue()` 会把它放进 `next_pending_`，留到下一波。这条规则避免了自反馈 effect 在同一波里锁死 UI 线程，代价是这类更新要多一个波次才稳定。作为兜底，单次 flush 的执行数超过 10000 会抛出 `reactive effect cascade exceeded flush limit`，而不是静默卡死。

应用层还有更外层的提交点：`NanWindow::tick()` 在帧开始时通过 `graph.defer_effects()` 进入延迟域，在 `FramePhase::reactive` 阶段调用 `commit()`。整个 input / tasks / process 阶段写入的值仍然同步可读，但 effect 被合并到该阶段统一跑一波，随后才进入动画与布局。`DeferredEffects` 的析构不 flush，只恢复自动调度，因此中途放弃延迟域不会丢队列内容，只是留待下次写入或显式 `flush()` 处理。

## ReactiveScope 的持有与清理顺序

`ReactiveScope` 是页面与组件把「一组响应式资源」绑到对象生命周期上的容器。它按类型分别持有四类资源：signal（通过 `SignalHolderBase` 做类型擦除后以 `unique_ptr` 保存）、computed、effect，以及 `Event` 订阅。它的清理顺序是有意设计的：先清订阅，再 dispose effect，再 dispose computed，最后释放 signal。先断开外部事件，是为了避免拆除过程中还有回调重新进入这个作用域；先断开 effect / computed 的上游依赖，再销毁它们所读取的 signal，避免留下指向已释放 source 的边。

注意 ownership 的层次：computed 与 effect 的内存实际由 Graph 持有，`ReactiveScope` 只保存句柄并在清理时调用 `dispose()`，由 Graph 从自己的持有列表中移除并释放。

`lifetime()` 返回一个 `weak_ptr<void>` 代际标记，用来保护安装在控件上的应用回调：构建作用域被清空后，回调不应再进入。`clear()` 先让当前代际失效，再清理资源，然后重新生成一个新代际以便作用域复用；析构则是先失效代际、再清理、不再续期。这个顺序保证回调守卫总是比它保护的响应式值先失效。

## 拆除路径：dispose 与整体析构

单个节点的提前释放统一走 `dispose_reactor()`，并且是幂等的：已 dispose 的节点直接返回。它先置 `disposed` 并清空双向边，然后把该指针从 `pending_`、`next_pending_` 和 `ran_in_flush_` 中抹掉，最后才从 Graph 的持有列表中移除并真正析构。先清队列这一步不能省——flush 期间队列里存的是裸指针，任何存活到下一轮的已释放节点都会变成 use-after-free。`Effect::dispose()` 与 `Computed::dispose()` 都只是转发到这里，因此「先 dispose 再让 Graph 析构」与「从不 dispose」都是安全的。

整体析构是另一条路径。`Graph::~Graph()` 先置 `tearing_down_ = true`，再清空持有列表；`Computed` 与 `Effect` 的析构函数看到这个标志就跳过解绑上下游。此时所有节点都将一起释放，逐个解绑既无必要，又会读到正在被释放的邻居。这也是为什么跨节点解绑必须由 Graph 主导，而不能写进 reactor 的析构里。

参与这套机制的类型都不可拷贝也不可移动（Graph、Reactor、Signal、ReactiveScope、EffectScope 均删除了拷贝与移动）。对外暴露的都是裸指针句柄，其有效期由 `dispose()` 与 Graph 的生命周期共同界定，不存在所有权转移的中间状态。

## 维护约定

改动这一层时，有几条不变量需要一并保持：

- 依赖边必须成对增删。新增任何建立依赖的路径，都要同时确认 `clear_reactor_deps()` 与 `detach_source()` 能覆盖它；
- 失效传播必须保持幂等。`invalidate_reactor()` 的「已 dirty 即返回」是去重与防重复传播的唯一根据；
- 执行用户代码前一律先清空旧依赖边并进入读取作用域。动态依赖的正确性完全依赖这两步，任何「优化掉」的尝试都会让分支切换后的旧依赖继续触发；
- `dispose()` 必须可重入、可在 flush 中调用；
- 不要在 `nandina/reactive/` 里重新引入全局或线程局部状态。追踪上下文刻意放在 Graph 上，正是为了让多实例隔离与单线程假设保持显式。

## 与 BuildContext / PageContext 的归属

Graph 的粒度最粗：它由应用持有并跨页面共享。`PageContext` 持有路由信息、各类服务，以及一个页面级的 `ReactiveScope`；`ui()` 用同一个 graph 和这个 scope 构造出 `BuildContext`，因此页面内所有 `ui.signal*()`、`ui.computed()`、`ui.effect()` 都落在这个页面作用域里。`NanRouter::push_page()` 为每个页面帧创建独立的 `ReactiveScope` 并随帧保存，页面出栈时随之清理。

`BuildContext::with_scope()` 只替换 scope，graph、主题与资源保持页面级。这个区分对应两类派生场景：`make<T>()` 为自定义组件建立自己的 `ReactiveScope`，并以 shared_ptr 的自定义删除器保证「先清作用域、再析构节点」；条件区域与列表项则各自持有作用域，在重建或离开场景树时清理。延迟域的存在也解释了为什么构建期间的大量写入不会逐次触发绑定重算。

作用域还可以继续细分：`IfRegion` 与 `ListView` 内部持有 `EffectScope` / `ReactiveScope`，在 `on_exit_tree()` 中整体清理，并在重建时先清掉旧的绑定作用域再重新安装。`NodeBuilder::bind()` 把绑定 effect 装进 `bind_scope()` 指定的作用域，而 `guard_callbacks()` 单独绑定页面级代际——控件回调的守卫寿命与绑定 effect 的寿命是两个维度。

## event 与 property 在模型中的位置

`Event` 不属于依赖图：它没有 Source / Reactor，也不持有 Graph，只是一个共享状态里的订阅者列表。`subscribe()` 返回 move-only 的 `Subscription`，其析构即断开；`emit()` 先对订阅者取快照，再逐个确认订阅仍然存在后才调用，因此「回调里断开自己或别人」是安全的。订阅状态由 shared_ptr 持有，`Subscription` 晚于 `Event` 销毁也不会悬空。

`Property` 是控件侧的适配层，同样不是图节点：它是一个普通值，加一个 `Apply` 落点和一个 `changed` 事件。`set()` 做等值检测，先应用落点再发出变更事件。它接入响应式的方式是 `bind(EffectScope&, Source&)`：注册一个 effect 去读 source 并调用 `set()`，也就是把「signal 驱动控件属性」变成一条普通依赖边。绑定捕获的 source 与 `Property` 自身一样是非拥有引用，调用方需保证生命周期覆盖。

## 已知限制与未收口点

- **单线程**：`nandina/reactive/` 中没有任何 mutex / atomic / 线程设施，Graph 的队列、批处理深度与去重标志都假定同一线程内顺序调用。应用层的 `UiDispatcher`、`AsyncScope` 属于 app 层，不是响应式的同步机制；从后台线程直接 `set()` 不受支持。
- **重入语义**：effect 在执行中触发的自身失效被推迟到下一波，而不是被禁止；硬保证只有「每波每 effect 最多一次」。批量与延迟域都是 Graph 上的深度计数器，可以嵌套，但不可跨线程。
- **异常**：effect 抛出的异常沿 flush 向上传播；`flush()` 在异常路径上会复位所有 `in_queue` 标志、清空队列并恢复 `flushing_`，RAII 读取作用域负责恢复追踪上下文。调度器不会停留在破损状态，但该波次尚未执行的 effect 会被丢弃。
- **`version` 目前只是观测值**：`Source::version` 的注释提到「供更精细的脏检查使用」，但当前脏判定只看 `clean` / `dirty` 标志，`version` 仅通过 `source_version()` 暴露给测试；把它用于增量比对仍是尚未收口的方向。
- **历史文档中的部分原语并未进入当前实现**：v1 的集合细粒度事件与 v2 讨论过的可写派生 signal 在当前 `nandina/reactive/` 下都没有对应文件。设计新能力时应以本目录实际存在的原语为准。
