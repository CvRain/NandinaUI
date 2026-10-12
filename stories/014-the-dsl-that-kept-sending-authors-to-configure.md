# DSL 一边提供链式 API，一边把作者送回 configure

> 2026-10-12，D6 设计讨论归档。**本文记录方向和边界，不代表方案已经实现。**
> 作者侧设计与分期验收见 [作者 DSL、文本能力与内容测量](../docs/references/authoring_dsl_and_content_measurement.md)。
> 本篇依据当前工作区的 Label showcase 和 `ComponentInfoTable` 讨论；它们是作者的在研改动，
> 不在本次文档整理中改写。

## 一、写一个 Label 页面，却要同时讲好几种语言

作者开始给组件写介绍页，几行展示代码很快混成了几种不同层次：

```cpp
ui.make<widget::Label>("标题")
    .color_token(...)
    .font_size(34.0F)
    .width(fill)
    .configure([](widget::Label& label) {
        label.set_overflow(...);
        label.set_max_lines(...);
    });
```

外层用 `NodeBuilder`；字号和颜色是 builder 的平铺方法；换行与行数退回 `.configure()`；
想把已建节点给另一个 builder 时又出现 `.build()`；容器则有 `.child()`、`.children()`。
anchors 和响应式信号再加入后，复杂 UI 读起来很容易像“容器、builder、lambda、signal、容器”交错。

作者问的不是“能不能多加几个 builder method”，而是这套 API 会不会把人训练成两个坏习惯：
要么每个节点都只用 `.configure()`，要么绕过 DSL 直接命令式装树。我们需要的是有可预测边界的声明式表层，
不是给命令式 API 加一圈括号。

## 二、QML 给的是需求地图，不是复制清单

作者把 QML `Text` 的属性清单拿来对照：字体 family/weight/style、wrap、elide、对齐、padding、
截断与内容尺寸都很直观；而 `Label` 还能有背景。这个参照很有价值，因为它提醒我们：文本组件不只是
“一个字符串 + 一个字号”。但 QML 的某些属性背后依赖 Qt 文本引擎、平台字体信息、自动绑定和绘制器；
把属性名字搬进 Nandina 并不等于这些行为也存在。

因此我们把清单当成**能力访谈表**：每个候选属性都要回答谁能设置、谁能读取、它会让 paint/layout/shape
哪部分失效、主题和实例覆盖谁赢。对齐只改绘制时不应无端重整字形；字体家族可能改变度量；wrap 必须知道
最终宽度；elide、maximum lines 和 scale-to-fit 不能混成一个模糊的“文本模式”。

## 三、从 Slint 学到“属性是契约”，不是“属性越多越好”

我读了 Slint 仓库的 property、binding、container slot 文档和文本布局实现。最有用的不是某个语法，
而是它把几个边界写得很清楚：组件属性有输入/输出/双向/私有可见性；设置值是否替代 binding 是正式语义；
容器 children 在定义的 slot 处参与父布局；文本横向的首选/最小宽度和给定宽度下的纵向高度需要不同处理。

我因此不建议 Nandina 每个组件都开一套万能属性系统。先用 C++ 的类型约束和组件文档定出 Label 能力表，
确认多个组件确实共享重复模式后再提炼 helper。组件可以继承实现，但继承不是公共 API 许可。
`Label` 能调字体，不意味着 Button 的内部 label 也必须暴露一套无前缀字体字段；那一处更适合是受控的文本 part。

同样，作者提的“所有东西都继承一个 Component 空类”解决不了 DSL 接线：仓库已经有
`NanControl`、`ComponentTraits`、`BuildContext::make<T>()`、`NodeBuilder<T>` 的类型化路径。
先让 showcase 的 `ComponentInfoTable` 走这条正式创建、装入和 scope 清理路径；若它仍显示真实的重复样板，
再决定是否需要组件注册辅助。

## 四、核查发现，主题覆盖章节自己也不能照旧写

