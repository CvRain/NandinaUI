# Image

> 🟡 使用手册提纲；加载接口和新增 cover 锚点已按源码核对，正文待维护者手写。

## 核对表

| 项目 | 当前接口/规则 |
| --- | --- |
| 来源 | `set_source()` / `source()`；文件路径或 res URI，经注入的资源/纹理服务加载 |
| 缩放 | `set_scale_mode(ImageScale)` / `scale_mode()`；stretch（默认）、contain、cover |
| 作者入口 | `ui.make<Image>(source).image_scale(mode)` |
| contain 对齐 | `set_alignment(ImageAlignment)` / `alignment()`；默认 center，对 cover 无影响 |
| cover 锚点（experimental） | `set_cover_anchor(scene::percent(x), scene::percent(y))` / `cover_anchor()` |
| 默认/范围 | 百分值 50/50，两个轴各在 [0,100]；越界或非有限抛 invalid_argument |
| 锚点含义 | 在 source_rect 的可裁剪余量中定位，不是目标坐标或纹理像素 |
| builder | `.cover_anchor(percent(x), percent(y))`，仅 Image 能力可用 |
| 源区域 | `set_source_rect()` / `clear_source_rect()`，纹理像素坐标 |
| 变更代价 | cover 锚点只标 paint；不改加载选项/纹理缓存键，不重新解码或上传 |
| 自然尺寸 | `natural_size()`；加载前零，不受 cover 锚点改变 |

## 适配与裁剪

<!-- TODO: 手写 0/50/100 百分比、横图/竖图/source_rect 示例，并区分 contain 对齐。 -->

## 资源生命周期

<!-- TODO: 手写同步/异步加载、资源作用域与缓存生命周期的应用用法。 -->

## 验收

<!-- TODO: 维护者补 Image/Surface/Card 组合 Showcase；现有自动与页面回归见 D6 §18。 -->

实验性规则见 [D6 §18](../references/authoring_dsl_and_content_measurement.md#18-p41b-视觉表面与图片裁剪锚点契约2026-11-03实施中)。
