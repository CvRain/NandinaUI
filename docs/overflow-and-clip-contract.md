# Overflow / Clip Contract 设计文档

> 状态：草案（2026-06）
> 当前判断：`Text` 已具备自身 bounds 裁剪能力，`render::Scene` 与 `ThorVGSceneAdapter` 也已经存在 `PushClip / PopClip` 雏形；但当前主线仍缺少一套从 `runtime::NanWidget` 到容器组件再到 render backend 的统一 overflow / clip contract。因此本文的目标不是“立刻实现所有容器裁剪”，而是先固定一版能支撑 `Card / Dialog / Popover / ScrollView` 后续收口的基础边界。

## 目的

这份文档主要解决一个当前已经反复暴露的问题：

- 文本越界已经可以在文本层局部裁剪
- 但任意 child 的越界绘制还没有统一基础设施
- `Card`、后续 `ScrollView`、`Dialog`、`Popover` 等容器都需要“裁掉子内容”的能力
- 如果继续在单个 widget 内局部补洞，会把 overflow 语义打散到 widgets 层，放大后续 render 抽象与维护成本

因此，NandinaUI 需要一套统一的 overflow / clip contract，用来回答下面几个问题：

1. 谁负责声明“子内容应该被裁剪”？
2. 裁剪矩形/形状的语义落在哪一层？
3. 当前直连 ThorVG 的路径如何先接通，而不把临时实现固化成长期架构？
4. `Card` 这类结构化组件如何消费这套能力，而不是继续写局部特判？

## 当前现状

### 已经存在的能力

- `text::NanFont::paint(...)` 已支持可选 `clip_rect`
- `widgets::Text` / `widgets::Label` 已按自身 bounds 执行文本裁剪
- `render::Scene` 已定义 `PushClip` / `PopClip`
- `render::ThorVGSceneAdapter` 已能对后续图元应用当前 clip 栈顶

### 仍然缺失的能力

- `runtime::NanWidget::draw(...)` 仍是直接递归绘制，不具备统一 parent-child clip traversal
- `Surface` / `Panel` / `Card` 没有对“子节点裁剪策略”形成稳定公开 contract
- section 型组件（如 `Card`）还不能把 body/footer/header 的裁剪边界统一建模
- 当前 render 抽象仍未成为主线唯一绘制通道，因此 overflow 语义还没有稳定落点

### 当前问题的真实形态

当前最需要解决的，不是“任何 widget 都能做任意复杂 mask clip”，而是：

- 父容器声明 child overflow 是否可见
- 需要时，把 child 绘制限制在某个稳定边界内
- 这套行为在 runtime/widgets/render 三层之间保持一致

也就是说，当前问题更接近 **container child clipping contract**，而不是通用图形裁剪系统。

## 设计目标

### 1. 先固定“容器对子内容的裁剪 contract”

当前最重要的是：

- 让容器能表达“子内容是否允许溢出”
- 让容器能表达“裁剪使用哪个矩形/圆角边界”
- 让 widgets 作者不再自己拼接局部 clip 逻辑

### 2. 让 runtime 持有语义，render 负责执行

overflow / clip 的“是否启用、裁剪到哪里”属于 runtime / widget tree 的结构语义；
真正把裁剪落成后端调用，属于 render backend 的职责。

也就是说：

- runtime / widgets 负责声明裁剪边界
- render 负责把它翻译成 `PushClip / PopClip` 或后端等价实现

### 3. 兼容当前直绘路径，但不把 ThorVG 细节写死到 widgets

当前主线仍有大量 `NanWidget::draw(tvg::SwCanvas&)` 直绘逻辑，因此第一阶段不可能强制所有组件立即改成 Scene-only。

但这不代表 contract 应该直接设计成“每个 widget 都自己对 ThorVG 做 clip”。

更稳的方向是：

- contract 在 runtime / widgets 层定义
- 当前 ThorVG 直绘路径先实现一层过渡适配
- 后续 render 主线收口时，自然落到 Scene / DrawCommand

### 4. 支持结构化组件，而不是只服务单一容器

这套 contract 必须能服务：

- `Card` body/footer 的 child containment
- `Dialog` 面板内容裁剪
- `Popover` / `Select` 面板内容裁剪
- 后续 `ScrollView` viewport 裁剪

它不应被设计成只为 `Card` 的当前问题服务。

## 非目标

当前阶段不追求：

- 任意复杂路径 mask / alpha mask 裁剪
- 每个 widget 暴露完全自由的多段 clip path API
- 立即重写整个直绘路径为 Scene-only
- 立即实现完整的 scroll / viewport / sticky header 系统
- 在 `Card` 上继续新增大量临时 API 去回避底层缺失

## 术语约定

### OverflowBehavior

建议把容器对子内容溢出的策略固定为最小枚举：

- `visible`
- `clip`

