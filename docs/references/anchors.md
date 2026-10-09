# Anchors 布局设计

> 状态：**scene 内核与作者入口均已落地并测试；真实窗口的人工验收待完成**。
> §2 / §4 仍是目标伪代码：锚线赋值表达式（`editor.anchor.left = …`）有意不实现，实际 API 见 §5.1 与 §5.3。
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
百分比冲突策略已经由现有布局边界测试固定：有限的父级 `max` 约束用于解析百分比；无界轴上的
百分比回退到当前内容测量，绝不读取上一帧尺寸。百分比 `min` / `max` 使用同一有限父级轴解析，
再参与尺寸钳制。锚定画布若在某轴无界，不能用自身内容反过来推导该轴的百分比尺寸；anchors
求解器必须拒绝这种循环关系，或采用文档化的非循环内容规则。

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

## 5. 已验证边界与实现前检查

### 5.1 内核落地契约（2026-10-08）

本轮先交付 `scene::NodeRef<T>`、`AnchorSpec` 和 `AnchorCanvas`，不把 builder 伪代码标为可用。
`NodeRef<T>` 以共享身份槽保存弱节点引用；复制引用共享同一身份，只允许绑定一次（含节点销毁后），
绑定类型在编译期检查。未绑定与已销毁分别报错；detach 不销毁引用，求解时重新核对直接父级。
默认构造、绑定都不依赖 RTTI；复制后的引用共享绑定槽，引用赋值被禁用以避免身份替换。
`ref.anchor.right + 12.0F` 与 `ref.parent.anchor.right` 是目标表达式；本轮用
`node.set_anchors({.left = target, ...})` 完整替换描述；节点自身 `.anchor` 门面与 builder 入口由 §5.3 记录。

画布实现位于 scene，只依赖现有布局协议。普通容器拒绝带锚点的子项（含 reparent 前置检查），
画布拒绝非 Control 子项；未写锚点的轴从零开始，尺寸仍由 `ControlSizeSpec` 解析。
画布不由子项撑开，测量必须得到两轴有限上界；可通过显式尺寸在无界父约束中提供有限上界。
隐藏节点仍参与锚定求解，隐藏只影响绘制/命中，因此隐藏目标不会使引用悄悄失效。

每轴允许单边、中心、或首尾双边；中心不能与边混用。双边与固定/百分比主尺寸冲突，
`fill` 和 flex 策略在画布直接子项上非法。双边跨度若违背 min/max，报错而非破坏锚线；
百分比 min/max 始终相对整幅画布解析，不相对跨度解析。单边尺寸继续复用 `measure_layout()`。
兄弟依赖以节点为单位拓扑排序（v1 保守拒绝节点级环，含跨轴互相依赖），不读旧位置或 global bounds。
全部子项求解成功后才写入 layout rect，失败不部分移动子项；测量缓存不在此原子保证内。
运行时多节点切换须使用画布的批量 `set_child_anchors()`：先校验候选描述及解，再一起替换描述；
实际几何在下一次布局提交。用户自定义测量/布局回调的副作用不属于事务回滚范围。
批量入口单独命名，避免与画布自身继承的 `set_anchors({})` 清空操作产生重载歧义；编译期断言锁住此调用。

错误使用异常报告，包含源节点名称（未命名则节点身份）与失败原因。前向引用允许先声明/挂载，
但第一次求解前必须绑定；detach/reparent 后下一次求解必须重验，禁止静默删除关系。
离树 builder 组装允许提前测量（例如 Column::add 会 relayout）；挂载到普通父节点时拒绝锚点，
活动树中带锚点的根节点也不能绕过父画布检查。求解失败后画布保留布局脏标记，允许修正后重试。

当前可编译的基础用法（包含 `scene/anchor_canvas.hpp` 和 `scene/node_ref.hpp`）：

```cpp
auto canvas = std::make_shared<scene::AnchorCanvas>();
auto sidebar = std::make_shared<scene::NanControl>();
auto editor = std::make_shared<scene::NanControl>();
scene::NodeRef<scene::NanControl> side(sidebar), edit(editor);
sidebar->set_width(scene::percent(30.0F));
sidebar->set_anchors({
    .left = side.parent.anchor.left,
    .top = side.parent.anchor.top,
    .bottom = side.parent.anchor.bottom,
});
editor->set_anchors({
    .left = side.anchor.right + 12.0F,
    .right = edit.parent.anchor.right,
    .top = edit.parent.anchor.top,
    .bottom = edit.parent.anchor.bottom,
});
canvas->add_child(editor); // 可以先添加依赖者，不依赖插入顺序
canvas->add_child(sidebar);
// 将 canvas 交给父布局或 scene tree，由其提供有限布局尺寸。
```

### 5.2 测试与故障注入

`tests/anchors_tests.cpp` 已注册为 unit；10 个用例覆盖本轮 scene 内核。
下列故障注入逐项编译并执行过，每项均导致对应测试失败；注入后已还原并核对源文件 SHA-256。

