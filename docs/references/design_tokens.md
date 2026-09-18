# 设计令牌与主题系统

组件一旦把颜色、圆角和字号写死在自己的绘制代码里，亮暗切换、品牌换肤和「只改一个按钮」就会变成散落各处的修改点。NandinaUI 的做法是把「值从哪来」和「组件画成什么样」分开：令牌提供可复用的原子值，语义调色板把原子值翻译成角色，组件配方再把角色和交互状态组装成一个可直接绘制的解析结果。本文说明这条链路在 C++26 主线中的真实形态，以及控件作者应该遵守的约束。

## 三层令牌模型

当前代码的分层比「primitive → semantic → component」这个抽象说法多出一层，因为它把**参考色阶**与**语义色板**分开了；同时语义色板有**两条并列的作者路径**：

```text
NanTokens（尺度）
        │
        ├── 参考色阶路径（品牌族）───────────────────────────────┐
        │   NanReferencePalette（7 组 11 档色阶）                 │
        │   make_color_scheme(reference, appearance, policy)     │
        │                                                        ▼
        └── 直接声明路径（默认主题 / 逐角色手写）──────► NanColorScheme（语义角色，light / dark 各一份）
            SemanticColorSpec                                        │  组装为共享视觉片段：BoxStyle / TypeStyle /
                                                                     │  FocusRingStyle / TrackStyle / ThumbStyle /
                                                                     ▼  ControlMetrics / ShadowStyle
                                                          ComponentRecipes（每组 = base + 有序 rules）
                                                                     │  resolve_*(system, appearance, 语义变体…)
                                                                     ▼
                                                          Resolved*Style（全是 NanColor / float 的具体值）→ painter
```

- **Primitive tokens** 是 `NanTokens`：`spacing`（`xs/sm/md/lg/xl`）、`radius`（`sm/md/lg/full`）、`border`（`thin/medium/focus_ring`）、`opacity`（`disabled/hover_overlay/pressed_overlay`）、`typography`（`label_sm/md/lg`）、`motion`（`short/medium/long_duration`）。它们只提供尺度，不决定组件行为。圆角尺度对齐 shadcn 的 `--radius: 0.625rem` 派生关系（`sm = 6`、`md = 8`、`lg = 10`），改主题时三者要一起调。
- **Reference palette** 是品牌方的着色输入：`NanReferencePalette` 持有 `primary/secondary/tertiary/neutral/success/warning/error` 七条 `NanColorScale`，每条 11 档（`ColorShade::shade_50`…`shade_950`）。主题作者通常用命名的 `NanHexScale` 书写，再由 `nan_color_scale()` 编译为内部 OKLCH 色阶。**注意 `error` 是唯一一条降序书写的色阶**（`shade_50` 最深），语义映射已按此适配，不要"顺手"改成升序。
- **Semantic palette** 是组件唯一允许引用的颜色层：`NanColorScheme`。核心角色沿用 shadcn 的 `x` / `x_foreground` 命名约定，另有若干扩展角色，见下节。
- **Component recipes** 是唯一事实来源：`ComponentRecipes` 为每个组件保存 `{ base, rules }`，`base` 完全指定所有片段，`rules` 是按 `tone/treatment/size/state` 等选择器匹配的增量覆盖。

### 语义色角色

`NanColorScheme` 的角色分三组。前两组是新代码应该使用的规范名；第三组是兼容别名，与规范名**同值**，会在构造时一次性同步（直接改写别名不会回写规范字段，因此新代码不要写别名）。

| 组 | 角色 |
| --- | --- |
| shadcn 对齐核心 | `background` / `foreground`、`card` / `card_foreground`、`popover` / `popover_foreground`、`primary` / `primary_foreground`、`secondary` / `secondary_foreground`、`muted` / `muted_foreground`、`accent` / `accent_foreground`、`destructive` / `destructive_foreground`、`border`、`input`、`ring` |
| 扩展 | `surface` / `surface_foreground`、`surface_variant` / `surface_variant_foreground`、`tertiary` / `tertiary_foreground`、`success` / `success_foreground`、`warning` / `warning_foreground`、`error` / `error_foreground`、`info` / `info_foreground`、`selection` |
| 兼容别名 | `on_background`、`on_primary`、`on_secondary`、`on_tertiary`、`on_surface`、`on_surface_variant`、`on_muted`、`outline`、`outline_variant`、`on_success`、`on_warning`、`on_error`、`on_info`、`focus_ring` |

