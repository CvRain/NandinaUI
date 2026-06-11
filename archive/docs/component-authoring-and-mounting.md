# 组件 Authoring 与挂载 API 设计

> 状态：已校正（2026-05）  
> 当前判断：本文应视为 authoring 方向的原则文档，而不是“尚未落地”的纯提案。`Node` / `Ref<T>` / `children(...)` / `mount(Node)` / 第一批 typed builder 已有实现；但接口边界仍在继续收口。

> 目的：明确 NandinaUI 面向使用者的组件组合、挂载与引用模型，避免业务开发者显式处理坐标计算、子节点遍历与 `std::move` 所有权转移。

## 当前落地状态

当前主线已经具备：

- `nandina.app.authoring` 统一入口
- `Node` / `Ref<T>` / `Children` 与 `children(...)`
- `mount(Node)` 与窗口侧 `set_root(Node)` 消费链路
- `label()` / `button()` 等第一批 typed builder
- `row/column/stack/...` 以及 `.width/.height/.size` 的链式 authoring 语义

因此，本文更适合回答“为什么 authoring 要这样收口”，而不是重新发明具体签名；具体接口应参考 [无显式 move 的组件组合 API（V1）](component-composition-api-v1.md) 和现有测试。

## 背景

当前主线已经逐步把 showcase 与 widgets 内部的手工几何计算收回到 `Row / Column / Stack / Padding / Expanded / SizedBox` 等布局原语内部。

这证明了一个更大的方向：

- 使用者不应手写坐标公式
- 使用者不应手工遍历 child 做布局
- 使用者也不应显式思考组件所有权是否需要 `std::move`

如果业务层在写组件时仍然频繁出现下面这些代码，说明框架对外 API 还没有真正收口：

```cpp
child->set_bounds(x + ..., y + ..., ...);
for_each_child(...);
float cursor_y = ...;
row->add(std::move(button));
```

这些代码最多只能存在于：

- 少量基础布局原语内部
- 极少数低层自绘叶子控件内部

它们不应该成为框架使用者的日常 authoring 方式。

## 核心目标

### 1. 不让使用者手写坐标

业务层组件应描述结构与布局意图，而不是描述几何公式。

开发者应当表达：

- 这是一个 `Column`
- 这里有一个 `gap`
- 这里的内容要 `Expanded`
- 这里需要 `Padding`
- 这里是一个 `Stack`

而不是表达：

- `x + 16`
- `y + header_h + gap`
- `w - 32`
- `for child in children`

### 2. 不让使用者显式处理组件所有权

当前直接暴露 `std::unique_ptr<NanWidget>` 的组合方式，会把“组件树所有权转移”这个问题直接丢给业务开发者。

这会带来两个直接问题：

- 开发者需要判断某个组件到底该不该 `std::move`
- 如果保留局部变量又没有正确转移所有权，就可能让 authoring 逻辑与运行时对象树分裂

框架应该隐藏这一层细节，让使用者写的是“组件组合”，而不是“所有权编排”。

### 3. 让挂载入口更明确

根组件挂载应当是单一入口能力，而不是由业务代码自己理解：

- 什么时候创建 root component
- 什么时候把它交给窗口
- 什么时候同步尺寸

当前第一版已经落地的根入口是：

```cpp
return mount(column(children(
    label("Dashboard"),
    button("Run")
)));
```

以及：

```cpp
window.set_root(create_shell(std::move(router), {.header_title = "My App"}));
```

长期理想目标仍然可以继续收敛成更高层的表达，例如：

```cpp
app.mount(DashboardPage());
```

或：

```cpp
window.set_root(DashboardPage());
```

而不是暴露一串底层桥接细节。

### 4. 把“挂载”和“后续访问”解耦

很多业务代码之所以会保留局部组件变量，本质上不是为了管理所有权，而是为了后续还能访问这个组件、改状态、挂动画或读值。

因此，框架不能只隐藏 `std::move`，还必须提供独立的组件引用机制，例如：

- `Ref<T>`
- `Handle<T>`
- `Key`

否则使用者仍然会绕回“先保存对象，再手工接树”的旧模式。

## 设计原则

### 1. 使用者写的是 authoring tree，不是 runtime ownership tree

对使用者来说，组件组合应该首先是一棵“描述树”。

框架内部可以继续维护真实的 `NanWidget` 运行时树，但不应要求使用者显式参与这棵树的所有权构造。

### 2. 业务层组件禁止手工布局协议细节

业务层 authoring 代码应避免：

- 手写 `cursor_x / cursor_y`
- 直接遍历 `child`
- 自己分配 `set_bounds`
- 手工同步绝对坐标

业务层应优先使用固定布局原语来表达结构。

### 3. 公共 API 不应强迫理解 move 语义

框架内部继续使用 `unique_ptr` 没问题，但不应让业务层高频出现：

```cpp
auto button = Button::create();
row->add(std::move(button));
```

这类写法暴露的是实现细节，不是面向使用者的最终 API。

### 4. 引用能力必须独立于所有权

“我想在后面拿到这个按钮”不应该等价于“我需要先把这个按钮保存在局部变量里”。

框架应该显式提供引用机制，而不是依赖使用者通过变量生命周期间接维持访问能力。

## 推荐模型

NandinaUI 后续建议形成三层模型。

### 第一层：面向使用者的 Authoring API

这是业务开发者直接编写的接口层。

它的要求是：

- 尽量不出现显式 `std::move`
- 组合方式接近声明式结构表达
- 可以自然表达布局与挂载

当前更贴近主线的 authoring 形态示例：

