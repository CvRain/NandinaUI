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

### 5.0.1 待决：动画模块边界（`scene ↔ animation`）

L2 逼出的真正决策。三个选项：

| 选项 | 做法 | 代价 | 评价 |
| --- | --- | --- | --- |
| **A 拆层（推荐）** | 把**纯值级**动画类型（`Easing` / `Behavior` / `SpringSpec` / `Keyframes` / `AnimatedProperty`）下移成只依赖 `foundation` 的低层；`animation` 只留调度与绑定（`AnimationHost` / `Group` / `PropertyEndpoint`） | 移动约 5 个头文件 + 更新 include 与 `module_dependency.md` | **消掉**已登记的偏离，且节点级属性与其它动画共用同一套策略词汇 —— 不会出现两套动画机制 |
| **B 宿主所有权上移** | 动画宿主交给窗口 / `app` 驱动（`module_dependency.md` 的建议之一） | `scene` 不再引用 `animation`，但**没解决**节点要存动画状态的问题 | 只解决一半 |
| **C 在 scene 定义最小推进接口** | `scene` 声明非模板的"可推进值"接口，`animation` 实现 | 节点级只能拿到 tween；spring / keyframes 需类型擦除或虚调用；**出现第二套动画机制** | 可行但不自洽 |

**推荐 A。** 理由是它把偏离**消掉**而不是绕过，并且避免"节点级一套、别处一套"的分裂 ——
后者正是这个项目反复吃亏的形状（同一个概念两处定义，然后漂移）。

**这一条待作者决定后，才动第 1 步以后的代码。**

现状只有 6 条（`label.color/font_size` + `container.fill/border_color/border_width/radius`）。
按第 1 节的分级补齐 **L2**：

| 新路径 | 值类型 | 说明 |
| --- | --- | --- |
| `visual::opacity` | `float` | 任何节点。底层 `NanNode2D::set_local_opacity` 已存在，只是不可动画 |
| `visual::translate` | `NanPoint` | 相对布局位置的偏移，不改布局 |
| `visual::scale` | `NanPoint` | 分轴缩放。缩放中心由**静态**配置 `.transform_origin(...)`（九宫格枚举，同 QML `transformOrigin`）给出，它不是动画 Path（见 §5.0 ④） |

`scale` 与 `translate` 不触发重排，但**会改变 `global_bounds()`** —— 命中测试要跟着走
（这是期望行为，不是副作用）。`font_size` 留在 L3/L4，不推荐用于动效。

补齐 L2 之后，005 里那些"造好了没接出去"的能力开始有用武之地：`Group` 的
`stagger`（错峰入场）、`Keyframes`（脉冲/呼吸）都需要能廉价改变的量。

## 6. 交付顺序

```text
0. 标签归属 + 动画边界（§5.0.1 决策）+ base/presentation 组合规则
1. presentation transform 数据模型
2. opacity 路径与动画
3. translate
4. scale + transform_origin
5. 命中、语义 bounds、布局抵抗测试
6. home_page 迁移 + showcase 真实窗口手感验收
7. 三套构建验证、故障注入、文档归档
```

第 0 步的三条不落定就不要往下走：**标签归 `scene`、动画边界的决策、以及
"布局 transform 与表现 transform 的唯一来源"**。否则 `opacity` 会顺利通过，
而 `translate` / `scale` 会在中途把 API 与存储模型推倒重来。

> 第 6 条（handler 的 concept 化与开发体验）不属于本文范围，见
> [配置与开发体验](authoring_configuration.md)。

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
