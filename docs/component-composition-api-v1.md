# 无显式 move 的组件组合 API（V1 草案）

> 状态：草案（Draft）
> 目的：在不推翻现有 `NanWidget + std::unique_ptr` 运行时模型的前提下，给出第一版面向使用者的组件组合 API 形态，减少手写坐标、手工 child 遍历与显式 `std::move`。

## 1. 适用范围

本草案关注的是第一版可落地 authoring API，而不是最终形态。

V1 的目标是：

- 先把 `std::move + add_child` 从业务层高频代码里隐藏起来
- 先把布局原语组合收口成统一的 authoring 入口
- 先给出根节点挂载与子组件引用的统一模型

V1 不要求一次性解决：

- 完整 diff / reconcile
- 完整 Element 虚拟树
- 所有 widgets 的声明式重写

## 2. 设计选择

V1 建议采用“Builder / Node 包装层”路线，而不是直接要求业务层继续操作 `std::unique_ptr<NanWidget>`。

也就是说：

- 使用者看到的是 `Node`、`Ref<T>`、`mount(...)`
- 框架内部仍然可以继续构建真实的 `NanWidget` 树

这条路线的优点是：

- 可以兼容当前 runtime 和 widgets 实现
- 可以先改善 authoring 体验，再决定是否走向更完整的 `Element / Spec` 层

## 3. 推荐模块位置

V1 建议新增一个面向使用者的公共 authoring 入口模块：

```cpp
export module nandina.app.authoring;
```

职责：

- 提供 `Node`
- 提供 `Ref<T>`
- 提供 `row(...)` / `column(...)` / `button(...)` 等工厂函数
- 提供 `mount(...)` 或 `set_root(Node)` 这类统一挂载入口

建议依赖方向：

`layout + widgets + app` -> `nandina.app.authoring`

即 authoring 是 app 层的面向使用者封装，而不是 runtime 底层能力的一部分。

## 4. 核心公共类型

### 4.1 `Node`

`Node` 是 V1 的核心 authoring 值对象。它不是直接暴露给业务层的 `std::unique_ptr<NanWidget>`，而是一个可移动、可链式配置的“组件描述包装”。

建议接口：

```cpp
export namespace nandina::app {

class Node {
public:
    Node();
    Node(Node&&) noexcept;
    auto operator=(Node&&) noexcept -> Node&;

    Node(const Node&) = delete;
    auto operator=(const Node&) -> Node& = delete;

    [[nodiscard]] auto empty() const noexcept -> bool;

    template<typename T>
    auto bind(Ref<T>& ref) && -> Node;

    auto key(std::string_view value) && -> Node;

private:
    friend class NodeFactory;
    friend class MountBuilder;

    explicit Node(std::unique_ptr<nandina::runtime::NanWidget> widget);

    std::unique_ptr<nandina::runtime::NanWidget> m_widget;
    std::string m_key;
    void* m_bound_ref_slot{nullptr};
};

} // namespace nandina::app
```

V1 要点：

- `Node` 本身仍可移动，但业务层不需要显式写 `std::move`
- 业务层看到的是组合值对象，而不是 child ownership
- `Node` 内部最终仍可以持有真实 `unique_ptr<NanWidget>`

### 4.2 `Ref<T>`

`Ref<T>` 负责“挂载后访问”，与所有权解耦。

建议接口：

```cpp
export namespace nandina::app {

template<typename T>
class Ref {
public:
    Ref() = default;

    [[nodiscard]] auto get() const noexcept -> T*;
    [[nodiscard]] auto operator->() const noexcept -> T*;
    [[nodiscard]] auto operator*() const noexcept -> T&;
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    template<typename U>
    friend class Binder;

    T* m_ptr{nullptr};
};

} // namespace nandina::app
```

V1 不要求 `Ref<T>` 具备复杂生命周期语义；第一步只需要支持：

- mount 后自动回填
- root 销毁后失效置空

### 4.3 `Children`

为了避免 authoring API 出现大量模板歧义，建议提供轻量的 children 聚合类型。

```cpp
export namespace nandina::app {

class Children {
public:
    Children() = default;
    Children(std::initializer_list<Node> nodes);

    auto append(Node node) -> Children&;
    [[nodiscard]] auto take() && -> std::vector<Node>;

private:
    std::vector<Node> m_nodes;
};

} // namespace nandina::app
```

## 5. 布局原语 API 草案

