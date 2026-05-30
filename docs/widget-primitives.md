# Widget Primitives 设计文档

> 状态：已校正（2026-05-29）
> 当前判断：Surface、Pressable、FocusRing 已有实际实现；Text 能力当前仍主要由 Label 吸收，尚未独立拆成 `widgets/primitives/text`。本文用于固定 primitive 与 control 的职责边界，避免后续控件继续把结构、交互状态和视觉样式混写在一起。

## 目的

NandinaUI 当前的 widgets 方向，不是简单堆更多“能用的控件”，而是先建立一套可组合、可复用、可被未来样式层统一消费的 primitive 体系。

这条路线与项目当前目标保持一致：

- 架构心智继续参考 Angular 的 `signal / effect / component`
- UI 组织方式继续参考 shadcn 的 primitives / composition
- 后续预留 `style.cppm` 这类类似 `style.css` 的样式层，用 primitives + tokens + 语义状态组织组件样式与行为

因此，primitive 文档需要先回答一个问题：

> 一个控件里的代码，哪些属于结构，哪些属于交互状态，哪些属于文本能力，哪些属于焦点可视化，哪些又应该留给未来的样式层？

## 术语约定

### Primitive

primitive 是可复用的低层积木，负责单一维度的能力，不直接承载完整业务语义。

常见维度包括：

- 结构容器
- 交互状态机
- 文本渲染能力
- 焦点可视化
- 布局组合

primitive 的重点不是“能单独出现在业务页面里”，而是“能被多个 control 复用，并且边界稳定”。

### Control

control 是面向页面作者的真实控件，通常组合多个 primitive，对外暴露更高层、更语义化的 API。

例如：

- Button = Surface + Text + Pressable state + FocusRing
- Input = Surface + Text editing + Pressable/focus state + FocusRing
- Field = Label + description + control slot + error/hint slot

control 可以有主题和语义状态，但不应该把本可复用的 primitive 能力再次私有化实现。

### Token

token 是设计系统的原子值和语义值来源，例如 spacing、radius、typography、outline color、focus ring width。

token 不直接处理布局、事件或绘制；它只负责提供值。

### 样式层

未来的样式层负责把：

- primitive / control 的语义状态
- theme token
- 视觉属性解析

统一连接起来。

它不应重新发明 primitive，而应建立在 primitive 已经收口的边界之上。

## 当前 primitive 地图

### Surface

Surface 是结构与视觉容器 primitive。

当前职责：

- 提供背景色、圆角、描边等基础视觉面
- 提供 padding，并把内容区约束收缩到 child
- 作为单内容区容器承载内部子树
- 参与 `measure() -> set_bounds() -> layout()` 主线

Surface 不负责：

- hover / pressed / focused / disabled 等交互状态机
- click / press / release 事件语义
- 文本测量与文本绘制
- 焦点 ring 的显示策略

适合的使用方式：

- 作为 Button、Card、Panel、Input 等控件的结构底座
- 作为“有背景和 padding 的内容面”使用
- 作为组合 authoring 节点的容器层

不适合的使用方式：

- 在 Surface 里直接塞 hover/press/focus 布尔分支，演变成半个 Button
- 在 Surface 子类里为了某一个控件临时私有实现 focus ring
- 把多种不相关语义都挂在 Surface 上，导致它膨胀成万能基类

一句话理解：

> Surface 负责“内容放在哪里、底面长什么样”，不负责“这个控件现在是什么交互状态”。

### Pressable

Pressable 是纯交互状态 primitive。

当前职责：

- 管理 hovered / pressed / focused / disabled 状态
- 响应 pointer / focus 事件并更新状态
- 提供 click / press / release / hover / leave 回调与 signal
- 为上层控件提供统一的交互状态来源

Pressable 不负责：

- 背景、边框、阴影、文本颜色等视觉绘制
- 文本内容与文本布局
- focus ring 本身的绘制
- 页面业务语义，例如 submit / destructive / primary button

