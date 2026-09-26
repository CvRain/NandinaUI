# 使用 app::Page 组织应用

> 本章正在随 Page / Router 重构迁移。当前源码仍提供旧的 keep-alive `push/pop` API；新的应用代码请以[Page / Router 目标合约](../references/page_and_router.md)为准。教程正文将在实现迁移完成后补齐。

根视图工厂很适合快速创建单窗口界面，但随着应用出现多个功能区域、页面参数和导航行为，界面需要一个更明确的结构边界。`app::Page` 是 NandinaUI 面向应用开发者提供的页面抽象：每个页面拥有自己的构建作用域，可以接收强类型参数，并由 Router 在 Outlet 中创建和销毁。

在这一章中，我们会把前面完成的界面迁移到一个具体的 `Page` 类型，并认识页面与普通控件之间的职责差异。随后可以增加第二个页面，通过路由完成页面之间的跳转。至此，开发者已经接触了 NandinaUI 应用开发的完整主干：应用入口、窗口、控件、布局、状态、页面与导航。

## 本章目标

- 从 `app::Page<>` 派生一个应用页面。
- 在路由表中注册页面类型和展示元数据。
- 实现 `build(app::PageContext&)` 并返回 `widget::View`。
- 使用 `app::run<PageT>()` 将单页应用作为入口。
- 了解 `NoParams` 与强类型页面参数的使用方式。
- 在 `NanWindow` 的 Shell 与 RouterOutlet 中使用 `navigate<PageT>()`。

## 建议覆盖的知识点

- `Page`、`NanPageT` 与 `PageContext` 的层次和适用范围。
- 页面级 `ReactiveScope` 以及页面离开后的自动清理。
- 路由表、页面参数与类型路由身份；框架不会通过 RTTI 或路径解析查找页面。
- Shell、RouterOutlet 与常驻导航 UI 的所有权关系。
- 回调中按值捕获 Navigation，并在 UI 任务阶段提交导航。
- 局部页面状态与应用级 `NanStore` 的职责边界。

> 建议本章以两个页面之间的往返作为结尾；复杂路由、共享 Store、异步任务和页面转场可作为后续进阶文档展开。
