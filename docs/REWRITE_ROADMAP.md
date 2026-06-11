# Zig 重写路线图

> 配套阅读：`docs/ARCHITECTURE.md`。本文回答「应该从哪个方向开始重写」。

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

### M1 —— reactive 核心闭环 ⬅️ 下一步
按以下子步骤推进，每步独立可测：
1. `State(comptime T)`：get / set / 版本号 / 变更订阅。
2. 依赖追踪上下文（tracking context）：在 effect/computed 执行期记录被读取的 State。
3. `Effect` + `EffectScope`：依赖变化时重跑；scope 析构自动解绑。
4. `Computed(comptime T)`：派生值，自动追踪依赖。
5. `batch(fn)`：批量 set 合并为一次 flush。
6. `Prop(comptime T)`：统一组件输入（静态值 vs 响应式源）。

**完成定义**：能构建 State→Computed→Effect 依赖图，set 触发可预测、无重复的重算，
并有覆盖依赖追踪、batch、scope 清理的测试。

### M2 —— render 抽象 + layout 协议
- `render`：定义 `DrawCommand`（FillRect / FillRoundedRect / DrawText / PushClip / PopClip）、
  `Scene` 命令缓冲与 `Backend` 接口（vtable）。先做一个可断言命令序列的内存后端用于测试。
- `layout`：定义 `Constraints` 与 `measure(constraints) -> Size` 协议，落地 Row / Column / Stack。
- 这两者互相独立，可并行，但都只依赖 foundation。

### M3 —— theme + text
- `theme`：颜色 token、spacing / radius / typography scale、Theme resolver。
- `text`：Font 句柄、`measureText` —— layout 需要它来确定文本尺寸。

### M4 —— runtime
- Node 树（owning 子节点 + handle 访问）、事件类型与分发、
  `mark_dirty → reflow → repaint` 主循环骨架。
- 此时 reactive / render / layout 已就位，runtime 才能把它们串成闭环。

### M5 —— widgets
- primitives：Surface / Pressable。
- controls：Label / Button / Panel / Card。
- 组件统一接收只读输入（Prop / ReadState），内部状态用 State 并绑定 EffectScope。

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
