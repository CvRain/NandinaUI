# 浮层架构

本文记录 NandinaUI 浮层组件共享的内部架构。该能力当前属于 **internal**，尚未作为应用层 API 发布；Tooltip 和 Select 已接入，Dialog 仍在迁移中。

## 目标

浮层内容需要脱离触发控件原本的父级绘制：它不能被 ScrollView、Card 或普通布局容器裁剪，同时必须位于应用内容上方参与绘制、命中测试和焦点管理。

不同组件仍保留自己的语义：Tooltip 是提示，Select 是单选输入，Dialog 是模态内容。浮层基础设施只解决它们共同面对的托管、定位、关闭和焦点问题。

## OverlayHost

`scene::OverlayHost` 建立在现有 `scene::LayerStack` 和 `scene::CanvasLayer` 上，拥有两个 screen-space layer：

```text
OverlayHost
├── content layer (order 0)
│   └── application content
└── overlay layer (order 1000)
    └── transparent overlay surface
        ├── tooltip / popup / menu
        └── dialog dismiss layer
```

两个 layer 都作为 viewport layout root 在同一轮布局中获得窗口逻辑尺寸。overlay surface 自身不参与命中，因此没有浮层覆盖某个位置时，输入会继续到达 content layer；真正需要模态阻断时，应由后续 DismissLayer 提供全屏命中区域。

`LayerStack` 的逐层布局与逐层命中不要求它位于场景树根部。当它嵌在普通控件之下时（例如通过页面根返回、由 Router 的 `PageHost` 持有），scene 仍会把每个 screen-space layer 的 layout root 按视口尺寸布局，并在命中测试中遵循 layer 顺序与 `block_below`；`block_below` 吞掉的命中会向上传播，父容器不会把自己报告为命中目标。

`NanWindow` 在构造时创建一个 OverlayHost 并把它作为场景树根：`set_content()` 挂载的应用 / Router 内容进入 content layer，`NanWindow::overlay_host()` 暴露该实例；页面通过 `PageContext::ui().overlay_host()` 取得同一实例并调用 `present()`。页面根因此不再需要自行创建 OverlayHost。

OverlayHost 归属 `scene` 而非 `widget::internal`：它只使用 `LayerStack` / `CanvasLayer` / `NanControl`，没有 widget 依赖；若留在上层的 widget 命名空间，底层的 `LayerStack` 就必须向上引用 widget 类型，违反「禁止向上依赖」的模块规则。`LayerStack::as_overlay_host()` 因此返回同层类型，`widget`/`app` 侧按正确方向（向下）使用它。

## Portal 生命周期

`OverlayHost::present()` 接受一个尚未挂载的 `NanControl`，将它托管到 overlay surface，并返回 move-only `OverlayHandle`。

- Handle 存活表示调用方仍拥有本次 presentation；
- 调用 `close()` 或销毁 Handle 会移除对应浮层；
- 移动 Handle 会转移关闭责任；
- Host 先销毁时，Handle 安全失效；
- 场景树遍历期间关闭会复用既有 deferred mutation 机制。

浮层组件应把 Handle 作为自身打开状态的一部分，不应在外部保存 overlay surface 指针或直接修改 layer 子节点。

## 叠放规则

`OverlayOptions::order` 决定不同 presentation 的显式层级；相同 order 使用后加入者位于上方的场景树规则。未来嵌套菜单或模态窗口需要新的层级策略时，应扩展 options 或引入分组，不应让组件直接依赖 CanvasLayer 的固定 order 数值。

## 当前边界

第一阶段已经提供 portal 托管、生命周期与锚点定位：

- 已实现 content/overlay layer 分离；
- 已实现透明 surface、viewport 布局与顶层命中；
- 已实现 RAII Handle 和确定性关闭；
- 已测试父级裁剪之外的浮层命中和多浮层顺序。
- 已实现四向 placement、三种 alignment、gap、offset、flip 与 viewport shift；
- 已通过独立几何测试覆盖边缘翻转、视口内收和显式溢出。
- 已实现 `DismissLayer` 的左键外部点击与 Escape 关闭原因分发。
- 已实现 `FocusScope` 的初始焦点、Tab 循环与卸载焦点恢复。
- 已实现 `OverlayOptions::block_below` 的模态命中阻断：阻断层之上命不中时吞掉该点，且该结果会向上传播。
- 已支持 `OverlayHost` 嵌在普通控件之下（含 Router 页面根）：层布局、层命中与阻断不再要求它位于场景树根部。
- 已实现 `NanWindow` 默认安装 OverlayHost：它成为场景树根，应用/Router 内容进入其 content layer；页面与组件通过 `BuildContext::overlay_host()`（`PageContext` 同源）取得同一实例。

尚未实现：

- 嵌套浮层的父子关闭关系。

## 后续顺序

1. 迁移 Dialog 的模态内容，保持现有公开构建方式兼容。

在上述迁移完成前，OverlayHost 不应进入 `<nandina/widget/controls.hpp>`，Getting Started 也不应直接使用它。
