# NandinaUI Godot-like 开发范式（历史参考）

> 状态：历史参考（2026-05）  
> 当前判断：本文件记录项目早期受 Godot 影响的应用/窗口继承式设计思路。它仍可作为理解 `NanWindow` / `NanApplication` 生命周期边界的背景材料，但已经不是主线推荐的 authoring 路径。

## 1. 文档定位

这份文档当前主要回答两件事：

- 为什么 runtime / app 层会保留一组类似引擎生命周期的钩子
- 为什么项目早期会强调“继承基类 + 重写回调”的开发体验

它**不再**用于指导当前页面、组件、showcase 的主线写法。

当前主线推荐路径已经转向：

- `NanPage::build() -> mount(Node)`
- `Node` / `Ref<T>` / `children(...)`
- `row / column / stack / label / button` 等 authoring DSL

详见 [组件 Authoring 与挂载 API 设计](component-authoring-and-mounting.md) 与 [无显式 move 的组件组合 API（V1）](component-composition-api-v1.md)。

## 2. 仍然保留的 Godot 式影响

虽然主线 authoring 已经转向组合式 DSL，但下面这些设计影响仍然存在：

- `nandina::runtime::NanWindow` 仍承担窗口、事件轮询、帧循环与绘制闭环
- `nandina::app::NanApplication` / `NanAppWindow` 仍保留应用级生命周期组织能力
- 平台层细节（SDL）仍被隐藏在 runtime / app 实现后面
- `on_ready` / `on_update` / `on_draw` / `on_resize` / `on_close_requested` 这类钩子仍是底层主循环的组织方式

换句话说，Godot 的影响更多保留在“应用/窗口生命周期建模”上，而不是保留在“业务层如何写组件”上。

## 3. 不再作为主线推荐的部分

以下做法当前不应再被视为推荐开发路径：

- 主要通过继承 `NanApplication` / `NanWindow` 来组织页面 UI
- 在 `on_draw()` 中直接手工堆 ThorVG 图元作为常规页面 authoring 方式
- 把窗口生命周期钩子当成组件组合的主要入口
- 把“继承式应用壳”视为替代 `mount(Node)` / `set_root(Node)` 的常规方案

这些模式今天更适合：

- 底层 runtime 验证
- 很薄的应用壳或平台桥接层
- 少量不适合直接走 widget tree 的特殊宿主场景

## 4. 当前推荐替代路径

当前页面和组件 authoring 更推荐：

```cpp
auto page = column(children(
	label("Overview"),
	row(children(
		button("Run"),
		spacer(),
		label("Ready")
	)).gap(12)
));

return mount(std::move(page));
```

应用壳层则更推荐：

```cpp
window.set_root(create_shell(std::move(router), {.header_title = "My App"}));
```

这条路线的重点是：

- 业务层优先描述结构，而不是重写窗口生命周期来直接绘制
- UI 组合优先通过 widget tree / authoring tree 完成
- 挂载后访问子组件优先通过 `Ref<T>`，而不是把局部变量当成所有权容器

## 5. 这份文档仍然适合用于什么场景

当前仍然可以参考本文件的场景包括：

- 理解 `NanWindow` / `NanApplication` 为什么存在这些生命周期钩子
- 讨论未来 bindings / script host 时，应用层宿主应保留哪些引擎式边界
- 分析 runtime / app 与上层 authoring DSL 的职责分层

## 6. 相关文档

- [组件 Authoring 与挂载 API 设计](component-authoring-and-mounting.md)
- [无显式 move 的组件组合 API（V1）](component-composition-api-v1.md)
- [Page / Router 合约（MVP）](page-contract.md)