三个最容易用错的角色：

- `accent` 是 **hover / 选中底色**，不是品牌强调色。品牌强调色是 `primary`。
  注意与 `ThemeColor::accent()` 区分：后者是「随 tone 解析的强调色对」（`ToneAccentRef`），与 `ColorToken::accent` 这个调色板角色不是一回事。
- `secondary` 是**中性**次操作色，不是品牌辅色。品牌辅色是 `tertiary`。参考色阶路径下 `secondary` 取 `PaletteVariantPolicy::light_secondary` / `dark_secondary` 指定的中性档。
- `muted` 是弱化**底**，弱化**字**用 `muted_foreground`。次级说明文字优先 `on_muted` 或 `on_surface_variant`，不要用 `outline`。

### 两条作者路径

1. **参考色阶路径**：`make_color_scheme(reference, appearance, policy)`，适合「一族色阶派生明暗两套」的品牌主题（`butter` / `fluent` / `material` 走这条）。色阶里承载了角色分配，因此它保持既有观感：`border`/`input` 取中性 500/600 档，`muted`/`accent`/`secondary` 取 200/800 档，`ring`/`focus_ring` 与 `primary` 同色。状态色（`success`/`warning`/`error`）上的前景会在中性色阶里挑对比度最高的一档——这三条色阶的明度跨度不足以固定档位达标。
2. **直接声明路径**：`SemanticColorSpec` + `NanColorScheme{spec}`，逐角色手写语义值，不需要先编出 7×11 色阶。**框架默认主题走这条**（`default_light_palette()` / `default_dark_palette()`）。

### 框架默认主题

默认快照（`default_design_system()`，也就是 `ThemeManager` 构造时应用的快照）对齐 **shadcn/ui 的经典中性默认**（neutral 基色 + 近黑 primary，即 `oklch(0.205 0 0)` / 暗色 `oklch(0.922 0 0)`）：

> 默认 primary **不带任何色相**（`chroma == 0`）。这是刻意的：框架不替用户决定品牌色，同时主题作者改动任何一处带色相的值时，在界面上都非常显眼，便于对照"哪里被改了"。要品牌色请走主题族或 `SemanticColorSpec`。

- 亮色：`background` 纯白、`card` 低 2.5% 明度、`popover` 纯白，均以 `border` 收边；`border` / `input` 为浅灰。
- 暗色：`background` 深灰、`card` / `popover` 提亮一档；`border` / `input` 用**半透明白**（10% / 15%），在任何堆叠层级上都保持相对亮度，不会在更亮的卡片上"消失"。
- 暗色品牌色提亮、状态色上的文字按底色明度选中性极端档；`ring` 是中性档而不是品牌色——焦点环需要 ≥ 3:1 才能看见，而边框本就该低调（对底色 ~1.2:1 正常），两者定位不同。

默认主题的对比度与层级门槛由 `tests/theme_palette_contrast_tests.cpp` 固化：正文与各表面文字 ≥ 4.5:1、焦点环与语义信号色 ≥ 3:1、卡片/浮层与页面至少差 2% 明度。改默认色值前先跑它。

## 令牌的引用形式

配方字段不是裸值，而是「token 或字面量」的声明：

- 标量用 `ThemeScalar`（即 `ThemeValue<float, ScalarToken>`）：`ThemeScalar::token(ScalarToken::spacing_md)` 或 `ThemeScalar::literal(40.0F)`。`ScalarToken` 枚举名与 `NanTokens` 的字段一一对应（`spacing_*`、`radius_*`、`border_*`、`opacity_*`、`typography_label_*`、`motion_*_duration`）。
- 颜色用 `ThemeColor`：`ThemeColor::token(ColorToken::primary)`、`ThemeColor::literal(nan_color(...))`。`ColorToken` 与 `NanColorScheme` 的语义字段一一对应（含上面的兼容别名）。
- 颜色还支持两类表达式。其一是随当前 tone 解析的引用：`ThemeColor::accent()` / `ThemeColor::on_accent()`，让 `filled` 之类的规则能表达「用当前强调色及其反色」。其二是浅层变换：`ThemeColor::with_alpha(source, factor)`、`ThemeColor::transparent(source)`、`ThemeColor::mix(lhs, rhs, factor)`。变换的操作数不嵌套表达式，保持可静态推导。
- 解析入口是 `resolve_color(system, appearance, value, tone)` 与 `resolve_scalar(system, appearance, value)`；片段级还有 `resolve()` 重载。它们在内部构造 `NanTheme {tokens, palette(appearance)}` 后交给 `resolve_theme_color()` / `resolve_theme_scalar()`。