`visible` 表示允许子内容越出父容器声明边界继续显示。

`clip` 表示子内容绘制应被限制在父容器给出的裁剪范围内。

当前阶段不建议一开始就把 `hidden / scroll / auto` 这些 Web 风格语义全部塞进来。`scroll` 更适合作为后续 viewport/scroll container 的更高层能力建立在 `clip` 之上。

### Clip Bounds

建议把“裁剪到哪里”理解为一个由父容器声明的稳定矩形边界，必要时附带圆角信息。

这不是 child 自己的 bounds，而是 **父容器为 descendants 声明的绘制可见区**。

### Child Clip Scope

裁剪语义应优先围绕“子树绘制范围”建模，而不是“当前 widget 自己怎么裁自己”。

原因是：

- 文本自裁是叶子节点特例
- 当前真正缺的是 parent 对 descendants 的 containment
- `Card` / `Dialog` / `ScrollView` 的问题本质都属于子树裁剪

## 推荐 contract

### 1. 在 runtime 层固定最小 overflow 语义

建议由 `runtime::NanWidget` 持有最小公开语义：

```cpp
enum class OverflowBehavior : std::uint8_t {
    visible,
    clip,
};
```

并提供对应访问接口：

- `set_overflow_behavior(...)`
- `overflow_behavior()`

默认值应为 `visible`。

理由：

- 保持向后兼容，避免现有 widget 默认行为改变
- 只有明确需要 containment 的容器才 opt-in

### 2. 用虚方法暴露 child clip bounds，而不是公开一堆 clip 参数

建议不要把“clip 到哪个矩形”“是否使用圆角”“content 还是 border box”一开始全部做成公开 setter。

当前更稳的做法是让 `NanWidget` 暴露一组可 override 的内部 contract：

```cpp
[[nodiscard]] virtual auto child_clip_rect() const noexcept
    -> std::optional<geometry::NanRect>;

[[nodiscard]] virtual auto child_clip_corner_radius() const noexcept
    -> float;
```

其默认语义建议为：

- `overflow_behavior() == visible` 时返回空
- `overflow_behavior() == clip` 时默认返回 `bounds()`

然后由 `Surface`、`CardBodySlot`、未来 `ScrollViewport` 等 override 成更具体边界。

这样做的好处是：

- 公共 API 先最小化
- widgets 可以按自己的结构语义提供 clip 区域
- 不需要先为所有组件暴露一整套复杂 clip 配置面

### 3. 区分“容器边界”与“内容边界”

很多容器并不应该默认把 child 裁到整个 `bounds()`，而应该裁到内容区。

例如：

- `Surface` 更适合裁到 `content_bounds()`
- `ScrollViewport` 更适合裁到 viewport rect
- `Card` body 更适合裁到 body section rect

因此 contract 不应写死为“clip = bounds”。

建议的默认理解是：

- `NanWidget` 提供“可声明 child clip rect”的基础协议
- 具体 clip 到哪一个 box，由容器类型决定

### 4. section 型组件通过内部 slot widget 承担细粒度裁剪

`Card` 这类组件不应要求 `NanWidget` 一次支持“header 一个 clip、body 一个 clip、footer 一个 clip”的通用多分区系统。

更稳的做法是：

- `Card` 继续承担语义模板职责
- 但把 body、footer 等需要独立 containment 的区域落成内部 slot widget / viewport widget
- 这些内部 slot widget 自己设置 `overflow_behavior = clip`

也就是说，推荐方向不是：

- 在 `NanWidget` 基类上做“任意数量 clip section”协议

而是：

- 用更细的内部结构 widget 表达多个 clip scope

这更符合当前项目“primitive + typed structure”的主线。

## 在现有模块中的落点建议

### Runtime

runtime 层负责：

- 持有 overflow 枚举与访问接口
- 定义 child clip rect / corner radius contract
- 在绘制 traversal 中建立 clip scope 推入 / 弹出时机

runtime 不负责：

- 决定后端如何调用 ThorVG / GPU API 执行裁剪
- 定义具体容器使用 content-box 还是 border-box

### Widgets

widgets 层负责：

- 决定哪些容器默认开启 `clip`
- 决定 clip rect 应来自哪一个结构 box
- 必要时通过内部 slot widget 细分多个裁剪范围

例如：

- `Surface` 可作为最常见的“内容区 clip 容器 primitive”
- `Panel` 默认可继续 `visible`，由上层按需开启
- `Card` 不应继续靠文本局部 patch 解决 body/footer 问题，而应逐步引入 body slot containment

### Render

render 层负责：

- 把 runtime/widget traversal 的 clip scope 降到 `Scene::push_clip()` / `pop_clip()`
- 或在当前过渡期，把同样的语义映射到 ThorVG paint clip

当前已经存在的 `PushClip / PopClip` 应被视为正式落点的预留位，而不是测试用旁路。