```cpp
auto page = column(children(
    label("Overview"),
    row(children(
        button("Run"),
        spacer(),
        label("Ready")
    )).gap(12)
)).padding(16);
```

这一层已经不只是长期目标，而是主线正在收口中的“第一版组件描述树”。

### 第二层：运行时 Widget Tree

框架内部仍可继续使用：

- `NanWidget`
- `std::unique_ptr`
- 真实 child ownership
- runtime/event/layout integration

这棵树是框架的运行时实现细节，不必直接暴露给业务开发者。

### 第三层：引用与交互句柄

如果业务层在挂载后仍需访问某个组件，应通过显式引用机制完成：

```cpp
Ref<Button> add_button;

auto header = HeaderBar(
    Button("+").bind(add_button)
);

add_button->set_text("Save");
```

这里的关键是：

- 组件挂载负责构树
- `Ref` 负责后续访问
- 两者不再通过 `std::move` 或局部变量耦合

## 分阶段建议

### 第一阶段：隐藏显式 move，但保留现有运行时树

短期不必立刻推翻现有 `NanWidget + unique_ptr` 体系。

可以先提供一层 builder / helper / factory 风格的 authoring API，把 `std::move` 封装起来。

例如从：

```cpp
auto row = nandina::layout::Row::Create();
auto button = Button::create();
row->add(std::move(button));
```

逐步收敛到：

```cpp
auto row = row(
    button("OK"),
    spacer(),
    label("Title"));
```

或者：

```cpp
auto row = Row::Create()
    ->children(
        button("OK"),
        spacer(),
        label("Title"));
```

这一阶段的价值是：

- 减少业务代码里的 `std::move`
- 减少“到底谁拥有谁”的心智负担
- 不必一次性重写全部 runtime 模型

### 第二阶段：明确根组件挂载入口

在 app 层应逐步收敛成统一 mount 能力，例如：

```cpp
app.mount(DashboardPage());
```

或：

```cpp
window.set_root(DashboardPage());
```

这一步的重点不是命名本身，而是把：

- root component 创建
- root ownership 接管
- 尺寸同步
- 生命周期接入

统一收口成单一入口。

### 第三阶段：引入 `Ref / Handle / Key`

在业务开发需要“挂载后仍访问子组件”的地方，使用句柄机制替代局部变量持有。

例如：

```cpp
Ref<RecentActivityCard> activity_ref;

auto page = DashboardPage(
    RecentActivityCard().bind(activity_ref));

activity_ref->set_pulse(t);
```

这一步可以显著减少：

- 为了后续访问而保留裸对象变量
- 因为引用需求而暴露所有权细节

### 第四阶段：从 Builder 过渡到真正的 Element / Spec 层

长期如果 authoring 体验要进一步接近现代 UI 框架，应考虑把使用者写的树从“运行时 widget 对象”升级为“组件描述对象”。

例如：

- `Element`
- `WidgetSpec`
- `NodeSpec`
- `View`

此时业务开发者写的只是描述，真正的 `NanWidget` 树在 build/mount 阶段由框架实例化。

这是最彻底的方向，但不要求当前阶段一次完成。

## 对公共 API 的直接约束

下面这些约束建议逐步成为面向使用者的默认规则：

1. 业务层组件禁止手写 `cursor_x / cursor_y`
2. 业务层组件禁止遍历 child 决定布局
3. 业务层组件禁止直接依赖绝对坐标分发布局
4. 业务层组件优先通过布局原语表达结构
5. 公共组合 API 不应要求业务层显式 `std::move`
6. 子组件后续访问应通过 `Ref / Handle / Key` 实现，而不是保留所有权变量

## 非目标

### 1. 现在就完全移除 `unique_ptr`

短期没有必要为了 authoring API 优化而推翻现有 runtime 所有权模型。

内部继续使用 `unique_ptr` 是合理的，问题在于它不应成为使用者高频接触的公共接口。

### 2. 现在就一次性完成完整 diff/reconcile 框架

长期如果引入 `Element / Spec` 层，可能会自然发展出更完整的构树与更新机制。

但当前阶段最关键的问题不是 reconciliation，而是：

- 不再手写坐标
- 不再手工处理 move
- 不再手工挂载 child

## 与当前主线的关系

当前主线已经在做的工作包括：

- 将 showcase 的页面壳层收进 `Row / Column / Expanded / Padding`
- 将 widgets 内部的 slot 布局收进布局原语
- 逐步消除业务层手工几何与手工 child 遍历

这份文档是在这些实现工作之上，把下一阶段真正需要稳定的“面向使用者的 API 目标”明确下来。

换句话说，当前重构不是终点，而是在为后续 authoring API 收口铺路。

## 下一步接口化草案

在这份原则性文档之上，第一版更具体的接口签名与使用方式草案已整理为：

- [无显式 move 的组件组合 API（V1）](component-composition-api-v1.md)

该文档重点回答：

- `Node` 在 V1 中长什么样
- `Ref<T>` 如何承担挂载后访问
- `row(...)` / `column(...)` / `button(...)` 这类工厂函数第一版建议长什么样
- `mount(...)` / `set_root(Node)` 应如何收敛根节点挂载

## 相关文档

- [无显式 move 的组件组合 API（V1）](component-composition-api-v1.md)
- [架构规划](architecture-plan.md)
- [布局策略](layout-strategy.md)
- [编码与 API 规范](coding-and-api-conventions.md)
- [Godot-like 开发范式（历史参考）](godot-like-authoring-draft.md)
- [开发 Issue 清单](develop-issue.md)