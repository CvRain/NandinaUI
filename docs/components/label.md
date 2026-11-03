# Label

> 🟡 使用手册提纲；符号与默认值已按源码核对，叙述正文按维护者分工留待手写。
> 新增盒内对齐 API 属于 experimental，正式契约见
> [D6 P4.1a](../references/authoring_dsl_and_content_measurement.md#16-p41a-文本盒内对齐契约2026-11-03首个实施子批)。

## 入口

| 项目 | 符号 |
| --- | --- |
| 组件 | `widget::Label`，`<nandina/widget/label.hpp>` |
| 创建 | `ui.make<widget::Label>(text)`、`Label::create(graph, text, theme)` |
| 语义 | `semantics::Role::static_text`，label 为完整原文 |
| 共享文本实现 | `primitives::Text`；直接 API 可经 Label 使用 |

## 能力表

| 能力 | 当前入口 | 行为 |
| --- | --- | --- |
| 文本 | `set_text()` / `text()`、`bind_text(source)` | 文本变化更新测量、绘制与语义；source 的生命周期必须覆盖绑定 |
| 字体 | `set_font()`、`set_font_family/weight/slant()` | 使用 framework FontRequest；改变字体重新解析、测量 |
| 字号 | `set_font_size()`、builder `.font_size()` | 逻辑字号；必须有限且为正 |
| 颜色 | `set_color_token()`、builder `.color_token()`；继承的 `set_color()` | 颜色只影响绘制；token 跟随主题，显式颜色遵守实例覆盖 |
| 水平对齐 | `set_align()` / `align()`、builder `.text_align()` | start / center / end，默认 start；每行在指派宽度内摆放 |
| 垂直对齐 | `set_vertical_align()` / `vertical_align()`、builder `.text_vertical_align()` | top / center / bottom，默认 top；整个文本块在指派高度内摆放 |
| 换行/溢出 | `set_overflow(text::TextOverflow)`、`set_max_lines()` | 当前仍共用 clip / ellipsis / wrap / scale 枚举；最大行数至少 1 |
| 测量输出 | `measured_text_width/height()`、`layout_result()` | 当前布局的逻辑内容尺寸、行列表与 overflowed；不是指派盒子的尺寸 |

## 盒内对齐

<!-- TODO: 手写顶部/居中/底部与单行/多行使用说明；富余空间与短盒 fallback；
     区分控件布局对齐、Label 对齐与 Button.label_align 配方。 -->

| 项目 | 已核对规则 |
| --- | --- |
| 默认 | 水平 start、垂直 top |
| 垂直中心/底部 | 整个已布局文本块按自然高度摆放；余量不足时偏移 0 |
| 失效 | 两轴 setter 等值不标脏；修改只标 paint，不 reshape/measure/semantics |
| 完整 style | 颜色/两轴对齐变化不重排；color/font_size/font 仍变成显式覆盖 |
| 绘制坐标 | `draw_in` 是轴对齐的 screen-space 盒子；`draw_at` 不施加对齐；DPI 不参与偏移；节点局部旋转/非均匀缩放不在本批 |

## 测量与绑定

<!-- TODO: 手写有限宽度自然长高、Column/Card/ScrollView 与窄宽度示例；
     当前 wrap/overflow 共用枚举，绑定直接写入不解除绑定，退出树清理；
     不将 D6 后续 reset/绑定替代提案当作可用 API。 -->

## 验证与边界

<!-- TODO: 补维护者的真实窗口使用体验与后续完整 P4.1 验收记录。 -->

- 自动回归：两轴偏移、clip、viewport/DPI、缓存、有限宽度自然行高与真实 Label 页/ComponentInfoTable。
- P4.1a 的实现与验证记录见 D6 §16；绑定/覆盖与完整能力表仍由 P4.1 后续批次定形。