这里也解释了 `DesignSystem` 与 `NanTheme` 的关系：`DesignSystem` 是**与外观无关**的不可变快照（同时内嵌 `light` 与 `dark` 两套 `NanColorScheme`），而 `NanTheme` 只是 `{ tokens, 单一 palette }` 的值对象，是某个外观下的解析视图。`ThemeManager::theme()` 返回的正是后者，供仍按旧接口读取令牌的代码使用。

## 外观选择与 ThemeManager 下发

`ColorAppearance { light, dark }` 是解析后的外观，`ThemePreference { system, light, dark }` 是用户偏好。`ThemeManager::appearance()` 把二者统一：显式 light/dark 直接返回，`system` 则回退到宿主机通过 `set_system_preferences()` 提供的 `SystemPreferences::appearance`；ThemeManager 自身不观察操作系统，缺省为 light。`DesignSystem::palette(appearance)` 负责在快照内选调色板。

主题变更只有一条收敛路径：`apply(shared_ptr<const DesignSystem>)` 原子替换快照指针并发布一次 revision；`commit()` 之后重建有效快照（合并遗留 `NanStyle` 规则）、刷新 `theme()` 视图，再 `publish_revision()`。相同指针的重复提交不会产生额外 revision，`set_preference()` 也只在有效外观真正改变时才下发。命名的 `register_theme` / `register_family` / `activate` / `register_theme_family` 都是这套原子路径之上的选择器。

下发采用观察者 + 虚函数广播，而不是响应式 Signal：`NanSceneTree` 私有实现 `theme::ThemeObserver`，在 `on_theme_revision_changed()` 后沿场景树调用每个节点的 `on_theme_changed(manager)`；节点在 `enter_tree` 时也会收到一次，离开场景树时收到 `on_theme_context_removed()`。控件在 `on_theme_changed()` 里重新解析自己的配方即可，不需要手动比较修订号。

与配方解析并行的是 `StyleContext`：它承载可继承的文本向值（`font`、`font_size`、`text_color`、`locale`、`direction`），每个字段是带 `unset/inherit/initial/explicit_value` 四态的 `StyleValue<T>`，自顶向下解析为 `ResolvedStyleContext` 并通过 `on_style_context_changed()` 通知。它解决「继承」问题，不替代令牌解析。

## 组件如何声明配方与解析

一个组件的样式接入需要以下几处协作：

1. 在 `design_system.hpp` 定义 `XRecipe`（片段组合）、`XRecipeRule`（**全部字段为 `std::optional`** 的增量覆盖 + 选择器，如 `ButtonRuleSelector`、`std::optional<bool> checked`、`std::optional<XVisualState> state`）、`XRecipes { base; rules; }`，并把 `XRecipes` 加进 `ComponentRecipes`。
2. 在 `design_system.cpp` 提供 `default_x_recipe()` 与 `resolve_x(...)`：先解析 `system.components.x.base`，再按顺序应用所有匹配规则（后匹配者胜），最后应用跨切面变换（例如 `disabled` 时按 `opacity_disabled` 统一缩放 alpha、隐去焦点环）。
3. 为规则提供 `apply_rule(system, appearance, ResolvedXStyle&, const XRecipeRule&)` 重载，保证解析器与实例覆盖走同一条字段映射路径。
4. 控件保存 `shared_ptr<const DesignSystem>` 与 `ColorAppearance`，实现 `resolved_style()`、`on_theme_changed()`，并按需实现 `on_style_context_changed()`。

## 解析优先级

[组件公共契约](component_contract.md) 第 7 节给出了推荐的优先级顺序；代码中的实现与它一致：

```text
DesignSystem 默认值（XRecipe.base）
→ 语义变体与尺寸（tone / treatment / size / state 规则）
→ 跨切面状态变换（disabled 透明度）
→ selector rule / typed recipe override（set_override，实例级）
→ 实例 visual property 与 animation / binding
```