V1 建议优先提供与当前 layout 能力一致的一组 authoring 工厂函数。

### 5.1 `row(...)`

```cpp
export namespace nandina::app {

[[nodiscard]] auto row(Children children = {}) -> Node;

class RowBuilder {
public:
    auto gap(float value) && -> Node;
    auto padding(float all) && -> Node;
    auto padding(float horizontal, float vertical) && -> Node;
    auto padding(float left, float top, float right, float bottom) && -> Node;
    auto align_items(nandina::layout::LayoutAlignment value) && -> Node;
    auto justify_content(nandina::layout::LayoutAlignment value) && -> Node;
};

} // namespace nandina::app
```

建议使用方式：

```cpp
using namespace nandina::app;

auto header = row({
    label("Dashboard / Overview"),
    spacer(),
    button("+")
})
    .padding(20.0f, 0.0f, 6.0f, 0.0f)
    .align_items(nandina::layout::LayoutAlignment::center)
    .gap(10.0f);
```

### 5.2 `column(...)`

```cpp
export namespace nandina::app {

[[nodiscard]] auto column(Children children = {}) -> Node;

class ColumnBuilder {
public:
    auto gap(float value) && -> Node;
    auto padding_top(float value) && -> Node;
    auto padding(float all) && -> Node;
    auto padding(float horizontal, float vertical) && -> Node;
    auto padding(float left, float top, float right, float bottom) && -> Node;
    auto align_items(nandina::layout::LayoutAlignment value) && -> Node;
    auto justify_content(nandina::layout::LayoutAlignment value) && -> Node;
};

} // namespace nandina::app
```

建议使用方式：

```cpp
auto content = column({
    stats_section(),
    middle_content_section(),
    bottom_content_section(),
    footer_section(),
})
    .padding_top(20.0f)
    .gap(20.0f)
    .align_items(nandina::layout::LayoutAlignment::stretch);
```

### 5.3 其他基础原语

```cpp
export namespace nandina::app {

[[nodiscard]] auto stack(Children children = {}) -> Node;
[[nodiscard]] auto padding(Node child) -> Node;
[[nodiscard]] auto expanded(Node child, int flex = 1) -> Node;
[[nodiscard]] auto sized_box(Node child) -> Node;
[[nodiscard]] auto center(Node child) -> Node;
[[nodiscard]] auto spacer(int flex = 1) -> Node;

} // namespace nandina::app
```

建议链式配置：

```cpp
auto action = sized_box(button("Run"))
    .width(96.0f)
    .height(32.0f);

auto body = padding(column({
    header,
    content,
}))
    .padding(20.0f, 0.0f);
```

## 6. 基础组件 API 草案

V1 不要求所有 widgets 都有完整 authoring DSL，但应先覆盖高频基础组件。

### 6.1 `label(...)`

```cpp
export namespace nandina::app {

[[nodiscard]] auto label(std::string_view text = {}) -> Node;

class LabelBuilder {
public:
    auto text(std::string_view value) && -> Node;
    auto font_size(float value) && -> Node;
    auto color(const nandina::NanColor& value) && -> Node;
    auto align(nandina::widgets::TextAlign value) && -> Node;
    auto vertical_align(nandina::widgets::TextVerticalAlign value) && -> Node;
};

} // namespace nandina::app
```

### 6.2 `button(...)`

```cpp
export namespace nandina::app {

[[nodiscard]] auto button(std::string_view text = {}) -> Node;

class ButtonBuilder {
public:
    auto text(std::string_view value) && -> Node;
    auto colors(const nandina::widgets::ButtonColors& value) && -> Node;
    auto on_click(std::function<void()> handler) && -> Node;

    template<typename T = nandina::widgets::Button>
    auto bind(Ref<T>& ref) && -> Node;
};

} // namespace nandina::app
```

### 6.3 `card(...)` / `panel(...)`

```cpp
export namespace nandina::app {

[[nodiscard]] auto card(Children children = {}) -> Node;
[[nodiscard]] auto panel(Children children = {}) -> Node;

class CardBuilder {
public:
    auto title(std::string_view value) && -> Node;
    auto bg_color(const nandina::NanColor& value) && -> Node;
    auto corner_radius(float value) && -> Node;
};

} // namespace nandina::app
```

## 7. 挂载 API 草案

V1 建议同时保留低层 `set_root_component(unique_ptr<...>)`，并增加面向 authoring 的高层入口。

