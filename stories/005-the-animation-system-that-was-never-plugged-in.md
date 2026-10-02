# 005 · 造好了却没接出去的动画系统

> 2026 年 10 月，紧接 [004](004-the-key-that-was-not-an-identity.md)。
> 起因只是"想让侧边栏的按钮有点反馈"，结果把 `animation` 模块从头读了一遍。
> 状态：sidebar 的动效已落地并通过测试；第五节记录的缺口**一个都没修**，
> 但都留了证据和可能的出口 —— 动画系统优先级不高，这是有意为之，不是遗漏。

## 一、最初的诉求只有三句话

项目作者的原话大意：选中页面的按钮样式要发生变化，再加鼠标悬浮动画和点击动画。

前两件是确定的活。第三件让我先去摸 API，然后撞上第一堵墙：

```cpp
// nandina/widget/primitives/pressable.hpp
[[nodiscard]] auto hovered() const -> bool;   // 普通 getter，不是响应式来源
[[nodiscard]] auto pressed() const -> bool;
```

`Button` 有 `visual_state()`，有 state layer，有 ripple，但**没有一个"当前处于悬停/按下"的信号**。
所以任何"悬浮时做点什么"的写法都得先自己搭一座桥：

```cpp
.on_hover_changed([&hovered](const bool value) { hovered.set(value); })
.on_press([&pressed] { pressed.set(true); })
.on_release([&pressed] { pressed.set(false); })
```

回调 → 信号 → `computed` → `ui.bind` 到视觉属性。桥本身没问题（`Pressable` 是专门
提供这些回调的）。

**但"为什么没有现成的信号"，我后来才想明白。** 读
[组件公共契约](../docs/references/component_contract.md) 第 2 节才知道，`hovered` /
`pressed` / `focused` 被明确归为**瞬时状态 —— 由输入系统维护**，而"派生视觉状态由前三者
解析，不单独作为应用事实来源"。也就是说 `Pressable::hovered()` 是普通 getter **是有意
的**：把悬停暴露成 Signal，等于邀请应用把业务逻辑挂到 hover 上，与这条设计相悖。

所以真正的问题不在事件层，在**视觉层**：瞬时状态要变成视觉，走的是 state layer 那条
路径，而那条路径不可动画（见第五节第 2 条）。我想在视觉上给悬停加过渡，才被迫绕到应用
层、自己把瞬时状态重建了一遍。修法也应该落在视觉层，而不是把 hover 变成 Signal。

## 二、点击动画不用加，它本来就有

这一步是我先入为主了。我以为"点击动画"要从零做，读了 `design_system.cpp` 才发现
默认按钮配方里写着：

```cpp
.ripple = RippleStyle {
    .color = ThemeColor::with_alpha(on_accent_ref, ThemeScalar::literal(0.20F)),
    .duration = ThemeScalar::token(ScalarToken::motion_medium_duration),
},
```

`ripple_duration` 非 0，所以 `Button::on_input` 会在左键按下时记录涟漪原点，
`on_process` 按 `motion_medium_duration`（0.20s）推进，并且**尊重 reduced-motion**。
这件事框架早就做完了，我差点又实现一遍。

我只在按下时补了一个圆角"弹一下"作为叠加反馈，没有碰 ripple。

## 三、我以为做不到的那件事，和真正做不到的那件

**悬浮的底色变化做不到。** 这是我最初想做的效果（也是几乎所有 UI 框架里最标准的
hover 反馈），但：

```cpp
// nandina/widget/button.cpp:289-291
auto state_layer = style.container;
state_layer.fill = theme::button_state_layer_color(style, visual_state());
primitives::BoxPainter::paint_fill(ctx, world, state_layer, opacity);
```

state layer 是**每帧现算**的一个叠加色，不是动画属性。`StateLayerStyle`
（`theme/design_system.hpp:83`）里只有 `ThemeColor hover` / `pressed` 两个纯色。
所以悬停底色**永远不可能平滑过渡**，除非改 `Button` 本身。

我退而求其次，让圆角在悬浮/按下时外扩。选它还有两个理由：它对**所有**条目都可见
（选中项是 `filled`、边框透明，改边框粗细在它身上完全看不出来）；而且它是纯绘制属性
（`DirtyFlags::paint`），不会触发重排 —— 我的流程里没有 GUI，能不动布局就不动。

