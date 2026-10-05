# 节点表现层

> 本文同时记录设计与落地状态。§5 的 L2 节点表现路径已经实现；anchors、shape 与盒模型
> 仍是规划。要改这里的 API 之前先改本文。
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

## 2. 定位：两套对等系统，以 anchor 为主

QML 里 layout 和 anchor 是**两套对等**的布局系统，而且分工是明确的：**anchor 为主、
layout 为辅**。本项目目前只有 layout。缺的不是"给个别子项开个后门"，是**一整套范式**。

### 2.1 最深的后果：树不再是布局

```text
只有 layout：  结构 == 排列。层层包裹层层递进：大框架 → 小框架 → 组件。
               树的形状就是 UI 的形状。

有 anchors：   结构 ≠ 排列。空间关系由"谁贴着谁"表达，
               因此可以是一个**扁平**的树。
```

这一条决定了下面全部设计 —— 包括为什么需要显式 z 序、为什么必须有兄弟锚定。

### 2.2 两者如何调和：**逐层治理**

不是"一个容器里既有排列又有锚定"，而是**每一层选一套**，子层可以用另一套：

```text
窗口（锚定画布）
├── sidebar        anchors: left / top / bottom
└── editor_region  anchors: left = sidebar.right, right / top / bottom
    └── RowLayout            ← 这一层改用排列
        ├── editor
        └── preview

sidebar（Column）            ← 容器自身被父层锚定，内部用排列
├── header
├── ListView
└── footer
```

这正是 markdown 编辑器那种三块结构：**大方向的关系（侧边栏贴左、编辑区贴右）用锚点，
局部序列（header / list / footer）用排列。** 侧边栏要在左在右，只是换锚点。

> **规则**：一个容器对自己**直接子项**只用一套 —— 要么是锚定画布，要么是排列容器。
> 子项若本身是容器，它内部可以换成另一套。
>
> 为什么不在同一层混用：会有"这个子项到底算不算在流里"的歧义，而 QML 是明确禁止
> 在 positioner（Row/Column/Grid）内部使用锚点的。逐层治理既确定又自洽，排除了
> 一整类"看起来生效、其实被排列覆盖"的困惑。

### 2.2.1 同层混用的失败模式：**目前是静默失效**（决定：改成精确报错）

两个方向都**不生效**，而且现在**都不报错**：

| 写法 | 现状 |
| --- | --- |
| 在排列容器（`Row` / `Column` / `Grid`）里给子项设锚点 | 锚点被忽略 |
| 给一个已锚定的节点用排列属性（`Expanded`、`width(fill)` …） | 排列属性被忽略 |

**决定：不再静默，改成精确报错。** 理由是本项目已经被同一类失败咬过多次 ——
配方 `apply_rule` 漏写、20 份 `same_text_style`、`activate` 永远非空、侧边栏那对死信号、
`guarded()` 的假保护。**"设置成功但永远不生效"在这里是最坏的选项**：它把一份错误的
UI 变成一次没有线索的调试。

分两级，都不会"太严苛"，因为矛盾**两侧的信息都已知**，消息能直接点名：

| 冲突 | 检测点 | 处理 |
| --- | --- | --- |
| 同一节点、同一轴：锚点与显式尺寸并存（如 `left+right+width`） | 设置时 | 抛 `std::invalid_argument` |
| **父子语境**：锚点 vs 排列容器 | **挂载时**（那时两侧都知道） | 报错并**点名节点与父容器**，消息给出两条出路：去掉锚点，或用非排列容器包一层 |

"太严苛"的担心化解在**可修复性**上：两种冲突都是一行能改好的（`clear_anchors()`
或加一层容器），而错误消息直接写出这两条出路。这比"设置成功但不生效"好得多。

### 2.3 求解：先隐式尺寸，再按依赖序解锚点

采纳 QML 式求解，**不引入约束求解器**（Cassowary 那类表达力更强，但是另一个工程）。

1. 每个节点先解出自己的**隐式尺寸**（内容 / 显式宽度 / 排列结果）；
2. 再按锚点的依赖关系做**拓扑排序**求解位置与尺寸；
3. **检测锚点环**并确定性报错（QML 的 "Anchor loop detected" 就是为此）。

