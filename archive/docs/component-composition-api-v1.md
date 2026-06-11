# 无显式 move 的组件组合 API（V1）

> 状态：已校正（2026-05）  
> 当前判断：V1 authoring API 已经部分落地，不再只是接口提案；但它仍处于“可用 + 持续收口”阶段，不应被误解为最终冻结形态。

## 1. 文档定位

本文件不再尝试逐条重新发明 `Node` / `Ref<T>` / builder 接口，而是记录两类信息：

- 当前主线里已经可以直接依赖的 V1 authoring 能力
- 仍未收口、因此不能提前假定稳定的差异点

阅读顺序建议是：

1. 先把这里当作 authoring API 的现状说明
2. 再结合 `app/src/nan_application.cppm` 与 `tests/app/test_app_authoring*.cpp` 看真实语义
3. 如果文档与源码冲突，以源码与测试为准

## 2. 当前已落地的 V1 子集

### 2.1 公共入口

当前主线已经存在统一的 authoring 入口模块：

```cpp
export module nandina.app.authoring;
```

该入口当前已承担：

- `Node` / `Ref<T>` / `Children`
- `mount(Node)` 与窗口侧 `set_root(Node)` 消费链路
- `row / column / stack / positioned / padding / expanded / sized_box / center / spacer`
- `label / button / card / panel` 等第一批工厂
- `create_shell(...)` / `setup_shell(...)` 这类 app 层组合能力

### 2.2 核心类型已经存在

当前不是“建议未来实现”，而是已经有以下一版实现：

- `Node`：可移动、可链式配置的 authoring 值对象
- `Ref<T>`：挂载后访问句柄，root 替换或销毁时会回收
- `Children`：通过 `children(...)` 聚合子节点，支持 lvalue / rvalue 混用

其中一个已经被测试锁住的关键语义是：

- `children(a, b, c)` 可以接受局部变量，不要求业务层显式写 `std::move`
- `.bind(ref)` 会在挂载时回填，在卸载时失效置空

### 2.3 V1 不是“通用 Builder 类”，而是 typed builder

当前实现不是文档早期草案里那种“`Node` + 一堆独立 `RowBuilder / LabelBuilder / ButtonBuilder` 名字”的拆法，而是：

- `Node` 承载通用布局/挂载能力
- `LabelNode` / `ButtonNode` 等派生 typed builder 承载组件专属链式 API
- 通过 C++23 deducing this 保持链式调用后仍返回派生类型

这意味着：

- `label("Title")` 不只是返回一个抽象 `Node`
- 它返回的是可继续 `.font(...)`、`.align(...)`、`.disabled(...)` 的 typed builder
- `button("OK")` 同理，可继续 `.variant(...)`、`.size(...)`、`.on_click(...)`

### 2.4 已有的高频 authoring 语义

当前可以把下面这些能力视为 V1 已实现子集：

- `mount(Node)`：把 authoring tree 挂成 `NanComponent::Ptr`
- `adopt(std::unique_ptr<NanWidget>)`：把既有 runtime/widget 对象桥接回 authoring tree
- `row(children(...))` / `column(children(...))` / `stack(children(...))`
- `.gap(...)` / `.padding(...)` / `.align_items(...)` / `.justify_content(...)`
- `.width(...)` / `.height(...)` / `.size(...)`
- `label("...")` / `button("...")` / `card(children(...))` / `panel(children(...))`
- `positioned(children(...))` 与 `anchor_*` 一组定位能力

其中 `.width(...)` / `.height(...)` / `.size(...)` 还有一个已经被测试覆盖的重要语义：

- 若当前节点不是 `SizedBox`，会自动包裹一层 `SizedBox`
- 连续调用不会重复双重包裹

## 3. 当前推荐写法

### 3.1 组合与布局

```cpp
using namespace nandina::app;

auto page = column(children(
    label("Overview")
        .font([](auto& f) {
            f.size(20).weight(nandina::text::NanFontWeight::bold);
        }),
    row(children(
        button("Run").variant(nandina::theme::ButtonVariant::outline),
        spacer(),
        label("Ready")
    ))
        .gap(12)
        .align_items(nandina::layout::LayoutAlignment::center)
))
    .gap(16)
    .padding(24)
    .width(360);
```