需要展开的有两点。第一，`set_override(XRecipeRule)` 是**类型化字段**覆盖：控件把它保存为 `std::optional<XRecipeRule>`，每次解析时在 DesignSystem 规则**之后**重新应用，因此它既压过配方规则，又不会冻结主题——切换设计系统后仍随新快照重解析。第二，`set_theme(NanTheme)` 是兼容用的整份覆盖，会把控件钉死在该主题上（不再跟随系统切换）；跨实例的统一调整应改 DesignSystem 或配方规则，而不是逐个控件刷 `set_theme()`。遗留的 `ThemeManager::set_style(NanStyle)` 规则目前只覆盖 Button 与 TextField，会在提交时转换为配方规则并入有效快照，属于迁移期糖。

## 控件作者应遵守的规则

- 优先暴露**有限的语义变体**：`ButtonTone`、`ButtonTreatment`、`ButtonSize`（以及各组件自己的 `XVisualState`）。不要为每个颜色、圆角和状态层新增平铺 setter；那会把主题重新变回硬编码表。
- 需要表达差异时先自问它属于哪个维度——是色族（tone）、外观处理（treatment）、尺寸（size）还是交互状态（state）——不要把多个维度折叠进一个大枚举。
- 新值优先引用现有 token；确需具体值时用 `ThemeScalar::literal` / `ThemeColor::literal` 显式标注为字面量，而不是散落的魔法数字。文字优先引用排版 role 或 `typography_label_*`，而不是裸字号。
- 颜色一律走语义角色，不直接读 `NanReferencePalette`。新增色值时**优先用规范名**（`foreground`、`muted`、`border`…），不要用第三组兼容别名。
- 新增配方字段时同步补 `*RecipeRule` 与 `resolve_*`，让主题作者和实例覆盖都能触达它。
- 改默认色值或新增语义角色时，同步跑 `tests/theme_palette_contrast_tests.cpp`；对比度与层级是硬门槛，不是观感偏好。

项目还内置了一份 UI/UX 设计系统 skill（`.dsh/skills/ui-ux-design-system/`），包含三层令牌模型、交互状态覆盖清单与视觉审查流程的上游参考资料。

## 主题族群与样式文档

品牌主题以 `ThemeFamilyDefinition { name, reference, policy, tokens, card_rules }` 声明，由 `build_family_design_system()` 编译为内嵌 light/dark 的完整快照，再经 `register_theme_family()` 注册。框架内置 `butter`、`fluent`、`material` 三族；`activate_family()` 对全快照族执行整体原子 apply，此后外观翻转只刷新 `theme()` 视图，不再更换 DesignSystem。`register_default_theme_families()` 是幂等批量注册入口，但**框架不会自动激活任何族**——不调用时应用看到的是上面的默认主题。

三族的成熟度不同：`butter` 是本项目自研风格（暖奶油中性 + 琥珀品牌 + 软阴影卡片，已打磨过品牌档对比度），`fluent` 与 `material` 目前主要是「配色 + 圆角尺度」的迁移，缺少各自设计语言在几何、密度、状态层上的差异，可作为后续打磨对象。

面向主题作者的外部入口是 TOML 样式文档：`StyleDocument` 可声明 `reference_palettes`、`themes`、`theme_families`、`active_theme`、`active_family`、`preference`、遗留 `NanStyle` 规则与字体族，`parse_style_document()` / `load_style_document()` 解析，`apply(ThemeManager&, FontFamilyRegistry*)` 以 `std::expected<void, std::string>` 报告失败。

## 当前已知空白

以下现象都可在当前代码中直接确认，属于尚未收口的部分：

- `TypographyRoles`（`label_sm/md/lg`）已在默认快照中填充，但组件配方仍直接引用 `ScalarToken::typography_label_*`，尚未真正消费该角色层。
- `tone` / `treatment` 这类平铺语义变体目前只有 Button 具备；其余控件以 `state`（Checkbox、Switch、RadioButton 另有 `checked`）为主。
- 遗留 `NanStyle` 只支持 Button 与 TextField 两类规则，因此 `set_style()` 不是通用扩展点。
- 主题通知是虚函数广播（`on_theme_changed` / `on_style_context_changed`），没有响应式主题绑定；部分控件（例如 `Label`）尚未暴露 `set_override`，只有 `on_theme_changed`。
- `motion` token 与 `reduced_motion` 偏好已存在，但动画侧尚未接入：浮层（Dialog / Tooltip / Select 弹层）目前没有打开/关闭过渡，缓动曲线也未与 token 对齐。这是明确的后续项，参考素材已归档在 skill 的上游仓库（transitions.dev）。
- `chart-*`、`sidebar-*` 这类 shadcn 的角色**有意未引入**：框架还没有图表与侧边栏组件，等组件出现时再按同一命名约定补。
- 参考色阶路径的 `error` 色阶明度跨度不足，状态色上的文字只能达到约 4.3:1（测试按大字级 3:1 放行）；直接声明路径没有这个限制。若要让三族也达到 4.5:1，需要重写这些状态色阶。

