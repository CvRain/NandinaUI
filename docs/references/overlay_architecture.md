# 浮层架构

本文记录 NandinaUI 浮层组件共享的内部架构。该能力当前属于 **internal**，尚未作为应用层 API 发布，现有 Select、Tooltip 和 Dialog 也尚未迁移。

## 目标

浮层内容需要脱离触发控件原本的父级绘制：它不能被 ScrollView、Card 或普通布局容器裁剪，同时必须位于应用内容上方参与绘制、命中测试和焦点管理。

不同组件仍保留自己的语义：Tooltip 是提示，Select 是单选输入，Dialog 是模态内容。浮层基础设施只解决它们共同面对的托管、定位、关闭和焦点问题。

## OverlayHost

`widget::internal::OverlayHost` 建立在现有 `scene::LayerStack` 和 `scene::CanvasLayer` 上，拥有两个 screen-space layer：

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

尚未实现：

- NanWindow 对 OverlayHost 的默认安装和 BuildContext 服务注入；
- 点击外部和 Escape 的统一关闭原因；
- 模态输入阻断与焦点限制；
- 嵌套浮层的父子关闭关系。

## 后续顺序

1. 让 NanWindow 建立默认 OverlayHost，并通过构建上下文提供服务；
2. 实现 DismissLayer 与 FocusScope；
3. 先迁移 Tooltip 验证非模态定位；
4. 再迁移 Select 和 Dialog，保持它们现有公开构建方式兼容。

在上述迁移完成前，OverlayHost 不应进入 `<nandina/widget/controls.hpp>`，Getting Started 也不应直接使用它。