这类写法已经是主线推荐路线，而不是未来设想。

### 3.2 引用绑定

```cpp
using namespace nandina::app;

Ref<nandina::widgets::Button> run_button;

auto header = row(children(
    label("Sandbox"),
    spacer(),
    button("Run").bind(run_button)
));
```

挂载完成后，`run_button` 可直接访问真实组件；root 被替换或销毁后会失效。

### 3.3 根节点挂载

当前主线的两条典型路径是：

```cpp
auto component = mount(column(children(
    label("Hello"),
    button("OK")
)));
```

以及：

```cpp
window.set_root(create_shell(std::move(router), {.header_title = "My App"}));
```

也就是说，V1 已经不只是 `mount(...)` 的实验接口，窗口侧 authoring root 消费链路也已经打通。

## 4. 与早期草案相比已经变化的点

### 4.1 `Children` 的主入口已经是 `children(...)`

早期草案更偏向手写 `Children{...}` 或单独描述聚合类接口；当前实际推荐入口是：

```cpp
children(a, b, c)
```

而且它已经明确支持：

- 全 rvalue
- lvalue + rvalue 混用
- 空 children

### 4.2 `adopt(...)` 已成为重要桥接能力

V1 当前不是“只能通过纯 authoring 工厂创建节点”。对于仍保留 runtime 形态的对象，可以直接：

```cpp
adopt(std::make_unique<MyWidget>())
```

这对壳层、路由宿主、渐进迁移中的 legacy widget 很关键。

### 4.3 shell 级组合已经进入 authoring 层

`create_shell(...)` / `setup_shell(...)` 已经说明 authoring 不再只是一组“页面内部小工厂”，而是开始承担 app 级装配职责。

### 4.4 文档早期提到的某些名字不再是准确抽象

例如：

- 独立的 `RowBuilder / ColumnBuilder / ButtonBuilder` 类型名不是当前对外主叙事
- `Node` 的私有字段和内部绑定槽位也不适合再当作公共接口文档的一部分

这些实现细节后续仍可能继续调整。

## 5. 当前仍未收口的部分

以下内容不应被提前视为 V1 已稳定能力：

- `key(...)` 目前还没有形成 diff/reconcile 语义
- `Node` 的内部结构与 mounted wrapper 细节仍可能调整
- 并非所有 widgets 都已接入 typed builder authoring API
- `Input / Field` 等下一批表单控件尚未落地
- 完整 reconciler / virtual tree 仍然不是 V1 范围
- app 层 authoring 与脚本层 authoring 的映射关系还没有定稿

## 6. 对当前实现的直接指导

如果现在继续推进页面、showcase 或高层控件，建议遵循下面的边界：

1. 新页面优先写成 `build() -> mount(Node)`，而不是回退到手工 `add_child(std::move(...))`
2. 需要桥接现有 runtime widget 时优先使用 `adopt(...)`
3. 需要后续访问子组件时优先使用 `Ref<T>`，不要把局部变量当成所有权句柄
4. 新的 authoring 语义若没有测试，至少要补到 `tests/app/test_app_authoring.cpp` 或 `tests/app/test_app_authoring_qol.cpp`

## 7. 后续收口重点

当前更合理的下一步不是重写整套 V2 语法，而是继续沿 V1 收口：

1. 继续让更多控件消费 typed builder authoring
2. 用 forms / Input 垂直切片验证 `Ref<T>`、signals 和控件交互语义
3. 等 layout 主线稳定后，再决定哪些 authoring 语法应被视为可冻结接口

## 8. 相关文档

- [组件 Authoring 与挂载 API 设计](component-authoring-and-mounting.md)
- [布局策略](layout-strategy.md)
- [开发 Issue 清单](develop-issue.md)
- [Godot-like 开发范式（历史参考）](godot-like-authoring-draft.md)