当前实现状态说明：

- 目前 `Pressable` 仍是一个 widget 形态的 primitive
- 历史文档里出现过“Pressable 去布局化”的设想，但当前主线还没有切换到纯非 widget 组合状态机版本
- 因此，后续控件开发应以“Pressable 提供交互状态边界”来理解，而不是提前按未落地方案写接口

一句话理解：

> Pressable 负责“用户正在怎样与控件交互”，不负责“控件看起来应该变成什么样”。

### Text primitive

Text primitive 当前是“能力边界已明确，但模块还未独立拆出”的状态。

当前现实：

- 文本测量、换行、字体、overflow、真实绘制等能力，主要由 `NanFont` + `Label` 承担
- widgets 层还没有单独的 `widgets/primitives/text` 模块
- 因此，现在不能把 Label 完整等同于最终形态的 Text primitive

当前可视为 Text primitive 的能力边界包括：

- UTF-8 文本内容
- 字体族、字重、字号、颜色
- 单行 / 多行 / overflow / line height
- 真实测量与真实绘制
- 文本对齐与垂直对齐

不应混入 Text primitive 的内容：

- 点击行为
- hover/pressed/focus 状态机
- 背景与容器边框
- 表单字段级语义，例如 required/error/hint 布局

当前实践要求：

- 在真正拆出独立 Text primitive 之前，`Label` 先承担文本 primitive 的主要消费面
- 后续做 Label 状态映射时，应把“文本能力”和“Label 作为 control 的语义状态”分开考虑
- 如果未来拆出独立 Text primitive，Label 应成为对 Text 的语义包装，而不是继续自己私有一套文本系统

一句话理解：

> 当前 Text primitive 还是一条能力边界，不是一个已经独立成型的 widgets 模块。

### FocusRing

FocusRing 是焦点可视化 primitive。

当前职责：

- 只负责焦点描边的绘制
- 支持 active / color / width / offset / corner radius 等参数
- 作为可复用 overlay，被 Button、Input、Checkbox 等可聚焦控件复用
- 默认样式从 theme 的 `NanFocusRingStyle` 读取

FocusRing 不负责：

- 判断一个控件是否应该获得焦点
- 键盘导航、tab 顺序、focus policy
- hover / pressed / disabled 状态
- 主内容布局与背景绘制

当前实践：

- 第一处真实消费面已经是 Button
- 后续 Input、Checkbox、Selectable Row 等控件应继续复用它
- 不应再在单个控件内部各写一套 outline/border 焦点逻辑

一句话理解：

> FocusRing 负责“焦点怎么被看见”，不负责“焦点从哪里来”。

## Primitive 与 Control 的组合关系

下面这条分层是当前主线推荐的控件构建方式：

```text
Control
  = 语义 API
  + primitive 组合
  + 语义状态到样式的映射
```

对于典型控件，可以按下面的方式拆解：

### Button

建议心智：

- Surface：底面、padding、圆角、描边
- Text/Label：文本内容和测量
- Pressable 或等价交互状态源：hover / pressed / focused / disabled
- FocusRing：焦点反馈
- Button 自身：variant / size / loading / destructive / click 这些高层语义

Button 不应承担：

- 私有的焦点绘制系统
- 与其他控件重复的交互状态机实现
- 独立于文本 primitive 的第二套文本测量逻辑

### Input

建议心智：

- Surface：输入框底面和边框
- Text primitive：文本与占位符能力
- FocusRing：焦点可视化
- Input 自身：编辑语义、selection、caret、IME、validation 状态

### Card / Panel

建议心智：

- Surface：容器底面与 padding
- Text/Label：标题与正文文本
- Card / Panel 自身：标题区、内容区、accent/header 这些结构语义

Card / Panel 通常不是交互控件，因此不应默认塞 Pressable 或 FocusRing，除非它们真的是可聚焦/可点击容器。

