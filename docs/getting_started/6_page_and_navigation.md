# 使用 app::Page 组织应用

根视图工厂很适合快速创建单窗口界面，但随着应用出现多个功能区域、页面参数和导航行为，界面需要一个更明确的结构边界。`app::Page` 是 NandinaUI 面向应用开发者提供的页面抽象：每个页面拥有自己的构建作用域，可以接收强类型参数，并由 `NanRouter` 负责创建、显示、保留和销毁。

在这一章中，我们会把前面完成的界面迁移到一个具体的 `Page` 类型，并认识页面与普通控件之间的职责差异。随后可以增加第二个页面，通过路由完成前进和返回。至此，开发者已经接触了 NandinaUI 应用开发的完整主干：应用入口、窗口、控件、布局、状态、页面与导航。

## 本章目标

- 从 `app::Page<>` 派生一个应用页面。
- 实现 `build(widget::BuildContext&)` 并返回 `widget::View`。
- 使用 `app::run<PageT>()` 将页面作为应用入口。
- 了解 `NoParams` 与强类型页面参数的使用方式。
- 认识 `NanRouter` 的 `push`、`replace`、`pop` 及对应请求接口。

## 建议覆盖的知识点

- `Page`、`NanPageT` 与 `PageContext` 的层次和适用范围。
- 页面级 `ReactiveScope` 以及页面卸载后的自动清理。
- `route_key()`、页面参数与稳定路由身份。
- keep-alive 页面栈及 `on_activate()` / `on_deactivate()` 生命周期。
- 回调中优先使用 `request_push()`、`request_replace()` 和 `request_pop()` 延迟修改路由栈。
- 局部页面状态与应用级 `NanStore` 的职责边界。

> 建议本章以两个页面之间的往返作为结尾；复杂路由、共享 Store、异步任务和页面转场可作为后续进阶文档展开。
