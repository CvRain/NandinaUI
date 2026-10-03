# 节点表现层

> 本文是**设计**，不是现状。它规定"节点怎么在父容器里定位、怎么被画出来、哪些属性可以动画"
> 这三件事的统一形状。要改这里的 API 之前先改本文。
>
> 起因是三条使用抱怨：悬浮动画卡顿、`padding` 没有 margin、想在 Label 底下画一条下划线却
> 很难实现且会打破现约束。追下去发现它们**共用同一个前置**：节点的可定位、可动画表面太窄。
> 所以本文把它们当一件事设计，而不是三个补丁。

## 1. 先分级：哪些属性可以动画

卡顿的根因不是帧率，而是**写一个属性时到底触发了多少工作**。这是整篇的骨架：

| 级别 | 属性 | 触发的工作 | 连续动画 |
| --- | --- | --- | --- |
| **L1 绘制** | `color`、`border_color`、`border_width`、`radius` | 重绘该节点 | ✅ 便宜 |
| **L2 变换** | `opacity`、`translate`、`scale` | 重绘该节点（合成变换） | ✅ 便宜 |
| **L3 布局** | `width`、`height`、`font_size`、`padding`、`gap` | **重排子树** | ⚠️ 昂贵 |
| **L4 整形** | `font_size`（尤其） | 重排 **+ 按字号重新栅格化字形** | ❌ 病态 |

`font_size` 同时落在 L3 和 L4，所以它是这套体系里最贵的属性：`Text::set_font_size` 会当场
`update_metrics()` 重新整形文本，而字形图集按 `(字形, 字号)` 缓存 —— 一个 14→18px 的补间
意味着几百个互不相同的字号，**每个都要重新栅格化一遍**。

> **规则**：想表达"变大了 / 更醒目了"，用 L1/L2；`font_size` 只在对排版有真实意图时才改。

## 2. 定位：两套系统，不是一个

QML 把定位分成两件事，这个区分是关键：

| | QML | 本项目 |
| --- | --- | --- |
| **排列**（父容器自动摆放子项） | `Row` / `Column` / `Grid` positioner、`*Layout` | ✅ 已有：`Row` / `Column` / `Flex` / `Grid` / `Wrap` |
| **锚定**（把某一项钉在父容器或兄弟的边上） | `Item.anchors` | ❌ **没有** |

缺的是第二套。下划线之所以难，正是因为它要"钉在父容器底部、左右拉伸"，而这是排列系统
表达不了的。

### 2.1 规则：锚定的子项**脱离排列流**

这是两套系统能共存的前提，也是唯一的硬规则：

- 子项**设置过锚点** → 从父容器的排列流里排除，位置完全由锚点决定（等价 CSS `position: absolute`）；
- 子项**没有锚点** → 照旧由父容器的排列流摆放。

父容器因此不需要知道锚定的子项；锚定也不改变其它兄弟的布局。这条规则让"形状"可以和
正常内容混在一个容器里而不互相干扰。

### 2.2 API 形状（v1 只锚父容器）

```cpp
struct AnchorSpec {
    // 水平：三选一 —— 或 left+right 拉伸
    std::optional<float> left;               // 距父容器左边缘
    std::optional<float> right;
    std::optional<float> horizontal_center;  // 相对父容器水平中心的偏移
    std::optional<float> width;              // 与前两者互斥
    // 垂直：同理
    std::optional<float> top;
    std::optional<float> bottom;
    std::optional<float> vertical_center;
    std::optional<float> height;
};
```

用法——**这就是下划线**：

```cpp
ui.make<widget::Rectangle>()
    .anchors(widget::AnchorSpec{.left = 0.0F, .right = 0.0F, .bottom = 0.0F})
    .height(2.0F)
    .fill_token(theme::ColorToken::primary)
    .opacity(0.0F)                       // L2，便宜
    .behavior(widget::visual::opacity, animation::motion::tween(duration));
```

`left + right` 同时给 = 横向拉伸到两边，**不需要动画宽度**（那是 L3，会重排）。于是"下划线
淡入"变成一个纯 L2 动画。

### 2.3 冲突与非法组合：当场拒绝

过度约束必须**确定性失败**，不能像 QML 那样只是警告后行为未定义：

| 组合 | 处理 |
| --- | --- |
| `left` + `right` + `width` | 抛 `std::invalid_argument`（三者只能取二） |
| `left` + `horizontal_center` | 抛（都是水平定位来源） |
| 只给 `width` 不给任何水平锚点 | 合法：水平居中？**不合法** —— 水平方向无锚点即"不参与锚定" |
| `fill` 语义 | 用 `left+right+top+bottom` 表达，不引入第二个概念 |

### 2.4 margin

锚点的 `left/right/top/bottom` 就是 CSS 意义上的 margin（子项距父容器边的距离）。
排列流里的子项要外间距时用父容器的 `gap`（已有）或子项自身的 margin —— 后者与本文
第 4 节的盒子模型一起做。

**刻意不做（v1）**：锚定到**兄弟节点**（QML 允许 `anchors.left: other.right`）。它需要
一套"命名引用 + 依赖顺序"的机制，而下划线、装饰线、角标这类需求锚父容器就够。
按"用到再列"的原则留到有真实需求时再说。

## 3. 形状家族（v1 三种）

