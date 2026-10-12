# 作者 DSL、文本能力与内容测量

> 状态：**D6 讨论已汇总，设计方向已接受，详细 API 与测量契约待审阅；未实施。**
> 本文是设计输入，不是当前用法文档。示例中的新入口均为提案，不能当作可编译 API。
> 现有接口与已知边界以源码和组件文档为准。本设计不要求长期兼容旧 DSL；改变公共接口时，
> 库内示例、测试和使用文档应同步迁移。
>
> 讨论过程见 [story 014](../../stories/014-the-dsl-that-kept-sending-authors-to-configure.md)。

## 1. 问题与目标

作者在 showcase 中编写 Label 页面时遇到的摩擦，不是简单地“少几个 setter”：

- 高频、稳定的字体与文本配置得先查 `primitives::Text` 或 `.configure()`；
- Label 与 Button 等组件的文本部分共享能力，但不应由此把所有 Text 内部细节铺到所有组件上；
- 常用 DSL、`.configure()` 命令式逃生口、`.build()` 取得节点句柄、anchors、响应式绑定和内嵌 lambda 混在同一棵树里，嵌套越深越难读；
- 需要描边的组件和容器没有一个足够清楚的公共作者模型；
- `AnchorCanvas` 要求有限尺寸，而滚动内容中的有限宽度文本应自然测出更高的内容高度。若先用固定高度绕过测量问题，Section 语法糖只会把脆弱结构藏起来。

D6 的目标是让作者能从类型化 API 中发现一个组件**承诺支持**的常见能力，同时把低频、特化能力留给明确的高级入口。目标不是复制 QML 或 Slint 的完整属性表，也不是让所有东西变成 builder 链。

## 2. 代码现状（设计起点）

以下是 2026-10-12 审核时的仓库事实；它们描述代码，不表示已满足 D6 目标。

