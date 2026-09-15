# Tooltip 迁移契约

Tooltip 是统一浮层设施迁移的第一批组件，用于验证非模态、悬停触发和锚点定位场景。迁移目标是替换内部绘制与定位实现，不改变现有应用层构造方式。

## 公共 API

以下接口保持兼容：

- `Tooltip::create(text, trigger, theme)`；
- `set_text()`、`set_trigger()`、`set_placement()` 和 `set_delay()`；
- `show()`、`hide()` 与 `visible()`；
- 主题覆盖、文本管线和语义接口。

Tooltip 仍然是包裹触发控件的组件。页面作者不需要直接创建 `OverlayHost`，组件应从所属窗口的 `BuildContext` 获取窗口级浮层服务。

## 内部职责

触发器继续负责悬停状态和计时；气泡本身作为独立的 portal 内容由 `OverlayHost` 托管：

```text
Tooltip
└── trigger

OverlayHost / overlay layer
└── tooltip bubble
```

气泡的位置由 `AnchoredPositioner` 根据触发器的全局边界计算。Tooltip 是非模态浮层，不阻断 content layer，不使用 `DismissLayer`，离开触发器或隐藏 Tooltip 时应销毁对应的 `OverlayHandle`。

## 验收条件

- 触发器位于 `ScrollView`、`Card` 等裁剪容器内时，气泡仍可完整绘制；
- `top` / `bottom` 放置和窗口边缘 shift 行为与定位器一致；
- 悬停延迟、离开隐藏和显式 `show()` / `hide()` 行为保持现有测试语义；
- 气泡不抢占触发器点击，也不阻断页面其他输入；
- Tooltip 卸载或触发器替换时，`OverlayHandle` 不泄漏；
- 注入的窗口 OverlayHost 先于 detached Tooltip 销毁时，弱服务引用安全失效；
- 无窗口服务的 detached 单元测试仍可测试文本、主题和计时逻辑。

## 实现状态

迁移已完成：

- `ComponentTraits<Tooltip>` 在构建时注入 `BuildContext::overlay_host()`；`Tooltip::create()` 没有构建上下文时回退到最近的祖先 `OverlayHost`（窗口把它安装为场景树根）；
- 气泡是纯展示控件（`contains_point` 恒为 false），既不参与命中也不阻断 content layer 输入；
- 气泡由 `AnchoredPositioner` 定位，视口取 `OverlayHost::viewport_size()`；触发器在滚动或布局中移动时每帧重定位，视口尚未布局或触发器尺寸无效时跳过本次定位；
- 文本管线从 Tooltip 的 `primitives::Text` 复制到气泡，字体上下文随气泡入树自动解析；
- `hide()`、`set_trigger()`、Tooltip 离开场景树与析构都会释放 `OverlayHandle`；未挂载到任何 `OverlayHost` 时保留原有 detached 绘制。

迁移完成后，再将 Select 的 popup 和 Dialog 的模态内容接入同一套托管、定位、关闭与焦点设施。