第 3 条不是加分项：没有它，一次笔误就是死循环或未定义行为。

### 2.4 兄弟锚定是 v1 必需（修正）

没有兄弟锚定就表达不出"内容区占掉剩下的空间"：

```cpp
editor_region.anchors.left = sidebar.right;   // ← 核心用法，不是边角
```

上一版本文把它列为"留到有需求再说"，那是按"锚点只是装饰"的视角做的判断，错了。

### 2.5 z 序

结构不再等于排列 ⇒ **绘制顺序不能从树上推出来** ⇒ 需要显式的 `z`（同 QML `Item.z`）。
同层同级按加入顺序，`z` 不同的按 `z` 排。命中测试与绘制共用同一顺序
（与 [溢出与裁剪契约](overflow_and_clip.md) "绘制与命中共享一份语义"一致）。

### 2.6 API 形状

```cpp
struct AnchorSpec {
    // 水平：三选一 —— 或 left+right 拉伸。目标缺省为父容器。
    std::optional<Anchor> left;
    std::optional<Anchor> right;
    std::optional<Anchor> horizontal_center;
    std::optional<float>  width;             // 与 left/right 互斥
    // 垂直：同理
    std::optional<Anchor> top;
    std::optional<Anchor> bottom;
    std::optional<Anchor> vertical_center;
    std::optional<float>  height;
};

struct Anchor {
    float margin = 0.0F;          // 距目标边的距离（CSS 意义上的 margin）
    scene::NanNode2D* target = nullptr;   // nullptr = 父容器
    // target 指向兄弟时，则 "贴住它的哪条边"由使用的位置决定：
    //   anchors.left = {.target = sidebar}  → 贴住 sidebar 的右边缘
};
```

指向兄弟时**贴哪条边由字段决定**（`anchors.left` 就是"我的左边缘贴目标的右边缘"），
避免再多一个枚举维度。目标用**指针**优先（builder 链里本来就持有句柄），跨构建顺序时
用 `set_name()` + 名字兜底。

### 2.7 冲突与非法组合：当场拒绝

（与上一版相同，保留）

| 组合 | 处理 |
| --- | --- |
| `left` + `right` + `width` | 抛 `std::invalid_argument`（三者只能取二） |
| `left` + `horizontal_center` | 抛（都是水平定位来源） |
| 锚点指向**自己**或形成环 | 抛（求解期检测） |
| `fill` 语义 | 用 `left+right+top+bottom` 表达，不引入第二个概念 |

### 2.8 `Expanded` 的定位（决定）

`Expanded` **保留**，但它的语义收窄为一个**组件**，而不是布局系统的一部分：

- 它有**固定的 width / height**，行为等价于 Qt Widgets 的 `QSpacerItem`；
- **放进排列容器才有作用**（吃掉剩余空间、把两侧推开）；
- **在锚定画布里它什么也不做** —— 就是一个固定尺寸的占位盒子。

这样分工干净：跨容器的空间关系用锚点，容器内部的"推开/占满"用 `Expanded`，
不会出现"两个工具都能干同一件事"的困惑。

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

### 5.0 前置：开工前必须落定的四条

L2 看起来只是"加三条 Path"，核对代码后发现四个更底层的前置。**不先落定它们，
`opacity` 能顺利落地，而 `translate` / `scale` 会在中途迫使 API 与存储模型返工。**

#### ① 标签归属：下移到 `scene`

`opacity` / `translate` / `scale` 属于**所有** `NanNode2D`，不属于 widget。所以标签定义在
`scene`，由 `widget::visual` 重导出，应用侧写法不变（仍是 `visual::opacity`）。

**保留"部分 + 字段"两级**，不要放宽成"字段 + 值类型" —— 前者才装得下以后的 rotation /
clip / z。内部可以是 `PropertyPath<node_t, opacity_t, float>`，对外仍导出成
`visual::opacity`，不强迫用户写 `visual::node.opacity`。

（同一手法已用过两次：`TextAlign` 因 `theme::TypeStyle` 需要而下移到 `theme`。
本层需要的东西就放到本层，见 [组件公共契约](component_contract.md) §8。）