| 测试 tag | 故意改坏的位置 | 观察到的失败 |
| --- | --- | --- |
| `[ref]` | 关闭 `NodeRef::bind` 的重复绑定检查 | 本应抛错的第二次绑定成功 |
| `[order]` | 求解阶段按插入顺序而非拓扑顺序遍历 | editor.x = 12，预期 132 |
| `[lifetime]` | 跳过目标归属检查 | 缺失目标变成 `unordered_map::at`，丢失确定性诊断 |
| `[validation]` | 每求出一个节点就立即 `layout_to` | 后续约束失败后，先求出的节点仍移动到 50，预期保持 0 |
| `[sizes]` | 关闭双边与显式尺寸冲突检查 | 固定尺寸冲突未报错 |
| `[nested]` | 测量画布时跳过有限上界检查 | 无界画布未报错 |
| `[mixing]` | 跳过 `reparent` 锚点前置检查 | 失败后 child.parent 变成 nullptr |
| `[batch]` | 跳过候选锚点描述的提交 | sidebar.x 保持 0，预期 300 |
| `[geometry]` | 跳过求解后的 `layout_to` | global_bounds.x 保持 0，预期 350 |
| `[parent]` | 跳过批量更新重复目标检查 | 重复更新未报错 |

### 5.3 剩余验收

#### 审核补充（2026-10-09）

`.anchors(source)` 首次执行若因非法关系抛错，effect 的创建必须回滚：从 Graph 释放刚注册的
effect 并移除依赖/待执行队列，不允许留下尚未纳入 ReactiveScope 的订阅。此规则归 reactive
公共设施，不能只在 anchors 门面绕过。测试须验证异常后改变 source 不会再修改节点，且原有
合法 effect 仍能继续工作；scope 清理后也不得恢复这条失败的绑定。

批量入口提交的是**描述**而非即时几何：随后标记布局脏，下一次 layout 才更新位置、命中与语义。
示例的状态提示可以在描述成功提交后发布，但回调内不能据此读取“已经更新”的 bounds。

#### 作者入口落地形状（2026-10-08 约定，同日落地）

约定与实际实现一致，差异处已注明：
- `ui.ref<T>()` 返回既有 `scene::NodeRef<T>`，`.bind(ref)` 只绑定兼容类型且只允许一次；
  不额外注册名称，也不把节点所有权交给引用。
- `ui.anchor_canvas()` / `authoring::anchor_canvas()` 构造同一个 scene 画布；`.children(...)`
  只挂载，不在前向引用尚未绑定时提前求解。
- 命名 builder 提供只读的 `.anchor.*` / `.parent.anchor.*`，与 `NodeRef` 共享同一种目标值。
  裸 scene 节点继续使用 `parent()`；直接持有 shared_ptr 的作者可显式构造 NodeRef。
  不在每个运行时节点上预分配十二条 DSL 锚线。builder 可复制构造，引用表达式不拥有节点；
  含只读关系门面的 builder 不提供赋值操作，请链式配置或用新变量命名。
- `.anchors(AnchorSpec)` 完整替换，`.anchors(source)` 将有 `get() -> AnchorSpec` 的响应式源
  绑定到同一 setter；后者要求 BuildContext，effect 由当前 scope 管理、目标为弱引用。
  不支持旧伪代码中的锚线赋值表达式，避免“给代理赋值却没更新节点”。
- 多节点切换用 `AnchorCanvas::set_child_anchors()`；本轮不引入自动延期的批量响应式绑定。
  示例在已布局后的点击回调中提交整组候选，再更新状态信号；初始化直接声明初始关系。
  新入口须覆盖复制 builder、离开 build 栈、scope 清理、节点销毁及无 RTTI 的消费者。


v1 不包含文本 baseline、旋转、约束求解器、跨锚定画布引用、按百分比指定锚点偏移，以及由
子项内容自动撑开的锚定画布。浮层 `AnchoredPositioner`、形状节点和盒模型也不属于本设计。

现有布局基础已经通过 `tests/anchors_boundary_tests.cpp` 固化为以下规则，anchors 实现必须复用：

- 有限父级轴解析 `PercentLength`；无界轴回退到当前内容测量，不使用上一帧尺寸；百分比
  `min` / `max` 与主尺寸使用相同的有限轴解析。
- `Column` / `Row` 等排列容器的不可见直接子项不参与测量、间距分配或布局位置计算。Anchors
  若允许锚定不可见节点，必须另写明确规则，不能让它意外改变排列语义。
- `ScrollView::vertical` 只给内容宽度有限约束、高度无界约束；`horizontal` 反之；`both` 两轴
  都无界。滚动通过内容节点的位置承载，不能改变内容的布局尺寸。视口仍负责裁剪。
- `reparent()` 完成 detach → attach 后，旧父和新父都被标记为需要重新布局；节点的 `parent()`
  与子项顺序立即反映新关系。未来锚点引用必须在 detach、attach、reparent 时重新校验，不能
  静默清空。

