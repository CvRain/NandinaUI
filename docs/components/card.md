# Card

> 🟡 使用手册提纲；接口、配方默认及四侧内容边距已按代码核对，正文待维护者手写。

## 核对表

| 项目 | 当前接口/行为 |
| --- | --- |
| 创建 | `ui.make<widget::Card>()` / `Card::create(theme)` |
| 内容 | `set_child()`，builder `.child()`；单子项，null 被拒绝 |
| 样式 | `set_override(CardRecipeRule)` / `resolved_style()`，默认来自语义配方 |
| 四侧边距（experimental） | `set_content_insets(NanInsets)` / `content_insets()`；builder `.content_insets()` |
| 默认边距 | 未显式覆盖时使用当前配方 metrics.padding_x/y |
| 重置边距 | `clear_content_insets()` 恢复最新主题/实例配方值 |
| 零边距 | 内容占用 Card 矩形，背景/阴影/min_height 仍来自配方 |
| 参数 | 有限非负，四侧总和须有限；过大时内容尺寸归零 |
| 裁剪 | 圆角不等于圆角图片 mask；仅显式矩形 ControlOverflow::clip 可裁子树 |

## 内容布局

<!-- TODO: 手写配方默认与显式四侧覆盖、零内边距图片组合、reset 后跟随主题的用法。 -->

## 配方与视觉

<!-- TODO: 手写外观与阴影/边框/内容之间的关系；新 Showcase 组合留待人工验收。 -->

实验性边界见 [D6 §18](../references/authoring_dsl_and_content_measurement.md#18-p41b-视觉表面与图片裁剪锚点契约2026-11-03实施中)。
