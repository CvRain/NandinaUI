# 溢出与裁剪契约

一个容器把自己的子内容画到边界之外，是布局系统里最常见的意外。文本溢出、滚动内容越界、圆角面板被直角内容盖住，本质上都是同一个问题：父节点没有告诉渲染层「我的子树只能出现在哪里」。NandinaUI 把这件事拆成两个独立的角色——容器**声明**裁剪，render 层**执行**裁剪——并用一条贯穿绘制遍历的裁剪栈把两者接起来。本文记录这条链路上已经成立的约定，以及当前还没有的能力。

## 为什么需要契约

如果不把裁剪上升为跨层语义，每个需要 containment 的容器都会自己想办法：文本组件自己裁自己的字形，面板自己裁自己的子节点，滚动容器再发明一套。结果是同一种视觉约束散落在多个组件里，彼此不一致，也无法复用到下一个容器上。

因此当前的划分是：**是否裁剪、裁到哪里**属于 scene/widget 树的结构语义，由控件自己持有；**如何把裁剪落到后端调用**属于 render 层的职责。widget 作者只表达意图，不接触 scissor 或任何后端 API。

## 声明端：ControlOverflow

声明裁剪的入口只有一个枚举，定义在 `scene::NanControl` 上，与控件尺寸一起构成它的矩形语义：

```cpp
enum class ControlOverflow {
    visible,
    clip,
};
```

它由 `set_overflow(ControlOverflow)` 设置、`overflow()` 读取，默认值是 `ControlOverflow::visible`。默认不裁剪是刻意的选择：控件在 `NanNode` 之上引入 `size`，绝大多数容器只想限制自己的命中区域和背景，并不想切断子内容的绘制；让 `clip` 成为显式 opt-in，既保持了既有控件的行为，也让「谁在裁剪」在代码里一眼可见。

设置它会同时把 `paint` 与 `semantics` 标脏。绘制脏是显然的；语义脏则是因为裁剪同时改变了命中区域，这一点下面会展开。

需要裁剪的真实用例目前只有 `widget::ScrollView`：它在构造函数里就 `set_overflow(ControlOverflow::clip)`，把自己永久声明为一个裁剪视口，然后用子节点的负偏移来实现滚动。其他控件都保持默认的 `visible`。

## 执行端：ClipStack 与 DrawContext

render 侧的执行者是 `render::ClipStack`，它由 `render::DrawContext` 以成员形式持有，随绘制遍历一路向下传递。`DrawContext` 同时携带当前世界变换、继承 opacity 和设备引用，裁剪栈只是其中之一——它们是同一份「向下流动的绘制状态」。

`ClipStack` 的语义很窄，但正是这条契约需要的全部：

- 栈里存的是一组**屏幕空间**的轴对齐矩形；
- `push(rect)` 先把新矩形与当前栈顶求交，再入栈，因此任何子裁剪都不可能超出祖先裁剪；
- 栈顶每次变化都会立刻调用 `device.set_clip(top)`，栈清空时调用 `device.clear_clip()`；
- 求交结果为空时交出的矩形不是有效矩形，后端据此进入零面积裁剪，整棵子树不产生可见像素。

值得注意的是求交发生在**入栈时**而不是绘制时：后端只需要无脑应用栈顶，不必自己理解嵌套。当前的 raylib 后端正是这样做的——scissor 本身不能嵌套，但 `ClipStack` 提交的永远是已经求交过的有效边界。

## push/pop 的作用域

与裁剪栈交互的唯一正规方式是 `render::ClipStack::Guard`。`push()` 返回一个 Guard，Guard 析构时自动 `pop()`；它不可拷贝、可移动，移动会转移 pop 责任。这消除了「忘记 pop 导致后续所有绘制被误裁」这类最容易出现的泄漏。

遍历侧的挂钩是虚函数 `NanNode::_push_child_clip(render::DrawContext&)`。基类实现返回一个不生效的 Guard（`{nullptr, false}`），所以普通节点完全不参与裁剪；`NanControl` 覆写它：

```cpp
if (overflow_ != ControlOverflow::clip) {
    return {nullptr, false};
}
return ctx.clip().push(
    render::world_bounds_from_local(ctx.world_transform(), local_rect())
);
```