**真正做不到的是"条目依次入场"。** 我在读文档时看到 `Group` 有
`parallel` / `sequential` / **`stagger`**，第一反应就是"正好用来做侧边栏条目的错峰
淡入"。然后发现够不到 —— 这是第四节第 3 条。

## 四、我第一版选错了插值策略

第一版我装的是弹簧（stiffness 420 / damping 28），理由是"更有弹性、更高级"。
项目作者提醒我 `Button` 还有一个 `behavior` 成员函数，我才回头把两个策略算了一遍：

ζ = c / (2√(km)) = 28 / (2×√420) ≈ **0.683** → 过冲 = exp(−πζ/√(1−ζ²)) ≈ **5.3%**。

而圆角增量是 2px。**过冲量 = 0.1px。** 我为"回弹感"付出的东西，一点都看不见。

更关键的是：`MotionTokens`（`theme/tokens.hpp:49-51`）只有 short / medium / long
三个**时长**，弹簧的 stiffness / damping / mass **没有任何主题来源**。也就是说
我硬编码的不只是数值，而是**手感** —— 主题想统一调慢一点都不行。

换成：

```cpp
const float micro_motion = ui.theme_manager().design_system().tokens.motion.short_duration;
...
.behavior(widget::visual::container.radius, animation::motion::tween(micro_motion))
```

换来两个性质：时长跟着主题走；**主题把它调成 0 就等于关闭动效**
（`AnimatedProperty::set_target` 对 0 时长直接吸附，不占动画轨道）。

后一个性质我怕它只是"看起来成立"，补了一条测试钉住：0 时长下改信号，值就地到位、
`animation_host().active_count() == 0`。前面那条 behavior 路径的测试也换成了实际用的
策略，而不是我第一版的弹簧。

教训不是"弹簧不好"，是**我按"哪个看起来更高级"选，而不是按"哪个量级需要"选**。
弹簧适合需要连续性的大位移（拖拽、面板推出），不适合 2px。

## 五、这套系统比我想的完整得多，缺口也比我预期的集中

读完之后（`nandina/animation/` 1394 行 + 部件层接线），它是这个结构：

| 层 | 组件 | 职责 |
| --- | --- | --- |
| 值 | `AnimatedProperty<T>` | target/current + 一种插值策略 |
| 调度 | `AnimationHost`（每棵树一个） | 逐帧 tick、只在值真变时标脏、owner 弱引用+自动取消、reduced-motion、承载 `Group` |
| 组合 | `Group` | parallel / sequential / stagger |
| 糖 | `motion::tween()`、`motion::spring()` | 给 builder 的 `.behavior()/.spring()` 消费 |
| 部件门面 | `PropertyEndpoint<T>` | 记住策略；树内走 host、树外吸附 |
| 可寻址路径 | `visual::Path` | 只有 6 条（见下面第 5 条缺口） |

三种互斥的插值策略：

- `Behavior<T>` —— 定时长 + 缓动曲线，任意 copyable + equality_comparable 都可用；
- `SpringSpec` —— 阻尼弹簧，仅浮点；
- `Keyframes<T>` —— 关键帧，任意 copyable + equality_comparable（所以 `NanColor` 可用）。

三条当时没料到、但设计得很好的语义：

1. **策略 ≠ 值**。`set_behavior`/`set_spring` 在属性还没有值时**只记住**策略，`set()`
   时才装上。所以 `.behavior(...)` 不碰配方的值 —— 这是"实例装过渡、颜色交给主题"
   能成立的前提。
2. **树内走 host、树外吸附并 `finish()`**。组件在挂载前构建/绑定不会丢动画，也不会
   卡在中间值。
3. **帧序 `reactive → animation → layout`**（`scene/frame_scheduler.hpp`）。动画在布局
   前推进，带过渡的属性同一帧就能进布局/绘制，不会晚一帧。

下面的缺口有个共同形状：**能力都造好了，但停在部件层够不到的地方。**

### 1. 没有循环/重复播放

