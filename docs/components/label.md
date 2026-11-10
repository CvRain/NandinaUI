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
| 文本 | `set_text()` / `text()`、`bind_text(source)`、builder `.text()` / `.bind_text()` | 直接写入解除旧绑定（同值也解除）；重新绑定替代旧源；source 活到解绑/作用域清理 |
| 字体 | `set_font()`、`set_font_family/weight/slant()`；builder `.font/font_family/font_weight/font_slant` | 使用 framework FontRequest；改变字体重新解析、测量。缺对应 face 时 **P4.7 合成回退**生效：500/600/700+ 分级合成加粗、italic/oblique 约 12° 倾斜，advance 同步；真实多 face 优先。见 [合成字重/斜体回退](../references/font_synthesis.md) |
| 字号 | `set_font_size()`、builder `.font_size()` | 逻辑字号；必须有限且为正 |
| 排版尺度 | `set_text_size(theme::TextSize)`、`set_leading(theme::Leading)`、`set_line_height(float)`；builder `.text_size()` / `.leading()` / `.line_height()` | Tailwind 兼容 13 档，经主题 `NanTypeScale`（root base × ratio）解析；显式字号 > 继承 style context > 尺度 token > role 字号 > 组件默认（18）；行高数值覆盖优先于 leading token，`0` 恢复 token/role/默认。见 [排版尺度与单位](../references/typography_scale.md) |
| 语义 role | `set_typography_role(theme::TypographyRole)`；builder `.typography_role()` | display/heading/title/subtitle/body/label/caption 提供字号+行高+字重/斜体默认值；颜色仍由颜色 token 决定。显式尺寸/行高/字重覆盖对应 role 字段；role 表暂为固定 canonical 表（Step 2b 主题化）。见 [排版尺度与单位](../references/typography_scale.md) |
| 颜色 | `set_color_token()`、builder `.color_token()`；继承的 `set_color()` | 颜色只影响绘制；token 跟随主题，显式颜色遵守实例覆盖 |
| 水平对齐 | `set_align()` / `align()`、builder `.text_align()` | start / center / end，默认 start；每行在指派宽度内摆放 |
| 垂直对齐 | `set_vertical_align()` / `vertical_align()`、builder `.text_vertical_align()` | top / center / bottom，默认 top；整个文本块在指派高度内摆放 |
| 换行/溢出 | `set_overflow(text::TextOverflow)`、`set_max_lines()`；builder `.text_overflow()` / `.max_lines()` | 当前仍共用 clip / ellipsis / wrap / scale 枚举；最大行数至少 1 |
| 测量输出 | `measured_text_width/height()`、`layout_result()`、`line_count()` / `overflowed()` | 当前布局的逻辑内容尺寸、行列表与 overflowed；不是指派盒子的尺寸 |

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
     当前 wrap/overflow 共用枚举；文本绑定替代规则见下表；
     不将 D6 后续样式 reset/独立 wrap 提案当作可用 API。 -->

| 项目 | 已核对规则（P4.1 第二子批） |
| --- | --- |
| 绑定 | `bind_text(source)` 立即更新；新源替代旧源 |
| 页面绑定 | `bind_text(scope, source)`、`ui.make<Label>(source)`、`ui.bind(label, &Label::set_text, source)`；页面所有源优先用这些入口，直接 `bind_text(source)` 使用控件作用域 |
| 解绑 | `unbind_text()`、builder `.unbind_text()` 保留最后值；`text_is_bound()` 查询 |
| 直接写入 | `set_text()`、Property.set、builder `.text()` 都先解绑；同值不额外 changed |
| 生命周期 | 退出树或页面 scope 清理后无旧写入；重挂载不自动重新 bind；直接 `bind_text(source)` 的 source 必须活到解绑/控件销毁 |
| 输出 | `line_count()` / `overflowed()` 来自当前布局结果，不另外存状态 |

## 验证与边界

<!-- TODO: 补维护者的真实窗口使用体验与后续完整 P4.1 验收记录。 -->

- 自动回归：两轴偏移、clip、viewport/DPI、缓存、有限宽度自然行高与真实 Label 页/ComponentInfoTable。
- P4.1a 的实现与验证记录见 D6 §16；文本绑定契约见 D6 §17；逐字段样式 reset、独立 wrap 与完整作者体验验收仍由 P4.1 后续批次定形。