#### ② **阻塞点**：没有可复用的节点级动画存储

- `NanNode2D` 只有 `transform_` 与一个**普通** `local_opacity_`（无动画状态）；
- `PropertyEndpoint` 在 `animation`，只接受 `NanControl&`，依赖 `AnimationHost` / `SceneTree`；
- 而 `scene` 与 `animation` **互相引用**是 [模块依赖规则](module_dependency.md) 已登记的偏离。

**不能在 `NanNode2D` 里复制一份 endpoint 逻辑绕过去。** 这是唯一会阻塞开工的一条，
决策见 §5.0.1。

#### ③ `translate` 不能复用 `transform_.position`

`NanControl::layout_to()` 每次布局都调用 `set_position(...)`。动画若写 position，
下一轮布局会**静默覆盖**动画值。必须分成两层，并明确复合顺序：

```text
layout/base transform × presentation translate × origin × presentation scale × -origin
```

`translate` 是相对布局结果的偏移、`scale` 是表现变换 —— **两者都不能成为第二套布局位置**。

#### ④ `transform_origin` 是静态配置，不是可动画 Path

九宫格枚举没有自然的插值语义。公开 `.transform_origin(...)`，但**不支持**
`.behavior(visual::transform_origin, ...)`。需要"跳变式"动画时那是另一个特性。

九宫格锚点**每次按节点当前的布局尺寸解析**，不要在设置时就固化成像素坐标 ——
否则重新布局后中心点会停在旧尺寸上（容器变小会看到缩放中心偏出去）。这条要作为
实现约束写进代码注释，并进布局抵抗测试：**布局 → 设 origin → 再用不同尺寸布局一次 →
缩放中心跟着新尺寸走**。

### 5.0.1 已落地：动画模块边界（`scene → foundation`）

L2 逼出的真正决策。**这里不是"三选一"，而是三个正交决策加一个粒度决策**。
把它们混成一个选项，会导致"做完 A 环还在"：

| 决策 | 问题 | 结论 |
| --- | --- | --- |
| **A 值存储** | 动画值与策略放哪一层 | **A1**：纯值设施下移到 `foundation::motion`；`animation::*` 保留兼容别名 |
| **B 宿主所有权** | `AnimationHost` 由谁拥有、由谁每帧推进 | **B1**：宿主与 Group 下移到 `scene`，仍由 `SceneTree` 拥有 |
| **C 调度接口** | 调度器如何容纳不同值类型与插值策略 | **C1**：只类型擦除 `tick(dt)` / `finish()` 推进动作 |
| **D 宿主粒度** | 宿主接受什么 owner | **D1**：脏标记设施上提，owner 泛化为全部 `NanNode2D` |

这四条必须一起看：只下移值类型并不能消除调度环，宿主与 Group 也必须归 `scene`；同时
owner 泛化到 `NanNode2D`，否则“任何节点可动画”的公开承诺仍不成立。C1 擦除的是推进动作：

```cpp
TickResult tick(float dt);
void finish();
```

实现内部仍是同一份 `AnimatedProperty<T>`，所以 tween / spring / keyframes 没有分裂成第二套
机制。`DirtyFlags` 存储与传播也已从 `NanControl` 上提到 `NanNode2D`。

#### 已采用的组合

**A1 + B1 + C1（+ D1）**：

```text
foundation: motion 值与策略（Easing / Behavior / SpringSpec / Keyframes / AnimatedProperty）
scene:      AnimationHost / AnimationGroup / PropertyEndpoint + 节点表现值存储
animation:  兼容入口与作者侧 motion DSL，单向依赖 scene / foundation
```

依赖方向变成单边 `animation → scene → foundation`，`tree.advance_animations(dt)` 的
调用位置不变。B1 而不是 B2 的理由是前者不动现有帧循环，窗口层不必知道动画存在。

**验收条件是可检的**：`nandina/scene/` 下不再出现任何 `animation/` 的 include
（`.cpp` 也要查）。

