# AspectRatio

> 🟡 实验性（experimental）组件使用手册提纲；框架、符号表与默认值已按代码核对，叙述性正文待维护者手写。

<!-- TODO: 一句话说明 AspectRatio 容器与 NanControl::set_aspect_ratio 的分界：节点级 set_aspect_ratio 仅约束自身 measure_layout，当父容器（Expanded、Stack、Grid、Resizable）在 layout_to 阶段赋予紧约束槽位时仍会被父槽位尺寸覆盖；AspectRatio 容器在 on_layout 内部吸收外层槽位并始终将子控件布局在满足 width / height == ratio() 的最大内接矩形中。 -->

## 何时使用

| 场景 | 建议 |
| --- | --- |
| 在 `Expanded`、`Resizable`、`Grid`、`Stack` 或可变宽列中将媒体预览、卡片缩略图或画布严格保持在 `16:9` / `4:3` / `1:1` 等宽高比 | 本组件（`AspectRatio`） |
| 仅需在普通流式测量时给叶子控件自身设一个宽高比提示 | `NodeBuilder::aspect_ratio(float)` / `NanControl::set_aspect_ratio(float)` |

## 快速开始

```cpp
#include <nandina/widget/controls.hpp>

// 声明式布局简写
auto preview = ui.aspect_ratio(
    16.0F / 9.0F,
    ui.surface(ui.center(ui.make<widget::Label>("16:9 画面")))
);

// ComponentTraits 入口（支持配置内接盒子在父槽位中的对齐方式）
auto aligned = ui.make<widget::AspectRatio>(4.0F / 3.0F)
                   .alignment(widget::LayoutAlignment::start)
                   .child(ui.make<widget::Label>("左上对齐 4:3"));
```

## 测量与布局规则

| 父级约束情形 | `on_measure` 结果 | `on_layout` 子项落位 |
| --- | --- | --- |
| 宽度有限、高度无界（如垂直 `Column` / `ScrollView`） | `w = max_width`，`h = w / ratio()`（按 `min_height` 夹紧） | 在实际分配的 `size()` 内求最大内接 `ratio()` 矩形并按 `alignment()` 定位 |
| 高度有限、宽度无界（如水平 `Row`） | `h = max_height`，`w = h * ratio()`（按 `min_width` 夹紧） | 同上 |
| 两轴均有限（loose / bounded） | `max_width × max_height` 内的最大内接 `ratio()` 矩形 | 同上 |
| 两轴均有限且父级给紧槽位（如 `Expanded`、`Stack`、`Resizable`） | 容器接受父级槽位尺寸；比例由 `on_layout()` 下沉到 `content_rect()` | 即使外层 `size()` 比例不等于 `ratio()`，子控件仍以紧约束布局于内接 `content_rect()` |
| 两轴均无界（`loose()`） | 先测量子控件自然尺寸 `(cw, ch)`，取包络子控件的最小 `ratio()` 矩形 | 同上 |

## 公开 API

| 接口 | 说明 |
| --- | --- |
| `AspectRatio::create(ratio, alignment)` | 命令式创建（默认 `ratio = 1.0F`，`alignment = LayoutAlignment::center`） |
| `ui.aspect_ratio(ratio, child)` / `ui.aspect_ratio(child)` | `BuildContext` 与 `widget::authoring` 构造简写 |
| `ui.make<AspectRatio>(ratio, child)` | `ComponentTraits<AspectRatio>` 入口 |
| `set_ratio(float)` / `ratio()` | 设置或查询宽高比（`width / height`；非有限或 `<= 0` 抛 `std::invalid_argument`） |
| `set_alignment(LayoutAlignment)` / `alignment()` | 当外层槽位比例与 `ratio()` 不一致时，内接矩形在槽位内的对齐位置（`start` 左上、`center` 居中、`end` 右下） |
| `set_child(control)` / `clear_child()` / `child()` | 设置、清除或只读查询唯一子控件（拒绝空指针、已挂载节点或祖先环） |
| `content_rect()` | 最近一次布局计算得到的内接子控件局部矩形 |
