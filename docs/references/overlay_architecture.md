# 浮层架构

本文记录 NandinaUI 浮层组件共享的内部架构。该能力当前属于 **internal**，尚未作为应用层 API 发布；Tooltip、Select 和 Dialog 均已接入。

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

`OverlayOptions::level` 用语义层级表达浮层用途，组件不直接写 CanvasLayer 的 order 数值：

| `OverlayLevel` | 用途 |
| --- | --- |
| `popup` | 提示、下拉与菜单；位于应用内容之上，彼此按加入顺序叠放 |
| `modal` | 模态内容；高于所有 popup，并阻断其下全部输入 |
| `nested_popup` | 模态内容内部再展开的提示与下拉；必须高于模态遮罩，否则会被埋掉且点不到 |

同级之间使用「后加入者位于上方」的场景树规则。**浮层内部再展开的浮层**（对话框里的 Select、菜单里的子菜单）由发起方在 `present()` 时声明 `nested_popup`：它用 `OverlayHost::hosts_node()` 判断自己是否已经在浮层内容之下，是则抬高层级。层级必须显式声明而不是由 host 猜测，因为 `present()` 收到的是已经游离的控件，host 无法从祖先链推断它属于谁。

未来需要真正的嵌套深度、子浮层跟随父浮层关闭时，应扩展这套层级模型，而不是让组件依赖 CanvasLayer 的固定 order。

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
- 已实现 `OverlayOptions::level` 的语义层级（`popup` / `modal`）：模态内容压过提示与下拉，而不依赖 CanvasLayer 的 order 数值；
- 已实现 `DismissLayer` 的遮罩绘制、整体淡入淡出与内容居中：模态面板因此可以与遮罩共用一条动画；
- 已实现 `NanWindow` 默认安装 OverlayHost：它成为场景树根，应用/Router 内容进入其 content layer；页面与组件通过 `BuildContext::overlay_host()`（`PageContext` 同源）取得同一实例。

尚未实现：

- 浮层打开/关闭的过渡与缓动曲线，以及 `motion` token / `reduced_motion` 偏好的接入；
- roving focus 的 RTL 方向键极性（有意缓做，见 [选择与导航的键盘模型](../components/selection_and_navigation.md)）。

## 后续顺序

阶段 1 的两项遗留都已落地：

1. roving focus / typeahead：`widget::RovingFocus` 提供显式移动模型（`focus_and_selection` /
   `focus_only` / `selection_only`）以及 Home / End、PageUp / PageDown 与 typeahead，已被
   `RadioGroup` / `Tabs` / `Select` / `ToggleGroup` / `Pagination` 接入；
2. 嵌套浮层的父子关闭关系：`OverlayOptions::parent` 声明从属关系，父层关闭时
   `close_descendants()` 递归关闭后代并标记 `OverlayCloseReason::parent`，`Select` 与 `Tooltip` 已接入。

阶段 4 的 Popover / DropdownMenu / Combobox / CommandPalette 现在可以直接基于这套设施实现。
新增组件应复用同一份 MenuItem model 与 selection model，而不是各自定义选项结构
（见 [组件开发路线图](component_roadmap.md)）。

OverlayHost 仍不应进入 `<nandina/widget/controls.hpp>`：应用层继续通过组件构造，Getting Started 也不应直接使用它。