## 校验台（playground）

`playground/` 是本地组件与主题校验台（该目录被 `.gitignore` 忽略，不入库）：

```bash
meson compile -C buildDir
./buildDir/playground/playground
./buildDir/playground/playground --family=butter --appearance=dark
./buildDir/playground/playground --self-test     # 自检：自动轮换主题，见下
```

- 工具栏可在运行时切换**主题族**（默认主题 / butter）与**外观**（亮色 / 暗色）。切换不重建 UI 树：控件在 `on_theme_changed()` 里重新解析 `manager.design_system_shared()`，`SceneTree` 把 revision 广播给已挂载节点。
- 默认主题也注册成一个族（`"default"`），因此两个主题走同一个 `activate_family()` 入口。
- `--family=<name>` 与 `--appearance=<light|dark|system>` 用于以指定状态启动，便于逐状态截图比对。
- `--self-test` 在真实窗口里每 0.35s 轮换一次族/外观（3 轮共 12 次），覆盖工具栏点击走的那条代码路径，跑完用 `request_close()` 干净退出（退出码 0）。对应的回归测试是 `tests/theme_switch_tests.cpp` 与 `tests/window_lifecycle_tests.cpp`。

### 回调捕获 BuildContext 会悬垂（踩过两次）

工具栏的切换回调**不能**捕获 `&ui`：root view 工厂是 `[](const widget::BuildContext& ui)`，`root_view.hpp` 会把实参**复制**进形参，因此这个形参在 build 返回后就销毁了；点击时再访问就是悬垂引用，直接 SIGSEGV。正确做法是捕获 `ui.theme_manager()` 返回的 `ThemeManager&`（由 `NanApplication` 持有，生命周期覆盖整棵 UI 树），或者让辅助函数**按值**持有 `BuildContext`。

**第二次踩坑**在拖拽演示：`GestureArea::set_on_drag_start/move/end` 是**裸 setter**，
不像 `NodeBuilder::on_click` 那样被 callback-lifetime 守卫；拖拽回调会活到交互发生之后，
而 build 早已返回，于是 `[&ui]` 在拖动时变成悬垂访问并 SIGSEGV。两条修法：

- 捕获**服务指针自身**：`auto* drags = ui.drag_controller();` 然后用 `[drags, chip]`；
- 或走 builder 的 `on_drag_*` 钩子（`ui.make<widget::GestureArea>().on_drag_start(...)`），
  它内部会套上 `guarded()`。

另外**不要**手动 `set_pointer_capture(gesture.get())`：`PointerArea` 在 press 时已经把
捕获目标设成 child（即条目本身）。手动覆盖并非崩溃原因，但会让捕获语义与你以为的不一致。

`tests/theme_switch_tests.cpp` 固定了「挂满控件的场景树 + 反复切换族/外观 + 每轮重解析并绘制」这条路径；`tests/drag_gesture_tests.cpp` 用确定性几何走完
press → move → move → release 并断言换父，用于捕获同类回归。

### 节点换父：`NanNode::reparent()`

`add_child` / `insert_child` 只接受**已脱离**的节点，碰上一个还挂在别处的节点就报错。把控件从一个容器搬到另一个容器（从 ListView 拖进 Grid、把卡片挪进面板）是正常需求，为此提供：

```cpp
target->reparent(node);        // 追加到末尾
target->reparent(node, 0);     // 插到最前
```

语义：

- 先对旧父节点做一次完整 detach（子树 `on_exit_tree`、旧父节点标记布局/语义失效），再对新父节点做完整 attach（`on_enter_tree` / `on_ready`）。节点自身的内部状态（文本、数值、滚动位置、绑定）不受影响。
- 同一个父节点下调用等价于重排（走 `move_child`），**不**触发 exit/enter/ready。
- 拒绝三类非法输入：`nullptr`、把自己挂到自己下面、把祖先挂到后代下面（会成环）。失败时原结构不变。
- 在树遍历期间（process / layout / paint）调用会被**延迟**到本帧安全提交点，此时 `is_reparent_deferred()` 为 true；需要同步语义时请先确认它。