这个条件本身就替我们决定了一件事：`animation_host.hpp` 现在就 `#include "group.hpp"`，
而 `Group`（错峰、延迟、时间轴）是**调度容器**，所以 B1 必须把 `Group` 一起下移 ——
否则 `scene → animation` 依然存在，验收条件不成立。留给动手时再定的是
"作者侧句柄怎么组织"，不是 `Group` 的归属。

源码验收已满足：`nandina/scene/` 内没有指向 `animation/` 的 include。

### 5.0.2 值类型支持矩阵（已落地）

| 类型 | Tween | Spring | Keyframes |
| --- | --- | --- | --- |
| `float` | 是 | 是 | 是 |
| `NanPoint` | 是（已补 `lerp`） | v1 否 | 是（同一条 `lerp`） |
| `NanColor` | 已支持（OKLCH 最短弧） | 否 | 已支持（同样走 `lerp`） |

补 `NanPoint` 那条 `lerp` 时踩到的坑值得留着：`motion::lerp<T>` 的约束是
`std::is_arithmetic_v<T>`，另有一个 `NanColor` 重载，而 `NanPoint::lerp` 是**静态成员**
（`geometry.hpp:97`）—— 成员函数不参与 ADL，所以名字在 `Tween<NanPoint>::tick()` 里不可见。
**缺的不是数学，是名字可见性**：现在 `motion/tween.hpp` 里有一条转调静态成员的重载。
`NanInsets::lerp` 与 `NanTransform2D::lerp` 同样是静态成员，将来要动画时走同一条路。

Spring 只给浮点值是 `AnimatedProperty` 侧的限制（`set_spring` 约束
`is_floating_point_v<T>`），与 `lerp` 无关 —— 分轴 spring 是 v2 的事。

**弹簧推进契约**：`foundation/motion::Spring<T>` 使用目标固定期间的阻尼振子解析解，
分别处理欠阻尼、临界阻尼和过阻尼。API 与状态所有权不变；改目标保留速度，
`finish()` 与收敛阈值不变。非正或非有限 `dt` 不推进；有限正 `dt` 全量消费，
不截短时间、不进行无界子步循环。中间运算与内部速度使用 `long double`（高刚度时，
有限位移对应的速度可能超出 `float` 范围），过阻尼采用负实根的
指数形式，避免 `cosh` 溢出及慢根相减消失。测试覆盖帧分割一致性、卡顿后恢复、
高刚度/低质量、零阻尼与临界点两侧。

旧单步欧拉在刚度 300、`dt = 0.1s` 时可从 1 冲到 -2；呈现钳制只能限制有限值的
显示范围，不能修复积分稳定性、NaN 或保证收敛，不能作为积分器的保护措施。

### 5.0.3 已落地的三条 L2 路径

原有 6 条（`label.color/font_size` + `container.fill/border_color/border_width/radius`），
按第 1 节的分级补上 **L2**：

| 新路径 | 值类型 | 说明 |
| --- | --- | --- |
| `visual::opacity` | `float` | 任何节点。tween / spring 都支持；**呈现值钳制在 `[0,1]`**，见下 |
| `visual::translate` | `NanPoint` | 相对布局位置的偏移，不改布局 |
| `visual::scale` | `NanPoint` | 分轴缩放。缩放中心由**静态**配置 `.transform_origin(...)`（九宫格枚举，同 QML `transformOrigin`）给出，它不是动画 Path（见 §5.0 ④） |

`scale` 与 `translate` 不触发重排，但**会改变 `global_bounds()`** —— 命中测试、语义 bounds
与坐标转换都要跟着走（这是期望行为，不是副作用）。`font_size` 留在 L3/L4，不推荐用于动效。

**opacity 的超调契约**：`set()` 钳制的是**目标值**，弹簧的中间值不受钳制（欠阻尼弹簧本来
就会过冲）。钳制发生在**呈现边界** —— `NodePresentation::opacity()`（进而
`NanNode2D::local_opacity()`，也就是绘制读到的那个值）恒在 `[0,1]`，而动画值
（`OpacityProperty::value()`）保留真实物理状态。这样过冲既不泄漏到绘制，也不打断弹簧的
收敛过程。测试用一条欠阻尼弹簧（ζ≈0.35）同时断言"物理值确实冲过 1"和"呈现值没超过 1"——
只断言后者会在弹簧被换成过阻尼时静默失去意义。

