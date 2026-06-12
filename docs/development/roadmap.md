# Zig 重写路线图

> 配套阅读：[架构](architecture.md)。本文回答「应该从哪个方向开始重写」。


## 总原则

严格沿依赖方向自底向上：**先有稳定底座，再往上叠加**。
每一步都以「可编译 + 有单元测试 + `zig build test` 全绿」为完成标准，
避免重演旧版「实验性实现语义模糊」带来的返工成本。

## 推荐起点：reactive（响应式核心）

foundation 几何/颜色已经落地并有测试。下一个该动的不是 render，也不是 widgets，
而是 **reactive**。理由：

1. 它是整个数据驱动 UI 的「心脏」，runtime / widgets / app 全都依赖它。
2. 它纯逻辑、无平台依赖，最容易在纯单元测试下打磨语义，反馈快。
3. 旧版 reactive 语义已被验证过（见 `archive/docs/reactive-strategy.md`），
   重写时有清晰的语义参照，风险低、收益高。

先把内核语义钉死，后续 render/runtime/widgets 才有稳固的依赖。

## 阶段顺序

### M0 —— 骨架与 foundation ✅ 已完成
- 分层目录与 `root.zig` 聚合导出就位。
- foundation 的 Point / Size / Rect / Insets / Color 已实现并测试。
- `zig build run` / `zig build test` 可运行。

### M1 —— reactive 核心闭环 ✅ 基本完成（linkedSignal 待补）
> 命名采用 Angular signal 风格（`signal` / `computed` / `effect`），取代旧版 React 风格的 State/Effect。
>
> 实现采用显式调度图 `Graph`（无全局状态、多实例隔离），Push 失效 + Pull 取值：
> computed 惰性重算、effect 由调度队列执行，菱形依赖 glitch-free，动态依赖自动重追踪。

按以下子步骤推进，每步独立可测：
1. ✅ `Signal(T)`：init / get / peek / set / update / 版本号 / `asReadonly()`。
2. ✅ 依赖追踪上下文：`Graph` 在 effect/computed 执行期记录被读取的 source，自动建立双向边。
3. ✅ `effect(g, ctx, fn)` + `EffectScope`：依赖变化时重跑；`dispose` / `scope.deinit` 自动解绑。
4. ✅ `Computed(T)` / `computed(g, T, ctx, fn)`：惰性派生值，自动追踪依赖。
5. ✅ `batch(g, ctx, fn)`：批量 set 合并为一次 flush（支持嵌套）。
6. 🚧 只读视图 `asReadonly()` 已落地；`linkedSignal`（可写派生）待后续按需补充。

**完成定义**：能构建 signal→computed→effect 依赖图，set 触发可预测、无重复的重算，
并有覆盖依赖追踪、batch、scope 清理、菱形/动态依赖的测试。当前 `zig build test` 全绿（42 个测试）。


### M2 —— render 抽象 + layout 协议 ✅ 已完成
- ✅ `render`：`DrawCommand`（fill_rect / fill_rounded_rect / draw_text / push_clip / pop_clip）、
  `Scene` 命令缓冲（可复用）与 `Backend` 接口（vtable）已落地，并提供可断言命令序列的
  `RecordingBackend` 内存后端。showcase 新增 `render-scene` demo 展示一帧渲染产物。
- ✅ `layout`：`Constraints` 协议层 + 三套纯函数求解器 —— `flex`（盒子模型 column/row/stack，
  对标 Qt Widget）、`flow`（流式折行）、`anchors`（QML anchors 定位）。showcase 新增 `layout-box` demo
  展示三套求解器输出与「布局 → 渲染」链路。
- 两者互相独立，都只依赖 foundation。

### M3 —— theme + text
- ✅ `theme`：primitive token（spacing / radius / border / elevation / opacity / typography）、
  语义调色板（ColorRole + light/dark Scheme）、`Theme` 聚合与 resolver 已落地。默认
  主题类似「global.css」可被开发者整体/逐字段覆写。showcase 新增 `theme` demo。
- 🚧 `text`：Font 句柄、`measureText` —— layout 需要它来确定文本尺寸。接口须把「约束宽度
  + 换行 + 行数上限 + 截断」作为一等公民（防文字溢出组件）。⬅️ 下一步

### M4 —— runtime
- Node 树（owning 子节点 + handle 访问）、事件类型与分发、
  `mark_dirty → reflow → repaint` 主循环骨架。
- 此时 reactive / render / layout 已就位，runtime 才能把它们串成闭环。

### M5 —— widgets
- primitives：Surface / Pressable。
- controls：Label / Button / Panel / Card。
- 组件统一接收只读输入（只读 signal 视图），内部状态用 `Signal` 并绑定 `EffectScope`。


### M6 —— app + showcase
- App / Page / Router / 统一挂载入口与 Ref/Handle/Key 访问机制。
- 把 `src/main.zig` 从烟雾程序演进为真正的多页面 showcase。

## 关键决策（重写时先定）

- **分配器策略**：内核 API 显式接收 `std.mem.Allocator`，不藏全局分配器。
- **Computed 求值**：建议初期 eager（flush 时统一重算，实现简单、易测），
  性能需要时再切 lazy-on-get。
- **线程模型**：初期单线程（UI 线程）；追踪逻辑与调度执行分层，为未来 worker offload 留口。
- **错误处理**：用 Zig error union 取代旧版异常；effect 出错要恢复追踪上下文、不破坏内核一致性。

## 每步的纪律

- 一次只推进一层 / 一个原语，保持 `zig build test` 全绿。
- 新增公共 API 必须同时写 test。
- 不把几何/坐标计算泄漏到 widgets/app —— 由 layout 统一负责。