| 事实 | 位置 |
| --- | --- |
| `NodeBuilder` 的常见 setter 由 `requires` 检测节点能力；`.configure()` 同步执行，并且仍可直接调用节点方法 | [`authoring.hpp`](../../nandina/widget/authoring.hpp) |
| `.children(...)` 复用 `materialize()`，同时接受 `NodeBuilder<T>` 与已构造节点指针；`.child(...)` 是单子项对应入口 | [`authoring.hpp`](../../nandina/widget/authoring.hpp) |
| `BuildContext::make<T>()` 通过 `ComponentTraits<T>` 注入服务，或要求 `BuildContext` 感知构造函数；自定义节点可继承 `NanControl` 并经同一构建路径创建 | [`build_context.hpp`](../../nandina/widget/build_context.hpp)、[`component_traits.hpp`](../../nandina/widget/component_traits.hpp) |
| `Label` 公有继承 `primitives::Text`；字体、文本、测量、溢出等接口可从基类调用，但 Label 的头文件只显式声明少量入口 | [`label.hpp`](../../nandina/widget/label.hpp)、[`text.hpp`](../../nandina/widget/primitives/text.hpp) |
| Text 持有完整 `TextStyle`；`TextOverflow` 当前同时包含 clip、ellipsis、wrap、scale；`TextLayoutInput` 使用布局约束 | [`text_layout.hpp`](../../nandina/text/text_layout.hpp) |
| Builder 已有 `.font_size()`、尺寸、颜色 token、文本对齐等受约束入口，但没有完整的 Label typography 配置表 | [`authoring.hpp`](../../nandina/widget/authoring.hpp) |
| Card 已消费 box style 和公共 BoxPainter；Row/Column 等排列容器没有同一套语义边框入口 | [`card.cpp`](../../nandina/widget/card.cpp)、[`box_painter.hpp`](../../nandina/widget/primitives/box_painter.hpp) |
| `Column` 以传入约束测量子项；`ScrollView` 在滚动方向放宽子项约束，并将非滚动方向约束传入 | [`layout.cpp`](../../nandina/widget/layout.cpp)、[`scroll_view.cpp`](../../nandina/widget/scroll_view.cpp) |
| `AnchorCanvas` 要求测量时获得有限的两个轴尺寸；无界 `loose()` 下没有可推导的画布视口 | [`anchor_canvas.cpp`](../../nandina/scene/anchor_canvas.cpp)、[anchors §5](anchors.md#5-实施与已知边界) |
| Showcase 的 Label 页面混合了 `NodeBuilder`、`.configure()`、`.build()`、固定高度、Column/ScrollView/AnchorCanvas 与逐项 `.children()` | [`label_page.cpp`](../../showcase/pages/label_page.cpp)、[`component_info_table.cpp`](../../showcase/components/component_info_table.cpp) |

### 2.1 一个需要纠正的文本样式事实

`Text::set_style(TextStyle)` **会把颜色、字号和字体都标记为显式设置**。字段 setter 则逐字段标记。
已有组件应用主题样式时也会调用 `set_style()`。所以当前这套 API 把“替换一份完整样式快照”
与“作者显式覆盖实例字段”耦在一起；本设计不把这一细节误记成 derived style setter，
也不假定 `set_style()` 后主题上下文会继续覆盖这些字段。

这是需要改进/定形的现状，不在文档阶段偷偷改变行为。D6 实施时必须分别证明：主题或父级样式
更新、完整样式替换、单字段覆盖、清除实例覆盖和绑定写入之间的规则。

## 3. 设计原则

### 3.1 按能力契约开放，而非按继承树自动铺开

类型继承可复用实现，但不自动承诺应用 API。一个组件要进入推荐 DSL，必须对它公开的每个字段
说明：

1. 谁提供默认值（主题、组件语义默认值或初始构造值）；
2. 哪些作者可以读、写或监听；
3. 变化影响绘制、测量、布局、语义中的哪些部分；
4. 与主题、配方、实例覆盖和响应式绑定如何协调；
5. 不适用的能力如何表达，而不是静默忽略。

不要求第一步就设计全新反射/属性元数据系统。先将 Label 的推荐能力表落成显式的组件文档、公开声明和
builder compile-time 检查；只有当多组件确实共享重复适配逻辑时，才引入集中 trait/helper。
不得用宏生成一整面 API，也不得把名字相似作为支持能力的依据。

### 3.2 直接 DSL 与高级配置是同一对象上的两层入口

- **直接入口**：高频、跨应用常用、语义稳定、效果可预测的组件承诺能力。例如 Label 文本、颜色角色、
  字体请求/字号、有限行宽与换行策略。
- **高级入口**：低频、多个字段成组、实验性或紧贴实现的能力，继续通过 `.configure()`、
  具名配置对象或直接组件方法使用。
- `.configure()` 是高级 escape hatch，不应成为使用基础能力的必经之路；同时不应为了让链条
  看起来完整而把所有 `set_*` 一对一复制成 builder 方法。
- 高级入口与短 DSL 必须落到同一份组件状态与同一规则上，不能形成“DSL 值”和“configure 值”两套事实来源。
- 构建器链不应承载长业务逻辑、连接多个无关信号或藏住生命周期敏感捕获。复杂逻辑先用有名字的
  局部组件/值拆开；长期复用的 UI 结构做成组合组件。

### 3.3 输入、输出、动作与绑定是不同概念

参考 Slint 的 property 可见性模型，应用端概念上至少区分：组件输入、组件输出、双向状态、一次性动作。
这不是照搬 Slint 语法：Nandina 的 C++ reactive `Signal`、`Property`、事件与构建器仍按各自的
所有权和生命周期工作。

- 输入：文本、配置、样式意图、子项；
- 输出：测量结果、解析后的字体信息、截断状态等只读事实；
- 双向状态：明确由使用者和组件共同写入的值，例如输入框 value；
- 动作：click、submit 等事件，不应伪装为可绑定属性；
- 绑定：声明状态源如何更新目标，必须讲明后来直接 `.set()` 是保留还是替代该绑定，以及清除绑定的
  入口和生命周期。

**待审阅决定：** 建议绑定维持一个明确的绑定状态/绑定入口；普通直接 setter 是临时覆盖还是解除绑定，
必须在实施前写死，不允许由调用先后偶然决定。保留既有 `[&]` 与异步回调捕获规则，见
[配置与开发体验](authoring_configuration.md) §1。

## 4. Label 作者能力表

表中“首批候选”是能力范围，不等于每个属性都必须做成无前缀的 `NodeBuilder` 方法。
推荐入口必须可从 `Label` 本身与组件文档发现。与多个组件共用的视觉部分可采用具名 part，
避免 Button、TextField 等将内部文本字段扁平化后发生名称冲突。

| 能力族 | 建议首批能力 | 返回/失效性质 | 默认放置 |
| --- | --- | --- | --- |
| 文本输入 | 内容设置和现有 reactive 文本绑定 | 更新 accessible label、测量与绘制；等值更新不应多余重排 | Label 直接 API + `ui.make<Label>(...)` |
| 字体请求 | family、weight、slant；项目当前字体请求能表达的字段 | 影响字体解析、字形与度量；不得承诺当前字体后端不能兑现的特性 | Label 的具名 typography/text part |
| 字号 | 逻辑字号（延续现有推荐能力）；是否加入缩放范围模式另审 | 通常触发重新整形与布局；动画若只改视觉 presentation 则沿现有动画契约 | Label 直接 DSL 或具名 text part |
| 字色 | 语义颜色 token；必要时实例显式色值 | 仅绘制变化不得使文字重新整形或重排 | Label 直接 DSL 或具名 text part |
| 水平对齐 | start / center / end / justify 中后端实际支持者 | 如确为绘制期偏移，只标 paint；须保持命中和选择坐标一致 | Label 的具名 text part |
| 换行 | 不换行、词边界、字符边界等候选模式 | 必须受有限可用宽度约束，影响行数与内容高度 | Label 的具名 text part |
| 溢出 | clip / ellipsis 等清楚区分的策略 | 与换行策略独立；定义省略、最大行数、指定高度之间关系 | Label 的具名 text part |
| 只读结果 | measured width/height、line count、truncated/overflowed | 输出只读；只在对应布局结果确定后有效 | Label 查询 API；不强制所有结果都 reactive |
| 文本装饰 | underline 等确有通用需求且布局后端能表达者 | 明确改变字形绘制还是文本语义 | 后续候选，非首批默认范围 |
| 背景/边框/内边距 | Label 是否能承担有背景和 padding 的 surface | 影响整体盒子测量和绘制，需共享盒模型 | 作为 Label 容器/装饰能力单独评审，不由“它继承 Text”决定 |

QML `Text` 的 API 清单是**需求发现清单，不是移植验收单**。候选项包括 family/weight/style、
wrap、elide、alignment、padding、截断与内容尺寸；其 base URL、富文本/格式、链接悬停、
字体 shaping features、variable axes、渲染类型、抗锯齿和字体尺寸自动适配等依赖 QML/Qt
文本栈、渲染器或编辑能力，当前没有据此承诺实现。

不要混淆以下独立模式：

- **wrap**：在可用宽度下增加行数、自然增加内容高度；
- **clip / ellipsis**：内容或行数不能容纳时如何省略或裁切；
- **scale-to-fit**：改变排版字号以适应指定盒子，是单独的布局/呈现策略；
- **line-height / maximum lines**：修改行距或限制文本内容高度，不能被 wrap 隐式代替。

首批应优先做可预测的 wrap + 明确 overflow，不自动把 scale-to-fit、point size、QML `Text`
中所有 font metadata 等引入 Nandina。

## 5. 共享属性的归属规则

公开能力归其**语义拥有者**，而非“所有组件都能调的便利函数”：

| 属性类别 | 典型拥有者 | DSL 规则 |
| --- | --- | --- |
| 内容、字体、文本对齐与换行 | Text/Label 文本 part | Button 等复合控件只暴露其确实允许的 label 配置，不泄漏其内部文本节点 |
| 尺寸约束、最小/最大尺寸、宽高比 | `NanControl`/布局系统 | 所有满足控制节点约束的组件共享统一尺寸入口 |
| 子项与插槽 | 组件的内容槽位 | 单槽使用 `.child(...)`；多项布局使用 `.children(...)`；禁止用一个泛型 setter 模糊槽位数量 |
| 位置关系 | scene anchors / 锚定画布 | 不把普通 `Row` 的排列项误包装为任意兄弟锚点容器 |
| 背景、描边、圆角、内边距 | surface/box part | 没有盒子的组件不应自动得到一整套装饰 API |
| 交互事件与只读语义 | 组件动作 / 输出 | 事件不当作属性；可访问状态不因隐藏视觉呈现而消失 |

用户希望 Label 与容器能容易描边。现有 `BoxPainter` 已共享绘制机制，Card 已有盒子配方；
因此先不要给 Row/Column/Grid 分别复制 `border_color` / `border_width` / `radius`。
实施前需选择一个公共盒子模型：明确的 `Surface`/边框装饰容器，或让选定容器拥有共用 box part；
对照 Card 的主题、padding、测量、裁剪、命中和绘制次序评估。不能只加 painter 参数而不定义盒子占用的布局尺寸。

## 6. 自定义组合组件：复用对象模型，不增加“空壳基类”

不建议另造一个没有明确语义的 `Component` 空基类。当前扩展模型已有：

1. 组件实现继承 `scene::NanControl`（或适合的现有控件基类），通过组合已有 widget 构造 UI；
2. `BuildContext::make<T>()` 将 context-aware constructor 或 `ComponentTraits<T>` 作为类型化创建点；
3. `NodeBuilder<T>` 可通过公共 `materialize()` 规则加入支持 `add()` / `set_child()` 的容器；
4. 若组件需要响应式 effect 或 callback，使用注入的 `BuildContext` / scope，明确由谁清理；
5. 公开重复使用的组件时，提供推荐 include、构造入口、slots、事件、语义、主题覆盖、测试与
   `docs/components/` 页面。

例如自定义 `ComponentInfoTable` 可以继续作为实现型组件由 `BuildContext` 创建，并作为
`NodeBuilder<ComponentInfoTable>` child 塞进 Row/Column/Card；无需为了 `.children()` 再造继承体系。
若某类开发者组件重复遇到登记样板，先用一个最小外部组件验证，再决定是否需要更薄的 traits 辅助。

**验收重点：** `ui.make<MyComponent>(...)` 应在缺失 traits/context-aware constructor、缺失推荐
公共头或不满足容器 child 契约时给出局部的编译诊断；不能通过一个任意 `std::shared_ptr` 转换
抹掉组件类型或绑定上下文。

## 7. 布局工厂的子项简写

接受的方向是为常见容器工厂增加接收子项的重载，而不是为 Row/Column 引入不同的子项构造语义：

```cpp
ui.row(a, b)
ui.column(title, body)
ui.stack(background, content)
ui.center(child)
ui.padding(insets, child)
ui.grid(2, a, b, c, d)
```

以上都是**目标形状示意**，不是已存在的重载。规则建议如下：

- `ui.row()` 与 `ui.row(a, b)` 并存，前者供循环追加和分步配置，后者用于已知的短子树；
- N 子项重载复用当前 `AddableChild`、`materialize()` 和 builder context 准备流程；单槽复用
  `SettableChild`；只接受可作为 child 的节点/builder，不以无约束模板误吞配置参数；
- 参数顺序就是插入顺序，但不承诺 C++ 函数实参表达式会按从左至右求值；有副作用的构造、信号注册
  或昂贵逻辑应先放入有名局部值；
- 子项加入后继续在返回 builder 上配置容器，需测试尺寸/间距 setter 触发的重新测量不会丢失子项
  或重复插入；
- dynamic range / reactive model 另作列表 API，不把不定范围偷偷适配成 variadic overload。

这会去掉 `.children(...)` 这一层方法调用，但不会消除大树的嵌套。局部变量、命名组合组件和合理缩进
仍是 DSL 可读性的组成部分。不要为了让任意树“一行写完”牺牲可调试性。

## 8. 锚点与区域组合语法

### 8.1 Anchor 命名与 margin

用户指出当前 builder 同时用 `.anchor` 作关系读门面、`.anchors(...)` 作写入配置，很难凭词形
区分。建议未来使用显式动词式的**写入口**，保留只读 `.anchor.left` / `.parent.anchor.left`
供关系表达。`anchor_to(spec)` 是候选，不是决定好的名字；不得让只读引用既是 getter 又是可变赋值代理。

锚点间距要支持可读的 top/right/bottom/left margin 或具名 Insets，但不应让一张 `AnchorSpec`
里出现不清楚的“目标边距 vs 子项外边距”。实现前应验证：

- 边距符号和方向对四条边、中心线一致；
- 同一轴双边约束下 margin 参与 span 求解，不能导致负尺寸或绕过 min/max；
- 只有中心线时偏移与 margin 不双重计入；
- 父级和兄弟级关系都用同一套明确规则；
- 错误诊断能指出冲突的锚与 margin。

### 8.2 `AnchorCanvas` 与“区域”抽象

`AnchorCanvas` 表意偏内部，实现中也要求两轴有限视口。后续可以引入应用侧 `Section` / `Region`
概念，为典型页面区域组合有限约束、内部布局策略与滚动，但不能先改名就宣称响应式布局已解决。

`section` 不能是仅靠 `enable_scroll(bool)` 组合任意子树的万能组件。优先使用显式枚举/策略，
使不可共存的模式在调用点可见；例如滚动轴与内部布局类型分别具有确定默认值。目标形状仅作讨论：

```cpp
ui.section()
    .scroll(widget::ScrollAxis::vertical)
    .layout(widget::SectionLayout::column)
    .children(...);
```

在实际 Section 设计前须确定它是否：

1. 只是作者层组合（ScrollView + Padding + Column/AnchorCanvas），还是拥有 scene 测量策略；
2. 接收一轴/两轴滚动，如何管理 viewport 与内容尺寸；
3. 对内部 anchors 给出有限画布尺寸，或拒绝尚不支持的区域模式；
4. 暴露 content slot 与可读的滚动/布局句柄，而非强迫用户访问匿名内部节点；
5. 保证同一份内容不会因创建时复制、build 后装配而丢失 bindings 或 callbacks。

若它只做结构糖，先证明展开后的等价性再推荐；若要新增响应式测量语义，必须纳入布局父子协议，
不能仅在 DSL 层暗中推断父窗口尺寸。

## 9. 核心前置：有限宽度下文本内容自然长高

“响应式侧栏最大为父宽度 30%”是**最大尺寸约束**，不是固定宽度，也不是“占剩余空间”规则：

```cpp
sidebar.max_width(percent(30)); // 当前 NodeBuilder 已有此重载
```

它与 `fill` 或兄弟 anchors 是不同模型。百分比必须有确定的有限父级基准；无界预测测量不能凭空
解出某个视口宽度。

Label/Section 的首要测量契约建议是：

1. 有限最大宽度向下传到文本；若作者显式给定内容/百分比/固定/填满尺寸，则按既有
   `LayoutLength` 及其有界基准规则解释。
2. 文字布局按最终可用宽度换行；在一个测量周期内，文本布局只由文本、字体/字号、换行/溢出策略、
   约束宽度及后端等实际排版输入决定。
3. 测量结果向上返回需要的宽度/高度；`Column` 在约束宽度下汇总自然行高，`ScrollView` 在非滚动轴
   保持 viewport 约束、滚动轴累计内容尺寸。
4. 整形布局缓存只在会改变行断点/字形度量的输入改变时失效；颜色、selection、cursor blink、
   纯绘制对齐等已能证明不影响布局的变化，不得重新整形或重排。
5. measure 可重复、无绘制副作用；measure 与 arrange 使用同一份确定的尺寸/约束，重复 layout
   不应累积 padding、边距或滚动范围。
6. `line_count`、`truncated` 等输出必须源于当前已提交布局结果；不得从旧测量缓存读出“看似有效”状态。

以上是需要审阅并落实的目标契约，不是说当前所有父项已具备两阶段的 width-for-height 协议。
正式实现应明确采用哪一种机制：父项先分配横向宽度、再以该宽度测量文本高度；或者把横纵轴测量依赖
记录成可重复的 measure phases。不可依赖一次无界 `loose()` 测量结果再把节点硬塞进较窄盒子，
因为那不会自动重算正确的换行高度。

尤其不要在下列情况下返回静默的 0、NaN 或上一次高度：无有限宽度却要求 wrap；百分比基准未知；
锚点 span 小于最小尺寸；父子测量出现循环。要么有定义良好的 intrinsic fallback，要么清晰诊断，
并在 reference 中写明二者的适用条件。

AnchorCanvas 仍是显式关系布局，不负责自动内容扩高。若父项想以 anchors 填充分配区域，父项须先
提供有限 viewport；若 Canvas 本身想由内容撑高，则须给出不循环、可重入的两轴依赖定义，不能让
子项上下边锚点同时依赖尚未测出的父高。

## 10. 边框与盒子模型

目标是作者能简单描边 Label 页面中的信息表与可复用 layout surface，但首批不要把 `border_*`
机械塞进 Row、Column、Grid、ScrollView、Label、Button 每个节点。

实现设计前比较并测试两种归属：

- 独立的 Surface/装饰容器：持有 fill/border/radius/padding，包一个子项，负责内容缩进和绘制；
- 受控的 box part：仅对被契约认定为有盒子的组件公开一份 shared style/presentation。

选定后须复用 box painter / theme semantic roles，确定背景、内容、边框、focus indicator 的绘制次序，
定义 border 是否占布局空间、圆角裁剪是否影响子项、命中区域和语义是否归容器，并验证设置 border
不会额外 reshape 文本。边框宽度只改变绘制还是也影响测量，必须单一且可预测。

## 11. `build()` 与创建语义

当前 `ui.make<T>()` / `NodeBuilder<T>` 中，`build()` 把已经构造好的对象句柄作为 `shared_ptr<T>`
交出去。它不执行组件“完整构建”或布局。因此当前命名容易让新用户误读。

改名的目的不应是改名本身。下一轮 API 评审比较 `.build()` 与更明确的 `.node()` / `.handle()`：

- 返回 shared ownership 还是只读/非拥有句柄；
- 是否注册回调和 reactive scope，行为由何时触发；
- build context 退出后的资源生命周期；
- 是否由 `.expose(out)` 或 `.get()` 已足够满足常见需求。

在证明真实认知收益与迁移范围前，不作机械 rename。新文档须区分“创建 builder”“将 child 装入树/构造页面”
和“读取组件句柄”。

## 12. Slint 与 QML 的学习边界

本轮阅读的 Slint 源码快照为 `c7d2d7e1dc917227d2199a1db964a74401dd5d88`，位于
本地 `/tmp/reference/slint`；它仅作为架构对照，不是依赖或 API 兼容目标。可复核路径：

- `docs/astro/src/content/docs/reference/language/properties.mdx`：组件属性区分输入、输出、双向及私有可见性；
- `docs/astro/src/content/docs/reference/language/bindings.mdx`：赋值替换 binding 的语义是显式契约；
- `docs/astro/src/content/docs/reference/language/container-components.mdx`：use-site children 有内容槽位，子项按插入位置和父布局参与布局；
- `internal/core/items/text.rs::text_layout_info`：横向 preferred/minimum 与纵向宽度约束测量并非同一个问题；文字自动换行需要可用宽度；
- `api/rs/slint/tests/text_layout_cache.rs`：文字、字号、字体/换行变化与颜色/溢出变化分别核验缓存失效，不把绘制变化一概视为重排。

借鉴的是**能力分类、显式绑定语义、slot 归属与测量回归精度**，不是 Slint 的编译器绑定机制或语法。

QML Text / Controls Label 的属性页说明了成熟文本控件通常包含哪些需求族，且 Label 继承文本 API
之外还能有背景与 inset/padding。但 QML 有自己的文本引擎、自动属性绑定、平台字体能力和控件层，
某属性在 QML 存在并不表示它适合由 Nandina Label 暴露。

参考：

- [Qt Quick Text QML Type](https://doc.qt.io/qt-6/qml-qtquick-text.html)
- [Qt Quick Controls Label QML Type](https://doc.qt.io/qt-6/qml-qtquick-controls-label.html)
- [Qt Quick Controls Control](https://doc.qt.io/qt-6/qml-qtquick-controls-control.html)

## 13. 分阶段完成流程

不将 DSL 收敛、文本整形、anchors、装饰和 Section 塞入同一提交。每批先更新文档与能力表，
再按开发流程自下而上实现。

| 阶段 | 目标 | 完成标准 / 防止的回归 |
| --- | --- | --- |
| 0. 审阅并冻结首批表 | 决定 Label 高优能力、配置/绑定覆盖规则、alignment 和 overflow 名称、border 归属、builder `build()` 是否改名 | 当前 API 与目标 API 差异表已核实；每个决定有明确组件/层/兼容边界；未决定项不假装冻结 |
| 1. Label 作者能力 | 把字体请求、颜色、对齐、wrap/overflow、maximum lines 和必要输出落到 Label 文档、组件 API、推荐 DSL | `Label` 不 include primitive 也能完成推荐用例；主题切换、部分覆盖、解除覆盖、绑定替换行为可测；不适用属性编译失败或有诊断 |
| 2. 测量与缓存契约 | 在 Text → Column/Row/Card → ScrollView 的真实链路落有限宽度自然长高 | 短文本/长文本/多语言与边界宽度的测量和绘制一致；resize 后行数/高度即时更新；绘制-only 更新不 reshape、不 remeasure；无界百分比和 anchors 循环明确报错或按已定 fallback |
| 3. Showcase 验收 | 重写 Label 页面并让 `ComponentInfoTable` 用推荐入口，不以固定高度遮挡行为 | 真实窗口调宽/调窄、换主题、切字体和滚动；属性表不溢出；可读、可维护且没有 configure 嵌套墙 |
| 4. 通用 DSL 与装饰 | 依照验收后的重复模式推出 `row(children...)`、单 child 容器便利重载，以及 box/border 入口 | 与无参 + `.children/.child` 完全等价；copy/move、binding scope、child 顺序、重测稳定；border 不重复造绘制/测量规则 |
| 5. Section / 组合组件 | 选定有限 viewport 与 scroll/content/布局配置的组合 API；更新锚点写入动词 | Scroll/resize/anchors 同步，生命周期与槽位清晰；内部只组合已验证组件；不隐藏尚未完成的 fill 剩余空间能力 |
| 6. 迁移与归档 | 批量迁移 examples/playground/showcase/docs/tests；删除因开发快速迭代而不需保留的旧接口 | 新入口独立 include / 下游编译测试；旧调用全仓无遗漏；没有死重载、空转发层或兼容测试导致假绿 |

阶段 1 与阶段 2 是核心使用链路，不要先以 Section 包装固定尺寸 workaround。阶段 4 的 `row(children...)`
已接受为方向，但需在第 0 阶段确认模板约束、context 装配和求值/顺序测试后再落代码。

## 14. D6 待审阅决定

1. 首批公开文本能力：是否纳入 family/weight/slant、justify、多行上限、只读 font info；
2. 绑定后的 setter / builder 新写入是否替代绑定，是否需要显式 `.override(...)` / `.clear_binding()`；
3. `TextStyle` 完整替换的定位与逐字段 explicit override / reset 的独立 API；
4. 统一 text part 与各组件自己的扁平入口边界，尤其 Button、TextField、ComboBox；
5. wrap minimum width（最长词、任意字符断行、ellipsis）与缺省宽度定义；
6. `anchor_to(...)` 或其他显式动词、margin 类型和符号；
7. Section 是组合糖还是新增 viewport-aware layout component；
8. 公共 border 是独立 Surface wrapper 还是选定组件的 box part；
9. `.build()` 是否重命名，以及 `.get()` / `.expose()` 各自的推荐用途；
10. generic authoring traits 是否已有足够重复证据，还是组件头文件显式声明更合适。

这些决定不能阻止 showcase 先把局部变量和现有 `.configure()` 用法整理得更易读；但不要在正式评审前
把待定名称当作 public API 推进。

## 15. 验收矩阵

| 场景 | 必须证明 | 改坏哪里应失败 |
| --- | --- | --- |
| Label standalone API | 仅 public controls/component header 即可使用契约的直接属性 | 某字段只存在于 `primitives::Text` 时，作者示例编译失败 |
| 错误组件能力 | Label 专属属性不能误施加到没有文本 part 的组件 | NodeBuilder 删除能力约束后静态检查变红 |
| 主题与显式覆盖 | 一字段覆盖不锁死其余派生字段；清除后恢复父/主题值 | 将批量 style 与 instance explicit 混为一态时变红 |
| 绑定生命周期/替代 | 源改变、目标直接写入、scope 销毁都按明确规则 | 绑定偷读悬垂变量或直接 set 静默复活 binding 时变红 |
| wrap natural height | 有限宽度收窄时 line count 和 measured height 同时变化、实际绘制落入同一盒子 | 只重算绘制换行或只更新尺寸猜测时变红 |
| 缓存边界 | text/font/width invalidate；color 或其他纯 paint 输入不 reshape | 将颜色放入 shaping key 时 miss counter 变红 |
| Layout integration | Column、含文本的 Card、垂直 ScrollView resize 后无裁切且滚动范围正确 | 任何一层丢有限宽度或固定旧高度时变红 |
| Anchor cycle/basis | 无界百分比与父高循环有定义诊断，不陈旧缓存成功 | 将旧尺寸当可用百分比基准时变红 |
| Row/Column factory sugar | 多 child 重载与逐项 children 插入后树顺序和布局相同 | 重复插入、binding scope 未准备、顺序变化时变红 |
| Custom component | `BuildContext::make<Custom>` 创建、加入 Layout/Card、移出树后 scope 正确清理 | 单测直接 `make_shared` 旁路推荐创建路径时该集成测试变红 |
| Surface border | box 测量、padding、圆角/clip、paint order、theme 与文本布局一致 | 只调用 painter 却未占位/裁剪/失效时变红 |
| Showcase 页面 | label 示例完整可扫读、响应式高度、手工主题/窄宽度可验 | 把关键内容重新固定高度时组件页边界测试或人工验收失败 |

逻辑测试覆盖有限/无界约束与布局缓存；离屏渲染检查实际 clip/overflow；最后由作者在 showcase
实机窗口检查排版节奏和手感。测试文件和 suite 仍按
[开发流程](development_loop.md) 注册，并遵守“先冻结改动再全量验证”。
