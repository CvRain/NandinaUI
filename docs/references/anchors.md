# Anchors 布局设计

> 状态：**设计共识，尚未实现**。本文记录当前作者模型与实现边界，不代表 API 已可用。
> 公开 C++ 名称和 builder 调用均为伪代码，最终形状应在实现前按现有 authoring API 校准。
> 本文是 anchors 的权威 reference；[旧讨论草案](anchor_draft.md)保留问题脉络，但与本文冲突时以本文为准。

## 1. 目标与定位

Anchors 为场景树增加一种通过相对几何关系安排节点的布局系统，与现有 Row / Column / Grid
等排列布局并列。排列适合表达顺序和间距；anchors 适合表达“侧边栏贴左、编辑区接在侧边栏右侧”
这样的空间关系。引入 anchors 后，节点树可以组织组件，而不必复刻屏幕上的空间结构。

一个容器对其**直接子项**只使用一种布局系统；容器内部可以选择另一种系统。因此可以用锚定
画布安排侧边栏和编辑区，再由侧边栏内部的 Column 排列标题、列表和页脚。Anchors 不会改变
`NanControl::on_layout()` 的既有默认行为；使用 anchors 必须显式选择锚定画布。

Anchors 只求出子项的 layout rect，并通过现有 `layout_to()` 交付。不得把结果写成 presentation
translate，也不得用 `global_bounds()` 作为输入。这样重复布局、变换缓存、命中和语义几何仍走
现有布局与场景树契约。

浮层定位器 `AnchoredPositioner` 是另一项能力：它把 Popover / Tooltip 等放在触发控件附近，
不属于本文的场景树 anchors。

## 2. 作者模型

### 2.1 锚线与关系表达

v1 包含六条几何锚线：`left`、`right`、`top`、`bottom`、`horizontal_center`、
`vertical_center`。文本基线 `baseline` 暂不纳入：它只对部分节点有意义，且不应让低层 scene
依赖 text 或 widget。将来如有明确用例，再定义跨层的基线协议。

锚线通过组件的 `.anchor` 属性访问。目标是显式可读的，避免把 `.left` 与几何坐标或普通字段
混淆：

```cpp
sidebar.anchor.left
editor.anchor.left = sidebar.anchor.right + 12_px;
editor.anchor.right = editor.parent.anchor.right;
```

`parent` 是可布局组件的基本关系属性，表示直接父节点，不是“最近的锚定祖先”。所以
`self.parent.anchor.left` 指直接父容器的左边；父容器不是锚定画布时，不能因此越级寻找别的目标。

表达式左侧是当前节点的锚线，右侧是目标锚线，可附加逻辑像素偏移。水平和垂直关系分别验证；
例如 `editor.left = sidebar.right + 12` 表示编辑区左边缘位于侧边栏右边缘向右 12 个逻辑像素处。

### 2.2 匿名声明式组件与目标身份

不引入只为锚点服务的字符串 `id`。匿名 DSL 中可预先声明一个非拥有型、类型化 `NodeRef<T>`，
再把它绑定到 builder 声明的节点；引用只用于建立关系，不拥有节点，也不延长节点生命周期。
以下是表达意图的伪代码，`.bind()` 等具体名称不构成 API 承诺：

```cpp
auto sidebar = ui.ref<widget::Column>();
auto editor = ui.ref<widget::Editor>();

auto page = ui.anchor_canvas()
    .children(
        ui.column()
            .bind(sidebar)
            .anchors(/* left, top, bottom */),
        ui.editor()
            .bind(editor)
            .anchors({
                editor.anchor.left = sidebar.anchor.right + 12_px,
                editor.anchor.right = editor.parent.anchor.right,
                editor.anchor.top = editor.parent.anchor.top,
                editor.anchor.bottom = editor.parent.anchor.bottom,
            }));
```

引用不要求 builder 节点必须有变量名；普通命名组件也可直接使用 `component.anchor.*`。实现必须
让未绑定、已销毁或不属于同一锚定画布的引用可被确定性检测，不能回退到名称查找或静默清空。
兄弟目标限定为同一锚定画布中的直接子项；跨容器引用和自引用非法。挂载、解绑与 reparent
时重新校验引用关系，错误应指出节点及失效目标。

### 2.3 尺寸与百分比

锚点描述只表达位置关系，不持有 `width` / `height` 的第二份当前值。固定、填充、百分比及
min/max 尺寸继续归现有 `ControlSizeSpec`。在一个轴上，单边锚定保留该轴由尺寸模型确定的尺寸；
两侧锚定可确定可用跨度，与同轴显式固定尺寸冲突时必须明确报错，不能默默忽略任一来源。

百分比复用 `PercentLength`，相对于当前锚定画布的**内部可用布局尺寸**计算，而不是窗口尺寸、
屏幕尺寸或全局 bounds。因而侧边栏最大宽度可以写作画布宽度的 30%。百分比适用于尺寸约束
（包括 min/max）；锚点 margin / offset v1 使用逻辑像素，不支持百分比。