内核已覆盖引用生命周期、有限画布约束、尺寸冲突、兄弟依赖与环、布局系统混用、重复布局、
父目标随 reparent 更新，以及 bounds / hit test / semantics 一致性。作者层集成同日落地：

- `ui.ref<T>()`、builder `.bind(ref)`、只读 `.anchor.*` / `.parent.anchor.*` 门面、`.anchors(AnchorSpec)`
  与 `.anchors(source)`、`ui.anchor_canvas()` 均已实现于 `widget/authoring.hpp` 与
  `widget/build_context.hpp`，由 `tests/anchors_authoring_tests.cpp` 覆盖（11 用例，含审核补充）：
  复制 builder、离开 build 栈后的表达式、scope 清理、节点销毁后的弱绑定、无 BuildContext 报错。
- 应用级组合回归：`[page-root]` 覆盖"画布作页面根 + header/侧边栏/编辑区依赖链 + 批量换边"，
  `[scroll]` 覆盖画布在 ScrollView 中（无界轴必须显式尺寸），`[z-order]` 覆盖求解顺序不影响
  绘制与命中 z 序，`[nested]` 覆盖嵌套画布与跨画布拒绝，`[reactive]` 覆盖响应式替换。
- showcase 案例：`showcase/pages/anchors_page.cpp`（路由 `anchors`）以侧边栏停靠切换演示
  "树 ≠ 布局"，切换走 `set_child_anchors()`，先提交描述再发布状态，几何在下一次 layout 更新。
  侧栏宽度和页头高度分别最多占画布对应轴的 30%，不使用跨面板固定间隔（内容留白由 Card
  自己负责），避免小视口/零尺寸产生负跨度。编辑区说明随停靠边更新。
  `anchors-showcase` unit 直接编译此页面，覆盖缩小到 200×120 / 0×0、恢复大小、反复切换、
  描述/几何提交时机，以及页面销毁后仍被外部持有的按钮不会访问失效 NodeRef。

仍未完成：**真实窗口的人工体验验收**（含窗口缩放、快速重复点击与明暗切换下的手感），
以及 playground 侧的同类案例。自动测试不能替代这一项。

#### 已知边界：画布不能占据排列容器的剩余空间（2026-10-08）

画布要求测量时两轴已有**确定尺寸**：`on_measure` 采用父级给出的可用空间，`measure_layout`
会把已声明的确定尺寸（显式像素、可解析的百分比）收紧为有限上界，因此：

- **两轴显式像素尺寸的画布可以嵌入排列容器。** 例如 Column / Card 的即时 relayout 使用
  `measure_layout(loose())`，显式尺寸仍能把传给画布的上界收紧为有限值。
- **百分比不是无条件的确定尺寸。** `PercentLength` 与 `FillLength` 都只有在父级基有限时才能
  解析。单纯给画布设置百分比，在 `loose()` 下同样解析为 `nullopt`，因此会像 `fill` 一样报
  “finite”；不能用最终窗口尺寸推断离树构建时已有百分比基。`[arranged-parent]` 已覆盖此分支。

结论：**画布无法表达"占满排列容器剩下的空间"**。需要这种结构时，把画布放在布局根（页面根 /
`CanvasLayer` 的 layout root），再用同层兄弟锚点表达"header 之下的剩余区域"——
`showcase/pages/anchors_page.cpp` 就是这么写的。`ScrollView` 内容与浮层 surface 同样可用
（无界轴必须给显式尺寸）。

`tests/anchors_authoring_tests.cpp` 的 `[arranged-parent]` 用例锁定三支：显式尺寸正常求解，
`fill` 与无界百分比都抛 “finite”。要支持剩余空间画布，应单独设计排列容器的测量阶段与约束
传播（或明确非循环回退），不能从上一帧/未来窗口尺寸隐式取值；变更须先更新 §5.1 契约。

#### 审核回归与可复现注入（2026-10-09）

- `[failed-binding]`：原实现中安装非法锚点的 effect 抛错，但 scope.clear 后修改 source 仍会
  安装新锚点。`make_effect()` 在首跑失败时 dispose 已注册 effect 后，该测试通过。
- reactive 的 `[initial-failure]` / `[nested-initial-failure]` 还锁住捕获资源释放、订阅与排队
  清理，以及嵌套 flush 中初始化失败不破坏既有 effect；移除 catch 中 dispose 会失败。
- showcase 的 `[resize]`：移除侧栏 30% 上限，200px 视口中侧栏占满 200px（上限应为 60px）；
  `[expired]`：把 `side.lock()` 提到画布弱引用检查之前，页面销毁后点击会抛 “expired”。


第一阶段验收至少包括：侧边栏位置切换、同父兄弟填充剩余区域、嵌套排列布局、依赖顺序与环、
非法混用报错、重复布局稳定性，以及 global bounds / hit test / semantics 与布局矩形一致。
每条测试都应明确指出故意破坏哪条实现会使其失败；真实窗口手感由 showcase/playground 人工验收。