## 两条绘制路径的衔接策略

### 当前现实

项目目前同时存在两种心智：

1. `NanWidget::draw(tvg::SwCanvas&)` 直接递归绘制
2. `render::Scene` / `RenderBackend` 作为正在成型的抽象层

overflow / clip contract 必须能同时解释这两条路径。

### 推荐策略

#### 第一阶段：runtime contract 先成立，ThorVG 直绘路径先消费它

在正式 Scene 化之前，允许当前直绘路径通过一个轻量过渡层消费 `child_clip_rect()`：

- 进入 child subtree 前 push clip
- child 绘制完成后 pop clip

但这层逻辑应尽量集中在 traversal / backend bridge，而不是散落到具体 widget。

#### 第二阶段：统一降到 Scene clip commands

当 render 抽象进入主线后，应把 clip contract 统一翻译成：

- `PushClip`
- descendants draw commands
- `PopClip`

从那时起，widgets 不再感知具体后端，只表达 clip scope。

## 对现有组件的直接指导

### Surface / Panel

- `Surface` 应成为最自然的“可选内容区 clip 容器”基础 primitive
- `Panel` 继续保持中性语义，不额外引入 header/footer 结构能力
- `Panel` 是否默认 clip，不应凭当前 `Card` 问题匆忙决定

当前更稳的默认值仍建议是：

- `Surface` 默认 `visible`
- 由具体消费方 opt-in `clip`

### Card

`Card` 当前不应继续靠扩展更多 authoring API 来解决 overflow 问题。

更合理的方向是：

1. 保持 `Card` 作为结构语义组件
2. 把 body/footer 等结构区继续收口为更明确的内部 slot
3. 对需要 containment 的 slot 启用统一 `clip`

这意味着 `Card` 后续的稳定方案更接近：

- `Card` 定义结构
- body slot 负责 containment
- 文本 primitive 继续只负责文本层能力

而不是让 `Card` 自己无限追加局部绘制补丁。

### ScrollView（后续）

`ScrollView` 应建立在同一套 contract 之上：

- viewport 提供 clip rect
- content subtree 在 viewport 内绘制
- scroll offset 只是布局/坐标变换，不重新发明裁剪语义

## 建议的分阶段实施顺序

### Phase 1：固定 runtime contract

目标：让 overflow/clip 先成为正式语义。

建议产出：

- `OverflowBehavior`
- `set_overflow_behavior(...)`
- `overflow_behavior()`
- `child_clip_rect()` / `child_clip_corner_radius()` 基础协议
- runtime / render / widgets 对应文档与最小测试

### Phase 2：接通当前直绘路径

目标：让现有 ThorVG 递归绘制路径先能尊重 parent-declared clip。

建议产出：

- 在统一 traversal 位置推入/弹出 clip
- 验证 `Surface`/`CardBodySlot`/未来 viewport 的 child containment

### Phase 3：收口到 Scene / DrawCommand

目标：让 render 抽象成为 clip 语义的正式执行通道。

建议产出：

- runtime/widget traversal 降出 `PushClip` / `PopClip`
- backend 使用统一 clip stack 执行
- render regression tests 覆盖嵌套 clip 与圆角 clip

## 测试建议

至少应补下面三类测试：

### 1. Runtime / widget contract 测试

- 默认 overflow 为 `visible`
- 开启 `clip` 后能产生有效 child clip rect
- 容器 override 的 clip rect 与默认 bounds 有区别时仍被正确消费

### 2. Render clip stack 测试

- `PushClip` / `PopClip` 成对作用于后续命令
- 嵌套 clip 时以后入栈 clip 为当前有效边界
- 圆角 clip 与普通 rect clip 的行为一致可预测

### 3. Widget regression tests

- `Card` body child 在固定高度压力下不侵入 footer
- `Dialog` / `Popover` 面板 child 不越界刷出圆角外
- 后续 `ScrollView` 内容在 viewport 外不可见

## 当前结论

NandinaUI 当前最需要的，不是再给 `Card` 追加更多局部 API，而是把 overflow / clip 建成一套跨 runtime、widgets、render 的正式 contract。

更稳的方向应是：

- 在 runtime 固定最小 overflow 语义
- 用父容器声明的 child clip scope 表达 containment
- 用内部 slot widget 承担 section 型组件的多裁剪范围
- 当前先兼容 ThorVG 直绘路径
- 最终统一降到 `Scene / PushClip / PopClip / RenderBackend`

这条路线既能解决 `Card` 当前剩余问题，也不会把项目重新带回“在单个组件里继续补洞”的旧轨道。

## 相关文档

- [架构规划](architecture-plan.md)
- [布局策略](layout-strategy.md)
- [阶段路线图](roadmap.md)
- [Widget Primitives 设计文档](widget-primitives.md)
- [NandinaUI Runtime 模块设计文档](../runtime/docs/runtime-design.md)