设备层已经够用（`draw_rect` / `draw_rounded_rect(+outline)` / `draw_circle` / `draw_arc` /
`draw_line` / 阴影），中间也已经有一个 painter 家族，**缺的是顶层出口**。

v1 收口三种，都有明确的设备对应：

| 节点 | 字段 | 设备原语 |
| --- | --- | --- |
| `Rectangle` | `BoxStyle`（fill / border / border_width / radius） | `draw_rounded_rect(+outline)` |
| `Circle` | `fill` / `border` / `border_width` / `radius` | `draw_circle` / `draw_arc` |
| `Line` | `color` / `thickness` / 两端点（由锚点或尺寸决定） | `draw_line` |

**统一风格**：一律复用 `BoxStyle`，**不发明 `ShapeStyle`**。形状特有的量（圆的半径、线的
粗细）才新增字段。这样主题作者学一套就够，`set_override` 的形状也一样。

**不做（按你的判断）**：多边形 / 任意路径。它要扩设备接口，而且没有真实需求支撑 ——
用到再列。

### 3.1 公开的 painter 入口

自定义 `on_draw` 里画东西**不应该**需要新建一个节点。painter 家族已经写好了
（`BoxPainter` / `ArcPainter` / `ShadowPainter` …），问题只是它们在 `primitives/` 内部，
而契约规定 primitive 不该成为应用依赖。

所以形状这一节实际交付两件东西：

1. **公开的绘制入口**（给自定义 `on_draw` 用）—— 下划线走这条就够；
2. **形状节点**（参与锚定/布局、有语义、可主题、可动画）—— 需要复用现成控件时用。

两者共用同一份风格结构，所以"自己画的"和"用节点的"看起来是同一个东西。

## 4. 盒子模型

现状：`Padding`（容器内侧）+ `gap`（排列流子项之间）。缺的是：

- **子项的 margin**（排列流里的外间距）；
- **四方向不同的 padding**（`NanInsets` 本身已是四分量，需要确认只是 DSL 没暴露）。

按 CSS 语义对齐，并明确三者的层级关系：

```text
父容器 padding（容器内边距）
  └── 子项 margin（子项外边距）+ 子项 padding（子项内边距）
        └── 子项内容
排列流子项之间另有 gap（与 margin 不同：gap 不作用于首尾）
```

`gap` 与 `margin` 同时存在时的叠加规则要写死：**两者相加**（不做 collapse —— CSS 的
margin collapsing 是历史包袱，不该学）。

## 5. `visual::Path` 补全

现状只有 6 条（`label.color/font_size` + `container.fill/border_color/border_width/radius`）。
按第 1 节的分级补齐 **L2**：

| 新路径 | 值类型 | 说明 |
| --- | --- | --- |
| `visual::opacity` | `float` | 任何节点。底层 `NanNode2D::set_local_opacity` 已存在，只是不可动画 |
| `visual::translate` | `NanPoint` | 相对布局位置的偏移，不改布局 |
| `visual::scale` | `NanPoint` | 分轴缩放 + `transform_origin`（九宫格枚举，同 QML `transformOrigin`） |

`scale` 与 `translate` 不触发重排，但**会改变 `global_bounds()`** —— 命中测试要跟着走
（这是期望行为，不是副作用）。`font_size` 留在 L3/L4，不推荐用于动效。

补齐 L2 之后，005 里那些"造好了没接出去"的能力开始有用武之地：`Group` 的
`stagger`（错峰入场）、`Keyframes`（脉冲/呼吸）都需要能廉价改变的量。

## 6. 交付顺序

1. **`visual::opacity` + `visual::translate` + `visual::scale`**（第 5 节）—— 4 与 7 的共同前置；
2. **公开 painter 入口**，用它实现下划线（7 的最小可用切片，**验收用例**：下划线必须只走 L2）；
3. **锚点**（第 2 节）—— 下划线要的"钉在底部、左右拉伸"靠它，而不是靠动画宽度；
4. **形状节点**：`Rectangle` → `Circle` → `Line`；
5. **盒子模型 / margin**（第 4 节，与锚点一起收尾）；
6. 文档：把第 1 节的代价分级写进组件文档，让"别动画 font_size"成为可查的规则。

> 第 6 条（handler 的 concept 化与开发体验）不属于本文范围，它是 authoring/DX 的独立课题。

## 7. 明确的非目标

- **多边形 / 任意路径**：用到再列（需要扩设备接口）。
- **锚定到兄弟节点**：父容器锚定覆盖已知需求。
- **CSS margin collapsing**：不学。
- **旋转**：`scale`/`translate` 之外暂不做，需要时再加一条路径。

## 8. 验收：下划线

它同时压到本文的每一条，所以拿它当验收用例：

```cpp
// 一条钉在文字下方、能淡入的下划线。全程 L2，没有任何一帧触发重排。
auto underline = ui.make<widget::Rectangle>()
                     .anchors(widget::AnchorSpec{
                         .left = 0.0F, .right = 0.0F, .bottom = -2.0F})
                     .height(2.0F)
                     .fill_token(theme::ColorToken::primary)
                     .opacity(0.0F)
                     .behavior(widget::visual::opacity,
                               animation::motion::tween(motion.short_duration))
                     .build();
```

要求：**悬浮时改的是 opacity（或 scale），而不是宽度** —— 后者是 L3，会让这一页退回
"PPT 感"。这条要求本身就是本文存在的理由。
