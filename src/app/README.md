# app

应用开发层（authoring layer）。**依赖 widgets** 及其下层，不反向侵入 runtime 内核。
authoring 体验吸收 Flutter 式组合 + Angular 式 page / router。

## 计划 API

- `App`：应用入口与窗口编排，统一根挂载入口（`mount` / `setRoot`）。
- `Page`：页面描述对象（`routeKey()` / `title()` / `build()`）。
- `Router`：页面注册、当前路由、`navigateTo(key)`。
- `PageHost`：承载当前页面内容区，导航时重建页面。
- `Ref` / `Handle` / `Key`：挂载后的组件访问机制。

## 计划用法

```zig
window.setRoot(column(.{
    label("Dashboard"),
    button("Run"),
}));
```

## 设计要点

- 使用者写 authoring 描述树，不手写坐标、不手工管所有权。
- Page 是导航层契约；Node / Ref / mount 是页面内部 authoring 契约，两者分层演进。
- 默认 `rebuild-on-enter`，不隐式缓存页面状态。

## 状态

🚧 骨架。见 [Page / Router 合约](../../docs/development/page-and-router.md) 与 [Authoring 与挂载](../../docs/development/authoring-and-mounting.md)。