`insert_child` / `add_child` 的错误消息现在会带上「子节点名 @ 地址」与「当前父节点名 @ 地址」，并直接提示改用 `reparent()`。回归测试：`tests/node_reparent_tests.cpp`。

### 拖动换父：`widget::DragController`

`NanNode::reparent()` 解决"能不能搬"，`DragController` 解决"怎么搬" —— 它把
「命中测试 → 落点解析 → 提交换父」收敛成一个**窗口级服务**，挂在场景树上，页面与控件
都能拿到：

```cpp
// 从 BuildContext（页面 / root view 内）
auto* drags = ui.drag_controller();   // 未接入窗口时为 nullptr，先判空
```

生命周期（典型接法：在 `GestureArea` 的回调里）：

```cpp
on_drag_start : drags->start(node, ghost /*可空*/, grab_offset);
on_drag_move  : drags->update(pointer_position);   // 更新幽灵 + 重新解析落点
on_drag_end   : drags->commit();                   // 换父；或 cancel() 放弃
```

要点：

- `update()` 收的是**指针位置**（全局坐标），命中测试用它，幽灵则按 `grab_offset` 对齐 —— 用户看的是指针在哪，不是幽灵的左上角。
- 落点解析从 `hit_test(pointer)` 沿祖先链向上，取第一个"接受放置"的节点；**被拖节点自身与其后代永远不是落点**（那会成环）。
- 容器可以覆写 `NanNode::accepts_drop()`（默认 true）来拒绝放置，也可以覆写 `drop_slot_at(pointer)` 给出插入位置（例如网格落位）。默认插入位置按几何就近计算：主轴方向找第一个"指针位于其中线之前"的兄弟，插到它前面。
- `DragController::install()` 可以传自定义谓词覆盖 `accepts_drop()`。`install` 的第二个参数是承载拖动幽灵的 `OverlayHost`（可为 nullptr，则无幽灵），幽灵以 `OverlayLevel::nested_popup` 呈现且不阻断下层输入。
- 提交走 `reparent()`，所以树遍历期间会自动延后到本帧安全点（见上文 `is_reparent_deferred()`）。

回归测试：`tests/drag_controller_tests.cpp`（落点解析、拒绝自身/后代、插入位置、提交、取消、谓词收紧）。playground 里有一个可直接拖的演示（两个容器之间拖条目）。

### 单子节点容器要用 `set_child`，不是 `add_child`

`PointerArea` / `GestureArea` 是**单子节点**容器：内部只认 `child_`。用 `add_child` 挂进去
的节点会被 `scene` 接受（成为子节点、参与绘制），但 `PointerArea::on_layout()` 只看
`child_`，于是那个节点永远不被布局，而交互区域自身量成 0 尺寸 —— 一列这样的区域会全部
叠在同一位置。

```cpp
auto gesture = widget::GestureArea::create();
gesture->set_child(chip);   // 正确
// gesture->add_child(chip);  // 错：chip 不参与布局，条目标目全叠在一起
```

这条陷阱由 `tests/pointer_area_layout_tests.cpp` 固定：既验证 `set_child` 的布局契约，
也锁定「`add_child` 进去的节点不参与布局」这一事实，避免再次踩到。

### 拖拽要移动"整个条目"，不是条目内部的控件

校验台最初把 `Chip` 自己交给 `DragController::start()`，而 `Chip` 外面还套着一层
`GestureArea`。后果是截图里能看到的三件事：

| 现象 | 原因 |
| --- | --- |
| 拖走后原位置留下**空槽** | 空的 `GestureArea` 包装器仍留在原容器里占一个子节点 |
| 拖入后组件**互相覆盖** | `Chip` 被塞进目标容器已有包装器之间，槽位错位 |
| 再拖回来**时灵时不灵** | 命中的是目标容器里的包装器，拖动对象与命中对象不是同一个 |

正确做法是拖**条目本身**（`GestureArea`）：

```cpp
auto item = ui.make<widget::GestureArea>().child(chip);
item.expose(item_handle);                       // 拿到 shared_ptr 供回调捕获
item.on_drag_start([drags, item_handle](const auto& e) {
    drags->start(item_handle, /*ghost=*/nullptr, grab_offset);
});
```