## 何时应该复用 primitive

出现下面任一情况时，应优先复用已有 primitive，而不是直接在控件内部硬写：

- 这段逻辑未来至少会被第二个控件复用
- 这段逻辑描述的是结构、状态机、文本或焦点这类稳定维度
- 这段逻辑已经开始依赖 theme token 或组件状态
- 这段逻辑和控件业务语义无关，但和视觉或交互底层机制有关

典型例子：

- 需要 hover / pressed / focused / disabled：先想 Pressable
- 需要焦点描边：先想 FocusRing
- 需要背景 + padding + 单内容区：先想 Surface
- 需要文本度量与绘制：先想 Text/Label 能力边界

## 何时不该新增 primitive

下面这些情况不应急着抽 primitive：

- 只在单个控件里出现一次，且没有复用迹象
- 它本质上是某个 control 的高层语义，而不是低层能力
- 只是几个 setter 的简单打包，并没有形成稳定边界
- 它依赖具体业务场景，离 design system 复用还很远

例如：

- `ButtonVariant::destructive` 不是 primitive，它是 Button 的语义状态
- `Field` 不是 primitive，它是由多个 primitive/control 组合出的语义容器
- `SidebarGroup` 当前更偏结构化控件，而不是通用 primitive

## 与 token、theme、未来样式层的关系

当前建议的责任分层如下：

- primitive：定义可复用能力边界
- control：定义页面作者可直接使用的组件语义
- token/theme：提供原子值与语义值
- 样式层：把状态、token 和视觉属性解析接起来

可以把它理解为：

```text
token/theme 提供值
primitive 提供能力边界
control 提供语义 API
style layer 负责把状态和值映射到具体外观
```

以 FocusRing 为例：

- token/theme 决定颜色、宽度、offset 的默认值
- FocusRing primitive 负责把这些值绘制出来
- Button / Input control 决定何时激活 FocusRing
- 未来样式层决定在不同 theme / state 下，这些值如何被解析和覆盖

## 当前实现约束与后续演进

### 当前约束

- Text primitive 还未独立模块化，当前主要由 Label 承担
- Pressable 仍是 widget 形态，而不是纯非 widget 状态机
- Button 目前直接处理 pointer 事件，尚未完全收敛到“统一交互状态源 + 样式解析层”的最终形态
- 主题系统已有 primitive token 和部分 style primitive，但状态解析层还未完全建立

### 后续演进方向

- 补完 Label 的语义状态到样式映射
- 继续把 FocusRing 复用到 Input 等后续控件
- 评估是否将 Pressable 从 widget 收口为纯组合状态机
- 在 primitive 边界稳定后，推进更正式的 token resolver / style layer

## 实施规则

后续新增或重构控件时，至少应自查下面五个问题：

1. 这个能力是结构、交互状态、文本、焦点，还是控件特有语义？
2. 这段代码是否已经值得被第二个控件复用？
3. 这段逻辑是否应该由 primitive 承担，而不是继续写在 control 里？
4. 这段视觉属性未来是否需要由 token/theme 或样式层统一覆盖？
5. 如果今天不抽边界，后面做第二个控件时会不会复制一遍？

如果第 2、3、4、5 个问题里有两个以上答案是“会”，就应该优先考虑 primitive 化。

## 当前结论

NandinaUI 的 widgets 主线，不应继续走“每个控件各自带一套结构、状态、样式和焦点逻辑”的路线。

当前更稳的方向是：

- 用 Surface 固定结构容器边界
- 用 Pressable 固定交互状态边界
- 用 Text/Label 固定文本能力边界
- 用 FocusRing 固定焦点可视化边界
- 再在 control 层做语义化组合
- 最终让 token 与未来样式层统一消费这些边界

这也是当前 Phase B 要先做 primitive/control 收口，而不是继续快速铺控件表面积的根本原因。