若画布在某轴上的可用尺寸尚未确定，或百分比尺寸会反过来依赖由该内容撑开的画布尺寸，必须
由布局约束阶段拒绝或给出明确的非循环规则；不得用上一帧尺寸作为隐式回退。具体的无界约束与
百分比冲突策略是实现前待验证项。

## 3. 布局规则

### 3.1 显式画布与逐层选择

锚定画布是显式组件/容器。普通 `NanControl`、现有排列容器及其默认 `on_layout()` 不因 anchors
加入而改变。排列容器中设置锚点，以及锚定画布中使用 `Expanded`、流式 `fill` 等排列专属属性，
属于布局系统冲突，必须在能确定上下文的阶段报错并给出修复方向；不得静默忽略。

嵌套不冲突：锚定画布的子节点本身可以是 Column，Column 的子节点再按排列规则布局。

### 3.2 求解与非法关系

锚定画布先确定自己的内部可用尺寸，再测量不由双边锚定决定的子项尺寸，最后按依赖关系解析
子项矩形并调用 `layout_to()`。双边锚定决定的轴不再由内容隐式尺寸反向撑开。兄弟引用按依赖
顺序求解；环、缺失目标、跨画布目标和矛盾约束必须确定性报错，不能死循环、依赖插入顺序或
使用前一轮缓存结果。

`set_anchors(spec)` 的语义是完整替换该节点已有的锚点描述，而不是合并字段。一次布局模式切换
若要同时修改多个节点，应先整体校验，再原子提交，避免中间状态违反父子布局约束。

边距与偏移均为逻辑像素；v1 不做 CSS margin collapsing。`fill` 用四边锚定表达，不另造一套
“铺满”几何概念。锚点求解只关注布局矩形，表现层 translate / scale 在布局之后叠加。

### 3.3 绘制顺序、命中与语义

场景树已经有 `z_index`、稳定的同级顺序，以及绘制/命中排序机制。Anchors 不新增第二套 z 系统；
实现只需确认锚定画布子项仍复用现有顺序，且绘制与命中一致。锚点不改变裁剪和滚动语义：
它不跨容器引用，也不因目标被裁剪或滚动而改变求解坐标。锚点产生的 layout rect 通过既有路径
更新 global bounds、命中与语义 bounds。

## 4. 例子：侧边栏与编辑区

以下伪代码展示目标关系。实际 API 名称和 builder 形状以未来实现为准：

```cpp
auto sidebar = ui.ref<widget::Column>();
auto editor = ui.ref<widget::Editor>();

auto page = ui.anchor_canvas()
    .children(
        ui.column()
            .bind(sidebar)
            .max_width(widget::authoring::percent(30.0F))
            .anchors({
                sidebar.anchor.left = sidebar.parent.anchor.left,
                sidebar.anchor.top = sidebar.parent.anchor.top,
                sidebar.anchor.bottom = sidebar.parent.anchor.bottom,
            })
            .children(header, list, footer),
        ui.editor()
            .bind(editor)
            .anchors({
                editor.anchor.left = sidebar.anchor.right + 12_px,
                editor.anchor.right = editor.parent.anchor.right,
                editor.anchor.top = editor.parent.anchor.top,
                editor.anchor.bottom = editor.parent.anchor.bottom,
            }));
```

侧边栏内部仍可用 Column 排列；切换侧边栏位置时调整关系即可，不要求重塑整棵组件树。
若目标是右侧停靠，侧边栏改锚到父级右边，编辑区左右边关系随之调整。

## 5. 非目标与实现前检查

v1 不包含文本 baseline、旋转、约束求解器、跨锚定画布引用、按百分比指定锚点偏移，以及由
子项内容自动撑开的锚定画布。浮层 `AnchoredPositioner`、形状节点和盒模型也不属于本设计。

实现前仍需用现有布局约束和生命周期代码验证以下边界，并把答案写成可测试规则：

- `PercentLength` 遇到无界画布约束、min/max 冲突及双边拉伸时的具体解析顺序；
- 隐藏/不可见子项是否参与测量与锚点求解；
- `NodeRef<T>` 在声明、绑定、销毁、detach、reparent 和类型不匹配时的精确诊断；
- 锚定画布尺寸由父布局提供时的约束传播和重复布局稳定性；
- 锚点与现有 overflow / ScrollView 的交叉行为是否完全由容器边界限定。

第一阶段验收至少包括：侧边栏位置切换、同父兄弟填充剩余区域、嵌套排列布局、依赖顺序与环、
非法混用报错、重复布局稳定性，以及 global bounds / hit test / semantics 与布局矩形一致。
每条测试都应明确指出故意破坏哪条实现会使其失败；真实窗口手感由 showcase/playground 人工验收。
