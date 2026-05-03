# Layout Strategy

> 状态：草案（Draft）
> 目的：明确 NandinaUI 当前阶段的布局开发策略，以及 Yoga 的接入时机与边界。

## 背景

当前主线已经具备：

- runtime / reactive 的最小闭环
- showcase 应用壳层与若干展示组件
- Row / Column / Stack / 基础 flex 的初步能力

但 showcase 和部分 widgets 仍然包含较多手工位置与尺寸计算。这类工作对最终使用者来说应当是透明的，应该由组件内部布局与容器布局自动完成，而不是由页面装配层反复手算。

因此，layout 现在已经不是“以后再做”的模块，而是需要进入主线收口的基础能力。

## 已决定

### 1. 现在就继续推进 layout

当前最需要解决的问题不是“能否支持最复杂的布局”，而是：

- 让开发者不再手工计算组件位置
- 让 widgets 内部能自动完成 slot 排布
- 让 showcase 和未来 app 页面装配层只描述结构，而不是描述几何公式

因此，layout 应当从现在开始持续推进，并与 widgets 语义化收口并行演进。

### 2. 当前阶段不采用 Yoga-first

虽然 Yoga 是成熟且能力完整的布局库，但当前项目仍处于以下阶段：

- widgets 语义 API 还在收口
- preferred size / constraints / slot layout 语义仍在稳定
- theme 消费与控件内部自动布局还没有完全接通

如果现在直接让 Yoga 成为 layout 核心，很容易出现两个问题：

1. 上层控件 API 被 Yoga 的底层模型反向塑形。
2. 还未稳定的 widgets / theme / layout 协议会一起被耦合，导致后续反复返工。

因此，当前阶段不采用“Yoga-first”路线。

### 3. 现在就为 Yoga 预留接入位

不直接 Yoga-first，不代表忽略 Yoga。

从现在开始，layout 的协议层就应该为未来接入 Yoga 预留空间，使其可以在后续作为“复杂容器的求解后端”进入主线，而不是以临时外挂的形式插入。

## 推荐策略

NandinaUI 的 layout 建议按三层理解：

### 1. 语义层

面向 widgets 与页面装配的布局原语：

- Row
- Column
- Stack
- Padding / Insets
- gap / align / justify
- 组件内部 slot 布局

这一层决定开发者看到的 API，必须保持框架自身语义，不直接暴露 Yoga 风格细节。

### 2. 协议层

布局过程中需要稳定的中间语义：

- constraints
- preferred size
- child spec
- measure / layout 职责边界
- layout result / content bounds

这一层是 NandinaUI 自己必须定义清楚的核心边界。未来是否接入 Yoga，不应改变这一层对上提供的语义。

### 3. 求解层

负责真正计算几何结果的底层求解器。

当前建议：

- 简单容器继续使用自有轻量求解
- 未来复杂 flex 容器可以引入 Yoga 作为求解后端

换句话说，Yoga 应进入求解层，而不是直接成为语义层。

## 分阶段计划

### 第一阶段：当前阶段

目标：消除上层手工布局。

优先任务：

- 让 Button、Panel、Card 等基础控件内部完成自动布局
- 让 showcase 区域组合更多依赖 Row / Column / Stack / Split 这类原语
- 把业务层中的固定坐标、尺寸联动与偏移计算尽量收回组件内部

这一阶段的重点不是“布局能力最强”，而是“开发者不再亲自计算几何”。

### 第二阶段：基础协议稳定后

目标：明确 layout 协议边界，并引入可替换求解器概念。

进入这一阶段的标志：

- widgets 内部 slot 布局模式开始稳定
- preferred size / constraints / flex factor 语义不再频繁变动
- showcase 页面装配层已显著减少手工布局代码

在此阶段可以开始实验：

- 布局后端抽象
- 特定 flex 容器接入 Yoga 的可行性验证
- shrink / basis / min-max / wrap 等复杂语义的落点

### 第三阶段：复杂布局需求成为常态时

目标：将 Yoga 引入部分主线容器。

适合正式引入 Yoga 的信号包括：

- 多层嵌套 flex 布局频繁出现
- shrink / basis / wrap / min-max constraints 成为高频需求
- 自有轻量布局逻辑开始出现明显补丁化扩展
- showcase 或真实页面已经证明复杂响应式布局是常见场景，而不是少数例外

在这一阶段，Yoga 应优先进入：

- 复杂 flex / flow 容器
- 多约束自适应容器

而不是替换所有基础容器。

## 不建议

### 1. 现在就全面切换到 Yoga-first

原因：当前协议和控件语义尚未稳定，直接切换会放大耦合与返工成本。

### 2. 未来“简单布局自研，复杂布局临时直接调 Yoga”

原因：这会形成两套布局心智模型，长期维护成本很高。

正确做法应是：

- 上层语义统一
- 底层求解器可分层演进

## 待验证

### 1. Yoga 的首个主线落点

当前方向是让 Yoga 优先进入复杂 flex / flow 容器，但第一批真正接入的容器范围仍需在后续实现中验证。

需要结合以下信号判断：

- 多层嵌套 flex 是否已经成为主线高频需求
- shrink / basis / wrap / min-max constraints 是否已经频繁进入 widgets 与 showcase
- 自有轻量布局逻辑是否开始明显补丁化

### 2. 协议层需要补齐到什么粒度

当前已经明确需要稳定：

- constraints
- preferred size
- child spec
- measure / layout 职责边界
- layout result / content bounds

但这些中间模型最终应以多细的公开结构暴露，仍需在后续 widgets 接入与复杂布局实验中验证。

### 3. 基础原语与复杂容器的分界线

当前方向是：

- Row / Column / Stack / Padding / slot layout 保持自有语义
- 更复杂的 flex 求解再考虑引入 Yoga

但“复杂容器”的判定线仍需通过真实页面与 showcase 演进来验证，而不是仅凭预设结论。

## 对当前主线的直接指导

当前阶段，layout 应当作为 widgets 收口工作的一部分推进，而不是晚于 widgets。

也就是说，下一步最合理的主线不是：

- 先把 widgets 全做完，以后再碰 layout

而是：

- 一边收口 Button / Label / Panel / Card 的语义 API
- 一边补足这些控件内部的自动布局能力
- 同时逐步减少 showcase 页面装配层中的手工几何计算

Page / Router / AppShell 等更高层抽象，建议放在这一轮“基础控件 + 内部布局”稳定之后推进。

## 相关文档

- [Architecture Plan](architecture-plan.md)
- [Roadmap](roadmap.md)
- [Reactive Strategy](reactive-strategy.md)
- [Develop Issue](develop-issue.md)