`nandina/animation/` 全目录 grep `loop|repeat` **为空**。`Behavior` 一次性，
`Keyframes` 播完即 `finished_`，`Group` 也一次性。

后果是"时间驱动"那一整类动效都写不出来：

- `Spinner` 只能 `on_process` 手写角度累加 + 每帧 `mark_dirty(paint)`
  （`widget/spinner.cpp:90-100`，注释里还留着"持续请求下一帧，动画才会继续"）。
  它绕开了 owner 生命周期管理、脏标记归约、reduced-motion 降级。
- `Skeleton`（`widget/skeleton.cpp`）**连 `on_process` 都没有** —— 骨架屏是个静态灰块。
  而 `docs/components/skeleton.md` 早就把它记成了"已知空白"（"动画（shimmer / 呼吸）
  是已知空白，当前实现为静态弱化块"）。**这条缺口让那份记录从"没来得及做"变成
  "做不出来"**：要补 shimmer，就得像 `Spinner` 一样再手写一套 `on_process` 循环。

出口：在 `Behavior` / `AnimatedProperty` 上加 repeat / alternate / 次数即可，API 面不大。

### 2. Button 的 state layer 不在动画系统内

见第三节。`button.cpp` 每帧现算叠加色，`StateLayerStyle` 是纯色。
**全库所有按钮的悬停底色都不可能平滑过渡。**

出口：给 `Button` 加一个 `AnimatedProperty<float>`（状态层强度 0→1，target 由
`visual_state()` 决定）+ 配方里的时长字段。一次改动，全库受益。

### 3. `Group` 是死代码 —— 而且正好卡住我想要的效果

`parallel` / `sequential` / `stagger` 全部实现、测试覆盖完整（弹性的 time-based 与
completion-based 两种 ready 谓词都测了），但是：

```
$ grep -rn "animation_host().run(" nandina/ | grep -v nandina/animation/
（无输出）

$ grep -rn "run(\*probe" tests/animation_tests.cpp
tests/animation_tests.cpp:765  :786  :830  :865  :892
```

**`AnimationHost::run(owner, Group)` 的调用点只在测试里。** 生产代码只有
`dialog.cpp:344`、`command_palette.cpp:1433`、`tabs.cpp:454/456` 三处直接调 `set_target`，
没有一处用 `Group`。

根因是部件层拿不到 `AnimatedProperty&`：`PropertyEndpoint::property_` 私有且没有
返回它的访问器，`visual::Path` 只提供"写值 / 装策略"。而 `Group::clip(...)` 的签名
就是 `(owner, AnimatedProperty<T>&, target, behavior, dirty)`。

所以**"侧边栏条目依次入场"目前在部件层写不出来**，只能组件自持 raw
`AnimatedProperty`（像 `Tabs` 那样）。顺带发现 `Tabs::sync_indicator`
（`tabs.cpp:428-462`）手写了一遍"树内走 host / 树外吸附"的分支，而这段逻辑
`PropertyEndpoint` 已经封装好了 —— 组件侧改用 `PropertyEndpoint<T>` 能删掉这个重复。

出口：给 `PropertyEndpoint` 暴露一个可动画句柄，或在 authoring 层加 `.group()/.stagger()`。

### 4. `Keyframes` 在部件层不可达

`AnimatedProperty::set_keyframes` 已实现、有测试，`Keyframes<T>` 支持任意
copyable+equality_comparable（`NanColor` 可用，所以颜色关键帧是通的）。但
`widget/visual_property.hpp` 只有 `Writable` / `Animatable` / `Springable` 三个 concept，
**没有 keyframes 对应物**，authoring 也没有 `.keyframes(path, {...})`。

和第 1 条叠加起来就是：脉冲、呼吸、闪烁这类效果，在部件层**同时**缺"循环"和"关键帧"
两种表达方式。

### 5. `visual::Path` 只覆盖 box + label 六条

opacity / position / size 都没有路径。`NanNode2D::set_local_opacity` 存在
（`scene/node2d.hpp:95`）却不可动画。

这条会直接挡住已经写进合约的未来方向：[Page / Router 合约](../docs/references/page_and_router.md)
§7 提到页面转场要由 Outlet "在有限时段内同时持有新旧根节点完成动画" —— 面板滑入、淡出，
需要的正是 position / opacity。