核对 `primitives::Text::set_style()` 后发现，它设置一整份样式，同时把颜色、字号、字体都标成显式。
旧的作者配置文档却把它描述为只写派生值而不置标记。这不是纯文案小错：这意味着现在通过一次
`set_style()` 后，样式上下文和组件默认值对这些字段的后续应用就被显式标记挡住了。

所以 D6 不用一句“最后写赢”掩盖现状，而是把完整 `TextStyle` 快照、字段级显式覆盖、theme/context 派生值、
reactive binding 和 reset 分开审阅。文档先修正源码事实，再要求实现批次用行为测试决定更好的规则。

## 五、匿名树不该强迫作者 `build()` 再 `children()`

作者提出把 `ui.row().children(label)` 收成 `ui.row(label)`。这是一条值得做的 ergonomic overload，
不是一次新的组件对象模型。现有 `.children()` 已经接受 builder 和 `shared_ptr`，并通过
`materialize()` 装入；工厂重载只需复用它和 `BuildContext` 的 callback/binding 准备规则。

我们保留无参工厂，循环、动态 children 和分阶段配置仍然自然；稳定的已知子树可以直接传入。子项先后由参数
位置决定，但 C++ 实参表达式的求值顺序不是左到右保证，所以有副作用的构建先写成有名字的局部值。
这个短语法能少一个方法调用，不会神奇地把十层匿名组件变清晰。

`.build()` 的名字也被指出不准确：对象在 `ui.make()` 时已经创建，`.build()` 返回节点的 shared handle，
不是才把组件完整构造出来。我们记录了这个认知问题，但不为改名而改名，先弄清 `.build()`、`.get()`、
`.expose()` 的 ownership 差异与调用者意图。

## 六、另外几项 UI 痛点是同一份尺寸契约的上下游

作者在同一个 showcase 里发现：

- AnchorCanvas 需要固定宽高，像素高度不适合响应式页面；
- `anchor` 既是只读关系门面，`anchors(...)` 又是写入 API，名字难区分；
- 两节点锚定的边距不便表达；
- `anchor_canvas` 更像实现名，不像页面区域；
- 滚动 + padding + 线性/锚定布局每次都要手工套好几层；
- 信息表要边框，却没有一致的公共 box 作者入口。

如果立刻把这些组合成 `.section().scroll().layout().children()`，我们只会把问题盖起来。检查发现
`ScrollView` 会在滚动轴放宽约束、非滚动轴传有限约束；文本的自然高度取决于它最终拿到的宽度；
`AnchorCanvas` 则明确需要有限两轴尺寸。当前即时 `relayout(loose())` 路径不是一个能随便传进新 Section
就获得 width-for-height 正确性的通用两阶段 layout 协议。

因此先做有限宽度文本的测量/缓存/排列集成，并用 Label 页面和信息表验证；之后才定边框归属、锚点 margin
和区域组合 API。`AnchorCanvas` 改叫 `Section` 不会自动赋予它 viewport 分配行为，名字不能冒充能力。

## 七、最后收敛出的阶段次序

这批讨论把 D6 从“DSL 怎么写得短”收敛为一个可以逐段证明的作者体验项目：

1. 核对并冻结 Label 的公共能力表、主题覆盖/绑定优先级与编译边界；
2. 建立有限宽度下 text measure → Column/Card → ScrollView 的自然长高、缓存与失效契约；
3. 重写 showcase Label 页面和 `ComponentInfoTable`，做窄宽度、主题切换与人工视觉验收；
4. 在实际重复模式下再增加 `ui.row(child...)` 等容器工厂糖，并决定共享 border/Surface 模型；
5. 之后才设计能真正承载 viewport、滚动和内容槽的 Section/region composition；
6. 最后清理不需要保留的旧名字和重复配置路径，迁移示例、测试及用户文档。

本次归档**没有实现 D6 API**，也没有宣称测量或 Section 支持已完成；只把方向和验收写成可审阅设计。
因此 `row(children...)` 是已接受的目标形状，待约束与集成测试一起落实。下一轮先审阅 Label 作者能力表、
共同属性规则和有限宽度自然长高契约，再决定第一批代码改动。
