# 设计令牌与主题系统

组件一旦把颜色、圆角和字号写死在自己的绘制代码里，亮暗切换、品牌换肤和「只改一个按钮」就会变成散落各处的修改点。NandinaUI 的做法是把「值从哪来」和「组件画成什么样」分开：令牌提供可复用的原子值，语义调色板把原子值翻译成角色，组件配方再把角色和交互状态组装成一个可直接绘制的解析结果。本文说明这条链路在 C++26 主线中的真实形态，以及控件作者应该遵守的约束。

## 三层令牌模型

当前代码的分层比「primitive → semantic → component」这个抽象说法多出一层，因为它把**参考色阶**与**语义色板**分开了：

```text
NanTokens（尺度）+ NanReferencePalette（7 组 11 档色阶，创作输入）
        │  make_color_scheme(reference, appearance, policy)
        ▼
NanColorScheme（语义角色，light / dark 各一份）
        │  组装为共享视觉片段：BoxStyle / TypeStyle / FocusRingStyle /
        │  TrackStyle / ThumbStyle / ControlMetrics / ShadowStyle
        ▼
ComponentRecipes（每组 = base + 有序 rules）
        │  resolve_*(system, appearance, 语义变体…)
        ▼
Resolved*Style（全是 NanColor / float 的具体值）→ painter
```

- **Primitive tokens** 是 `NanTokens`：`spacing`（`xs/sm/md/lg/xl`）、`radius`（`sm/md/lg/full`）、`border`（`thin/medium/focus_ring`）、`opacity`（`disabled/hover_overlay/pressed_overlay`）、`typography`（`label_sm/md/lg`）、`motion`（`short/medium/long_duration`）。它们只提供尺度，不决定组件行为。
- **Reference palette** 是品牌方的着色输入：`NanReferencePalette` 持有 `primary/secondary/tertiary/neutral/success/warning/error` 七条 `NanColorScale`，每条 11 档（`ColorShade::shade_50`…`shade_950`）。主题作者通常用命名的 `NanHexScale` 书写，再由 `nan_color_scale()` 编译为内部 OKLCH 色阶。
- **Semantic palette** 是组件唯一允许引用的颜色层：`NanColorScheme`，字段为 `background/on_background`、`primary/on_primary`、`secondary/on_secondary`、`tertiary/on_tertiary`、`surface/on_surface`、`surface_variant/on_surface_variant`、`outline/outline_variant`、`success/on_success`、`warning/on_warning`、`error/on_error`，加上 `focus_ring` 与 `selection`。它由 `make_color_scheme()` 从参考色阶 + `PaletteVariantPolicy` 生成，控件不直接读参考色阶。
- **Component recipes** 是唯一事实来源：`ComponentRecipes` 为每个组件保存 `{ base, rules }`，`base` 完全指定所有片段，`rules` 是按 `tone/treatment/size/state` 等选择器匹配的增量覆盖。

## 令牌的引用形式

配方字段不是裸值，而是「token 或字面量」的声明：

- 标量用 `ThemeScalar`（即 `ThemeValue<float, ScalarToken>`）：`ThemeScalar::token(ScalarToken::spacing_md)` 或 `ThemeScalar::literal(40.0F)`。`ScalarToken` 枚举名与 `NanTokens` 的字段一一对应（`spacing_*`、`radius_*`、`border_*`、`opacity_*`、`typography_label_*`、`motion_*_duration`）。
- 颜色用 `ThemeColor`：`ThemeColor::token(ColorToken::primary)`、`ThemeColor::literal(nan_color(...))`。`ColorToken` 与 `NanColorScheme` 的语义字段一一对应。
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
- 颜色一律走语义角色，不直接读 `NanReferencePalette`。
- 新增配方字段时同步补 `*RecipeRule` 与 `resolve_*`，让主题作者和实例覆盖都能触达它。

## 主题族群与样式文档

品牌主题以 `ThemeFamilyDefinition { name, reference, policy, tokens, card_rules }` 声明，由 `build_family_design_system()` 编译为内嵌 light/dark 的完整快照，再经 `register_theme_family()` 注册。框架内置 `butter`、`fluent`、`material` 三族；`activate_family()` 对全快照族执行整体原子 apply，此后外观翻转只刷新 `theme()` 视图，不再更换 DesignSystem。

面向主题作者的外部入口是 TOML 样式文档：`StyleDocument` 可声明 `reference_palettes`、`themes`、`theme_families`、`active_theme`、`active_family`、`preference`、遗留 `NanStyle` 规则与字体族，`parse_style_document()` / `load_style_document()` 解析，`apply(ThemeManager&, FontFamilyRegistry*)` 以 `std::expected<void, std::string>` 报告失败。

## 当前已知空白

以下现象都可在当前代码中直接确认，属于尚未收口的部分：

- `TypographyRoles`（`label_sm/md/lg`）已在默认快照中填充，但组件配方仍直接引用 `ScalarToken::typography_label_*`，尚未真正消费该角色层。
- `tone` / `treatment` 这类平铺语义变体目前只有 Button 具备；其余控件以 `state`（Checkbox、Switch、RadioButton 另有 `checked`）为主。
- 遗留 `NanStyle` 只支持 Button 与 TextField 两类规则，因此 `set_style()` 不是通用扩展点。
- 主题通知是虚函数广播（`on_theme_changed` / `on_style_context_changed`），没有响应式主题绑定；部分控件（例如 `Label`）尚未暴露 `set_override`，只有 `on_theme_changed`。
- `motion` token 与 `reduced_motion` 偏好已存在，但动画侧是否全部遵循该偏好不在本文范围内，未作断言。