### 6. 没有"主题版本"的响应式来源

`ThemeManager` 有 `revision()` 和 `ThemeObserver`（推式接口），但**没有 Signal**。
后果很具体：任何写在 `visual::Path` 上的实例值，在切换明暗外观后**必然过期** ——
驱动它的 `computed` 依赖的是 `current_page` / `hovered`，主题变了它们不重算。

这是我唯一因为"缺东西"而**主动放弃一个方案**的地方。我当时想过：让 sidebar 自己拥有
`container.fill`，用 `theme::resolve_button(...)` 解析出 idle / hover / selected 三种
颜色，再做过渡 —— 这样悬停底色和选中态都能平滑。放弃的原因就是这条：它会在用户切到
暗色主题后留下一套亮色的按钮。

于是变成了现在的样子：**颜色全部交给配方，sidebar 只拥有 `radius`**（圆角令牌与明暗
外观无关，所以不会过期）。

出口：把 revision 暴露成 `Signal<std::uint64_t>`，或提供 `ui.observe_theme(...)`。
补上之后"自己拥有颜色 + 跟随主题"才成立，很多实例级定制会变得可行。

### 7. 弹簧参数没有主题预设

`MotionTokens` 只有 3 个时长。这不是错（弹簧本来就是手感参数），但意味着"统一动效
手感"只能靠约定、不能靠主题。

### 8. 小坑：`progress()` 对 spring 与 keyframes 只返回 0/1

所以"用动画进度驱动别的量"只对 tween 有意义。值得写进头文件，否则下一个人会以为
能拿 spring 的进度做联动。

### 顺带：`docs/references/` 没有 animation 专题

其他模块都有：`reactive_model.md`、`design_tokens.md`、`module_dependency.md`……
只有 `animation` 没有。它的知识目前只存在于 11 个头文件的注释与 `animation_tests.cpp`
里。这不是缺陷，但是成本：我得读完全部 1394 行才能回答"这个效果能不能做"，而答
案（能/不能）本来应该在目录里就能查到。

## 六、我当时不知道什么

- 不知道 state layer 是**每帧现算**的。第一版方案里我以为"装上 behavior 就能让悬停
  底色平滑"，因为配方字段本来就是经过 `PropertyEndpoint` 的 —— 但 state layer 不是。
- 不知道 `PropertyEndpoint` 会**永久遮罩**配方里的那个字段（`apply()` 只在有值时才
  覆盖，一旦有值就永远赢）。我一度真想用 `bind(container.fill, ...)` 做悬停底色，
  那会静默吃掉 `outlined` / `filled` 之间的配色差异。
- 不知道 `AnimationHost` 只在值真变时标脏。知道之后才敢让它每帧 tick 一堆轨道。
- 我以为 `Group` 会有人用。它是这套系统里设计最讲究的部分（stagger 的 ready 谓词、
  移动语义的原因都写在注释里），却是完全没被消费的。
- 我不知道 `Skeleton` 没有动画。发现"知道原因"之后，它反而成了第 1 条最好的证据。

## 七、留一份排序，不催着做

就算现在不修，也应该知道成本落在哪。我的判断是 **1 → 2 → 6**：

1. **循环/重复**：解锁一整类动效，API 面最小，能立刻回收 `Spinner` 的手写循环，
   顺带让 `Skeleton` 有可能动起来。
2. **state layer 可动画**：按钮手感上唯一还缺的拼图，一次改动全库受益。
3. **主题版本信号**：解锁"实例级样式定制"的前置，也是这轮唯一逼我改方案的约束。

第 3、4 条（Group / Keyframes 接线）价值同样高，但更适合等 1、2 定了之后一起做 ——
它们共用同一套"部件层句柄"的设计。

代码里已经留下了两处约束的注释（`showcase/components/sidebar.cpp` 的文件头和
`base_radius` 附近），免得下次有人重新推导一遍。

**这一轮的验证**：`buildDir` 与 `buildDir-no-rtti` 全量构建通过，单元测试 70/70；
`tests/animation_tests.cpp` 48 个 case / 246 条断言（新增 2 个 case，分别钉住 behavior
路径的完整过渡和 0 时长瞬时吸附）。
