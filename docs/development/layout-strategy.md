# 布局策略

> 状态：已校正
> 来源：改写自旧 C++ `archive/docs/layout-strategy.md`，结论在 Zig 重写中继续有效。

## 目标

布局的首要目标不是「支持最复杂的布局」，而是：

- 让开发者不再手工计算组件位置。
- 让 widgets 内部自动完成 slot 排布。
- 让页面装配层只描述结构，而不是几何公式。

## 三层模型

布局按三层理解，各层边界独立演进：

### 1. 语义层
面向 widgets 与页面装配的布局原语：`Row` / `Column` / `Stack` / `Padding` / `gap` / `align` / `justify` / 组件内部 slot 布局。
这层决定开发者看到的 API，保持框架自身语义，不暴露底层求解器细节。

### 2. 协议层
布局过程中需要稳定的中间语义：`Constraints`（min/max width/height）、preferred size、child spec、
measure / layout 职责边界、layout result / content bounds。
这层是 NandinaUI 自己必须定义清楚的核心边界，未来是否接入第三方求解器不应改变它对上提供的语义。

### 3. 求解层
真正计算几何结果的底层求解器：
- 简单容器使用自有轻量求解。
- 未来复杂 flex 容器可引入 Yoga 等作为可插拔求解后端。

换句话说，**Yoga 应进入求解层，而不是直接成为语义层**。

## 为什么当前不 Yoga-first

当前 widgets 语义 API、constraints / slot 语义、theme 消费都还在收口。
若现在让 Yoga 成为核心，会出现两个问题：

1. 上层控件 API 被 Yoga 的底层模型反向塑形。
2. 未稳定的 widgets / theme / layout 协议被一起耦合，导致反复返工。

因此先稳定自有协议层，为 Yoga 预留接入位。

## Yoga 接入前置条件

只有同时满足下列条件，才把 Yoga 从「预留」推进到「主线评估」：

- 自有布局在复杂 flex / flow 场景频繁出现补丁式扩展。
- `basis` / `wrap` / 高频 shrink / 复杂 min-max 约束成为 widgets 与真实页面的高频需求。
- widgets 与 showcase 结构语义已稳定，不会再被底层求解模型反向塑形。
- 回归测试已足以保护「替换求解层而不改变语义层」的重构。

## measure / layout 协议草案

```
measure(constraints: Constraints) -> Size      // 自底向上求 preferred size
layout(bounds: Rect) -> void                   // 自顶向下分配最终几何
```

- `Constraints`：`{ min_width, max_width, min_height, max_height }`。
- 容器先 measure 子节点，再按自身规则分配 bounds，最后递归 layout。
- 几何计算只发生在 layout 层内部，禁止泄漏到 widgets / app 业务代码。