补齐 L2 之后，005 里那些"造好了没接出去"的能力开始有用武之地：`Group` 的
`stagger`（错峰入场）、`Keyframes`（脉冲/呼吸）都需要能廉价改变的量。
组合调度的同一节点 L2 作者入口见 §6.1；Keyframes 的作者入口仍未开放。

### 5.1 L2 的脏标记契约

三条路径的脏标记**必须写死并测试**，否则实现者很可能只标 `paint`：

```text
opacity：              paint
translate / scale：    paint | transform
纯语义变化（名称/描述）： semantics
measure / layout：     始终不标
```

**`transform` 与 `semantics` 是分开的两个位，方向也不同：**

- `transform` 表示"这个节点的**有效变换**变了"（local transform、表现层 translate/scale、
  缩放中心、以及会改变缩放中心的布局尺寸）。它做两件事：让几何缓存失效（自身与全部
  后代），并让语义 bounds 需要重建 —— 所以 `transform` 单向**蕴含** `semantics`。
- `semantics` 单独出现表示只是名称、描述、角色这类非几何信息变了，**绝不去动变换缓存**。
  否则每改一次无障碍文案都要重算整棵子树的变换。
- `measure` / `layout` 永远不在这两条路径里：L2 是表现层变换，不参与布局。

反过来的方向不成立，也不能做：`semantics ⇒ transform` 会让无障碍改动变成几何改动。

**为什么必须分开（而不是"标 semantics 顺带失效缓存"）**：那种耦合下，
"动画期间只标 `paint`"这个看似安全的优化会**连缓存失效一起跳过** ——
缓存的 `global_transform()` 不再重算，**bounds 与命中会冻结在最后一次带 `semantics` 的值上**，
而不只是无障碍数据变旧。故障注入验证过：把 `translate` 的脏标记去掉 `transform` 后，
`[mid-flight]` 在第一帧就红（`global_transform().position().get_x()` 停在 0，
而动画值已经走到 3.33）。

已知成本与后续优化：动画期间每帧都会 `mark_semantics_dirty()`，也就是每帧重建语义树
（约 60 次/秒）。**做"落定时才刷新语义"的优化时，要降的是语义重建，不是 `transform`** ——
`transform` 必须每次变值都发，否则几何缓存就废了。

> **持久脏位的消费周期**：`mark_dirty()` 会读取传入的 transform/semantics 位，
> **当场**失效缓存、置语义树标志，且每次调用都必须执行，不能被已经置位的状态短路。
> 存入 `dirty_flags_` 后，生产调度目前只读取 `layout_dirty_flags`；另外三个位只是
> 契约与观测面。而且它们**粘滞**：只有 `layout_to()` 会清 `paint`，`semantics` 与
> `transform` 置上就不再清。
>
> 所以别把这套位当成"帧的开关"来读：绘制是无条件发生的（窗口每帧都 `render`），
> 布局也只认 `layout_dirty_flags`。加新读者之前先想清楚"什么时候清"。

## 6. 交付顺序

L2 范围（0–7）已全部落地；以下顺序保留为同类能力的实现模板。
组合调度的交付范围与剩余能力单独列在 §6.1：

```text
0. 标签归属 + 动画边界（§5.0.1 的 A/B/C/D 决策）+ `NanPoint::lerp`（§5.0.2）+ base/presentation 组合规则
1. presentation transform 数据模型
2. opacity 路径与动画
3. translate
4. scale + transform_origin
5. 命中、语义 bounds、布局抵抗测试
6. home_page 迁移 + showcase 真实窗口手感验收
7. 三套构建验证、故障注入、文档归档
```

第 0 步的**四条**不落定就不要往下走：**标签归 `scene`、动画边界的 A/B/C/D 决策、
`NanPoint` 的 `lerp`、以及"布局 transform 与表现 transform 的唯一来源"**。
否则 `opacity` 会顺利通过，而 `translate` / `scale` 会在中途把 API 与存储模型推倒重来。