`NanNode::_propagate_draw` 的调用顺序是：先更新世界变换与 opacity、执行本节点 `on_draw`、**然后**才调用 `_push_child_clip` 并递归子节点，最后由 Guard 析构与 `_pop_draw_transform` 收尾。由此得到一个对使用者很重要的推论：**裁剪作用域只覆盖子节点，不覆盖容器自身的绘制**。一个开启 `clip` 的容器仍然会完整画出自己的背景，`clip` 限制的是它的后代。

裁剪边界本身取自 `local_rect()`，也就是控件以自身原点为左上角的 `[0,0,w,h]`。当前没有 content-box 与 border-box 的区分，也没有 inset 后的内边距矩形——裁剪区就是控件声明的矩形。

## 裁剪与命中测试的一致性

`visible` 与 `clip` 的差别不只在绘制上。`scene::NanSceneTree::hit_test` 在向下递归时，对每个节点做同一件事：

```cpp
if (const auto* control = node->as_control();
    control != nullptr && control->overflow() == ControlOverflow::clip)
{
    const auto clip_bounds = control->global_bounds();
    visit_children = clip_bounds.is_valid() && clip_bounds.contains_point(world_point);
}
```

也就是说，`clip` 容器边界之外的点不会被继续交给它的子节点，子节点即使几何上落在那里也无法被命中。这正是把 `set_overflow` 标为语义脏的原因：绘制可见性与交互可达性由同一个开关一起翻转。

需要注意两点精确行为。第一，命中测试使用的是 `global_bounds()`——即局部矩形四角变换后的世界空间 AABB；控件发生旋转时它是包围盒，会比真实裁剪区域（屏幕空间轴对齐矩形）更宽松。第二，截断只影响**子节点**的访问，容器自己是否被命中仍由后续的 `contains_point` 决定，与绘制时「自己的 `on_draw` 不受自己裁剪约束」是对称的。

绘制与命中因此共享同一个判定来源：都以 `overflow() == clip` 加控件矩形为准。这不是两套逻辑的巧合对齐，而是刻意复用同一份结构语义的结果。

## Screen-space 层与浮层

`scene::CanvasLayer` 是唯一改写绘制变换起点的节点。`_push_draw_transform` 不继承父节点的世界变换，而是直接从根变换重新起算：

```cpp
auto saved = ctx.world_;
ctx.world_ = ctx.root_transform_.compose(transform());
```

`_propagate_draw` 使用 `NanSceneTree::draw` 构造的默认 `DrawContext`，其 `root_transform_` 是单位变换。因此 screen-space 层内节点的裁剪矩形天然落在屏幕坐标系里，不受任何祖先变换影响；`CanvasSpace::world` 层则保持正常的父子复合。

浮层之所以能脱离内容层裁剪，靠的不是某个「忽略裁剪」的开关，而是**不进入那条裁剪作用域**。`scene::OverlayHost` 是 `LayerStack` 的子类，持有两个 screen-space `CanvasLayer`（content 层 order 0、overlay 层 order 1000），各自拥有一个 viewport 尺寸的 layout root。`present()` 把浮层控件挂到 overlay surface 之下，于是它以 overlay 层为祖先，而不是以触发它的那个滚动或面板控件为祖先——内容层的 ClipStack 在遍历到该子树时早已随 Guard 析构清空。这也解释了为什么浮层不需要理解 `ControlOverflow`：它只是换了一个没有裁剪祖先的位置。

## 当前边界

以下都是代码里能确认的现状，不是待办清单的推测：

- **只支持轴对齐矩形裁剪**：`ClipStack` 的元素是 `NanRect`，后端对应 scissor，没有圆角、路径或 alpha mask 裁剪。开启 `clip` 的圆角容器裁出来仍是直角矩形。
- **只有 `visible` / `clip` 两种策略**：没有 `hidden`、`auto`，也没有基于内容的自动滚动。
- **没有独立的裁剪区 API**：裁剪区恒为 `local_rect()`，容器无法声明「裁到内容区而不是整个矩形」。
- **滚动建立在 `clip` 之上**：`ScrollView` 用 `set_overflow(clip)` 实现视口，用子节点位置偏移实现滚动，`on_measure` 以宽松约束测量内容高度并记录 `content_size_`，`clamp_offset()` 把偏移限制在 `[0, content - viewport]`。裁剪语义没有被重新发明。
- **裁剪不改变布局**：它只影响绘制与命中的可见范围，溢出内容依旧参与测量与占位。

## 相关文档

- [浮层架构](overlay_architecture.md)
- [组件公共契约](component_contract.md)
- [开发参考目录](README.md)
