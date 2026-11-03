# Surface

> 🟡 布局/绘制原语的使用手册提纲；符号和边界已核对，正文待维护者手写。
> experimental；契约见 [D6 §18](../references/authoring_dsl_and_content_measurement.md#18-p41b-视觉表面与图片裁剪锚点契约2026-11-03实施中)。

## 入口与核对表

| 项目 | 当前接口/规则 |
| --- | --- |
| 类型 | `widget::Surface`，`<nandina/widget/surface.hpp>`，继承 Padding |
| 作者工厂 | `ui.surface()` / `authoring::surface()`；不提供 ComponentTraits/组件目录项 |
| 内容 | 继承的 `set_child()`；builder `.child()`，拒绝 null，替换原单子项 |
| 内容边距 | `set_padding()` / `padding()`、builder `.padding()`；默认零，有限非负 |
| 盒子 | `set_box_style(BoxStyle)` / `box_style()` / `resolved_style()`、builder `.box_style()` |
| 默认样式 | surface 填充、border 描边 token，描边宽/圆角均为 0 |
| 实例/动画 | 既有 `visual::container.{fill,border_color,border_width,radius}` |
| 度量 | 只由内容和 padding 决定；圆角和描边不占布局空间 |
| 主题 | token 随当前主题重解析，实例 presentation 优先 |
| 绘制次序 | 背景/描边 → 子项；内容可盖住描边，需要留白时显式 padding |
| 裁剪/命中 | 默认不裁剪；显式 ControlOverflow::clip 仍是矩形；没有圆角子内容 mask |
| 语义 | 容器本身不新增角色、动作或焦点；子项语义保持 |

## 布局与样式

<!-- TODO: 手写零边距/带边距内容、语义 token/显式 presentation、动态主题示例。 -->

## 圆角边界

<!-- TODO: 手写圆角背景与矩形子树裁剪的区别；需要图片圆角 mask 时当前没有该能力。 -->

## 验收

<!-- TODO: 维护者新增 Showcase 示例与视觉验收；自动证据见 D6 §18。 -->
