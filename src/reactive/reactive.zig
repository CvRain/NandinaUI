//! reactive —— 响应式核心层
//!
//! 提供 State / Computed / Effect / EffectScope / Prop 等响应式原语与依赖追踪、
//! batch 调度。设计语义见 docs/ARCHITECTURE.md 与旧文档 reactive-strategy。
//!
//! 依赖方向：reactive 仅依赖 foundation（如有需要）。
//!
//! 现状：骨架占位。下一步落地 State(T) + 依赖追踪上下文 + Effect，作为整个
//! 数据驱动更新的最小闭环。
const std = @import("std");

// TODO(reactive): 实现 State(comptime T)
// TODO(reactive): 实现 tracking context（thread-local current tracker）
// TODO(reactive): 实现 Effect / EffectScope
// TODO(reactive): 实现 Computed(comptime T)
// TODO(reactive): 实现 batch(fn)
// TODO(reactive): 实现 Prop(comptime T)（静态值 vs 响应式源）

test {
    std.testing.refAllDecls(@This());
}