### 7.1 `mount(...)`

```cpp
export namespace nandina::app {

[[nodiscard]] auto mount(Node root) -> NanComponent::Ptr;

} // namespace nandina::app
```

作用：

- 将 authoring `Node` 构造成真实 root component
- 完成 `Ref<T>` 绑定回填
- 返回可交给现有 `NanAppWindow` 的 `NanComponent::Ptr`

### 7.2 `NanAppWindow::set_root(Node)`

```cpp
export namespace nandina::app {

class NanAppWindow {
public:
    auto set_root(Node root) -> void;
    auto replace_root(Node root) -> void;
};

} // namespace nandina::app
```

建议使用方式：

```cpp
using namespace nandina::app;

auto root = column({
    header_bar(),
    expanded(dashboard_page()),
});

window.set_root(std::move(root));
```

这里可以接受 `Node` 作为值参数，调用者不需要显式 `std::move(child_widget)`；唯一可能出现的 `std::move(root)` 只发生在根值对象层，而不是每一个组件节点层。

如果后续要进一步消除这一层，也可以把 `set_root(Node)` 扩展为完全面向临时值的写法：

```cpp
window.set_root(column({ ... }));
```

## 8. `Ref<T>` 绑定语义草案

V1 建议采用最直接的 `.bind(ref)` 形式。

```cpp
using namespace nandina::app;

Ref<nandina::widgets::Button> add_button;
Ref<nandina::showcase::RecentActivityCard> activity_card;

auto header = row({
    label("Overview"),
    spacer(),
    button("+").bind(add_button),
});

auto page = column({
    header,
    recent_activity_card().bind(activity_card),
});
```

挂载完成后：

```cpp
if (add_button) {
    add_button->set_text("Run");
}

if (activity_card) {
    activity_card->set_pulse(0.5f);
}
```

V1 约束：

- `bind(ref)` 只能绑定到最终构造出的真实组件类型
- 重复 mount 或 replace_root 时，旧 `Ref<T>` 应失效置空后再回填
- `Ref<T>` 不是所有权容器，不负责生命周期延长

## 9. V1 使用示例

### 9.1 当前写法

```cpp
auto row = nandina::layout::Row::Create();

auto title = nandina::widgets::Label::create();
title->set_text("Overview")
    .set_font_size(10.0f);
row->add(std::move(title));

auto run = nandina::widgets::Button::create();
run->set_text("Run");
row->add(std::move(run));
```

### 9.2 V1 目标写法

```cpp
using namespace nandina::app;

Ref<nandina::widgets::Button> run_button;

auto header = row({
    label("Overview").font_size(10.0f),
    spacer(),
    button("Run").bind(run_button),
})
    .align_items(nandina::layout::LayoutAlignment::center)
    .gap(10.0f);
```

### 9.3 根组件挂载

```cpp
auto page = column({
    header_bar(),
    stats_section(),
    middle_content_section(),
    footer_section(),
})
    .padding_top(20.0f)
    .gap(20.0f);

window.set_root(page);
```

## 10. V1 非目标

### 10.1 不在 V1 暴露完整 reconciler

V1 只是 authoring API 封装层，不强求虚拟树 diff。

### 10.2 不在 V1 改写全部 widgets 公共接口

V1 应优先覆盖：

- 高层布局原语
- 高频基础组件
- 根组件挂载
- `Ref<T>`

其他控件可以逐步接入。

### 10.3 不在 V1 取消现有低层 API

以下低层能力仍可保留给框架内部或高级用户：

- `std::unique_ptr<NanWidget>`
- `add_child(...)`
- `set_root_component(...)`

但它们不应再作为“推荐 authoring 路径”。

## 11. 建议的下一步实现顺序

1. 先实现 `Node`、`Children`、`Ref<T>` 三个核心类型
2. 先为 `row / column / stack / padding / expanded / sized_box / spacer` 提供 authoring 包装
3. 为 `label / button / card / panel` 提供第一批高频组件工厂
4. 为 `NanAppWindow` 增加 `set_root(Node)`
5. 用 showcase 页面先验证一版真实 authoring 体验

## 12. 相关文档

- [组件 Authoring 与挂载 API 设计](component-authoring-and-mounting.md)
- [架构规划](architecture-plan.md)
- [编码与 API 规范](coding-and-api-conventions.md)
- [Godot 式 Authoring 草案](godot-like-authoring-draft.md)