> 第 6 步里"showcase 真实窗口手感验收"**只有作者本人在真实会话里能判定** ——
> playground 自检与容器里的冒烟测试替代不了它。
>
> 第 6 条（handler 的 concept 化与开发体验）不属于本文范围，见
> [配置与开发体验](authoring_configuration.md)。

### 6.1 同一节点的组合调度（experimental）

`NodeBuilder::group(spec)` 已提供同一节点 L2 的作者入口，返回可复制的弱节点
`NanAnimation` 句柄。`widget::authoring::to(...)`、`parallel(...)`、`sequential(...)` 与
`stagger(...)` 描述目标与 Tween 规格；声明不改变属性，挂树后的 `play()` 显式触发。
设计与生命周期规则见 [同一节点的组合动画作者入口](animation_authoring.md)。

初始范围只包含 `opacity`、`translate`、`scale` 的扁平 Tween 组合。每次播放取得当前
节点与 SceneTree，再由稳定 endpoint 生成 clips；回调可以按值捕获句柄而不强持有节点。
离树或销毁时播放返回 `false`，重新挂载后句柄使用当前树。

- `PropertyEndpoint::clip(target, behavior)` 自带属性身份与脏标记。创建不改值；开始时
  借用稳定 endpoint，同步更新 endpoint 配置与实际 AnimatedProperty 的持久 Tween 行为。
- `AnimationHost::run()` 在仲裁前校验每个 clip 的 owner、重复身份和必要回调。接管普通
  轨道时保留当前值；与旧组重叠时先整体完成旧组，再安装新组。
- 普通 setter 命中任一组属性时完成并移除整个组。显式修改行为或 Spring 必须先完成旧组，
  再安装新配置，防止尚未开始的步骤覆盖作者的新设置。
- 离树、Host 清空和 reduced-motion 采用整组完成；播放时 reduced-motion 当场完成，
  不等待下一帧、不保留轨道。
- 公开入口测试贯穿真实指针事件、组合推进、几何/语义更新和实际重排计数；不能用仅测
  插值的底层测试替代作者路径。

未开放的部分：跨节点组合、嵌套时间线、组内 Spring、Keyframes 作者入口，以及浮层组件
的自动打开/关闭过渡。它们不能从本轮的同一节点入口推断为已完成。

## 7. 明确的非目标

- **多边形 / 任意路径**：用到再列（需要扩设备接口）。
- **同一层混用锚定与排列**：逐层治理是刻意的（第 2.2 节），不是没做；同层混用按第 2.2.1 节报错，而不是静默忽略。
- **约束求解器**（Cassowary 一类）：表达力更强但是另一个工程，QML 式依赖序足够。
- **CSS margin collapsing**：历史包袱，不学。
- **旋转**：`scale` / `translate` 之外暂不做，需要时再加一条路径。
- **`transform_origin` 的动画**：九宫格枚举没有自然插值语义，先只做静态配置（§5.0 ④）。

## 8. 验收：markdown 编辑器

定位范式的验收用例**必须能证明"树 ≠ 布局"**，所以用三块结构的编辑器，而不是下划线：

```text
窗口（锚定画布）
├── sidebar        anchors: left / top / bottom
└── editor_region  anchors: left = sidebar.right, right / top / bottom
    └── RowLayout                       ← 这一层换成排列
        ├── editor
        └── preview
```

验收要同时成立：

1. **侧边栏换边**只需改锚点（`left` → `right`，`editor_region` 跟着改成
   `left = parent.left, right = sidebar.left`），**不需要改树的形状**；
2. 侧边栏宽度可以由内容隐式决定，`editor_region` 仍然正确占掉剩余空间；
3. 侧边栏内部用 `Column` 排 header / list / footer，**不受外层锚点影响**；
4. 写错锚点（成环、`left+right+width` 同时给）**确定性报错**，不是未定义行为。

**冒烟用例（下划线）**：一条钉在文字底部、能淡入的 2px 矩形。要求全程 L2 ——
悬浮时改的是 `opacity` 或 `scale`，**而不是宽度**（那是 L3，会让整页退回"PPT 感"）。
它同时也是公开 painter 入口的第一个消费者。