这样源容器少一个子节点、目标容器多一个，`Column` 会自动重排 —— **不需要额外的槽位
管理**：容器按子节点顺序布局，条目走了槽位自然收起。回归测试：
`tests/drag_roundtrip_tests.cpp`（前进、再拖回、连续往返，断言容器子节点数守恒且每个
条目仍只占一个槽位）。

### 拖拽无效的两个真实原因（本轮修复）

「拖了没反应」不是交互没触发，而是链路上有两处断点：

1. **窗口级服务没接到页面上。** `NanRouter` 里有两处构造 `PageContext`：`make_context_for()`
   与 `push_page()` 的内联构造。给前者加了拖拽服务后，页面 push 走的是**后者**，因此
   `ui.drag_controller()` 恒为 `nullptr` —— 拖拽回调里判空后静默 return，界面毫无反应。
   现在两处都传（Router 直接持有 `DragController*`，由 `NanWindow` 注入）。
   教训：加窗口级服务时要同时检查 `PageContext` 的**全部**构造点。
2. **延迟 mutation 在 flush 里二次入队后丢失。** `reparent()` 在 `process` 阶段会入队；
   而 `flush_tree_mutations()` 本身仍在 `tree_commit` 阶段执行（同样算"延迟中"），
   于是 `reparent` 再次入队，而队列已被取空 —— 没人再处理，操作静默丢弃。
   现在 `flush_tree_mutations()` 在**关闭延迟标志**的作用域里执行待办，并在结束时
   补齐残余入队项，保证一次 flush 收敛。

顺带把 `NanNode::accepts_drop()` 的默认值从 `true` 改成 **`false`**：从命中节点沿祖先链
找落点时，默认 true 会让第一个祖先容器（往往是不相关的行/页面）成为落点。容器现在必须
显式 `set_accepts_drop(true)`；`PointerArea` / `GestureArea` 这类单子节点交互包装器显式
拒绝放置（落点应解析到它们外面的真实容器）。

### 已修复的引擎缺陷（本轮）

搭建校验台时暴露并已修复的问题，都在测试里固定：

1. **重复挂载的报错没有线索**（已修）。`insert_child` 抛错时现在会给出两侧节点名与地址，并提示 `reparent()`。同时**不要**把 `NodeBuilder` 按值在函数间传递：它未删除拷贝构造，内部 `shared_ptr` 可能在拷贝路径上被两个父节点同时引用 —— `cell()` 这类辅助函数应接**已构建的 `shared_ptr`**。
2. **在 `on_frame()` 里调用 `close()` 会解引用空指针**（已修）。`close()` 会立即销毁 render device 与原生窗口，而 `tick()` 在 `on_frame()` 返回后仍要 `device_->clear(...)` / 绘制。现在新增 `request_close()`：只置标志，`tick()` 在**整帧（含绘制与帧末提交）结束后**才真正关窗；`should_close()` 与 `close_requested()` 会立即反映该请求。回归测试：`tests/window_lifecycle_tests.cpp`。
3. **窗口关闭时控件文本资源晚于 render device 析构**（已修）。应用内容由 `overlay_host_` 的内容层持有，而 `overlay_host_` 是窗口成员；`close()` 只清了场景树，内容树便活到窗口析构 —— 那时 device 已销毁，控件里的 `FontPipeline → GlyphAtlasTexture` 析构会对已销毁的 device 调用 `destroy_texture()`，导致关闭后 SIGSEGV。现在 `close()` 先调 `overlay_host_->clear_content()`（内部 `CanvasLayer::clear_layout_root()`）把内容提前交还，再清场景树与设备。

### 关于「内容超出视口被推离可视区」

这条曾被我列为缺陷，**结论是判断有误，已撤回**。`tests/layout_overflow_tests.cpp` 用确定性的布局断言验证：内容高于容器时，第一个子项仍然落在容器内容区的起点（`Column` / `Row` 两个轴向都验证），溢出部分自然是向尾部裁掉。

当时的「内容跑掉」现象来自我的截图流程：Hyprland 平铺把窗口尺寸改成了请求尺寸之外的值，而我在**读取窗口几何与截图之间**存在竞态，裁剪坐标偏移，于是看起来像内容被推走。这不是引擎缺陷。真正确证的问题是上面第 2、3 条（窗口生命周期），以及主题回调的悬垂捕获。
