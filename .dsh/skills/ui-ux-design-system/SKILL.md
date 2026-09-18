---
name: ui-ux-design-system
description: Design-system and UI/UX guidance for authoring, reviewing, or extending NandinaUI themes and widgets. Use when choosing colors, spacing, radius, typography, shadows, or motion for a NandinaUI screen or component; when editing the theme layer (design_system, theme, builtin_themes, StyleDocument TOML); when adding a semantic color role, token, or component recipe; or when auditing a UI for contrast, state coverage, spacing rhythm, and semantic-token discipline.
whenToUse: Load before touching nandina/theme/*, a built-in theme family, a styles.toml document, or any widget's default recipe; also load when reviewing a screenshot or playground page for visual quality.
metadata:
  source: adapted from nextlevelbuilder/ui-ux-pro-max-skill (design-system references)
  upstream: https://github.com/nextlevelbuilder/ui-ux-pro-max-skill
  license: see references/ATTRIBUTION.md
---

# UI/UX 设计系统指南（NandinaUI 版）

本 skill 把业界通行的三层令牌模型、交互状态覆盖清单与视觉审查标准，落到 **NandinaUI 真实的 C++ 令牌/配方 API** 上。它不替代代码阅读：动手前先读 `nandina/theme/design_system.hpp` 与 `docs/references/design_tokens.md`。

## 铁律

1. **只用语义角色，不用原始色。** 组件配方的颜色字段一律引用 `ThemeColor::token(ColorToken::…)`。不要在配方、控件或页面里写 `NanColor::from_hex(...)` 或 `nan_color(...)` 字面量（`ThemeColor::literal` 仅用于确实无法用语义表达的一次性值，并在注释里说明理由）。
2. **字面量必须显式。** 标量用 `ThemeScalar::token(ScalarToken::…)`；确需具体值时用 `ThemeScalar::literal(...)` 并注明为什么现有 token 不够。
3. **不为每个颜色/圆角/状态新增平铺 setter。** 差异要归到正确的维度：色族（tone）、外观处理（treatment）、尺寸（size）、交互状态（state）。
4. **新增配方字段必须四处同步**：`XRecipeRule`（字段全部 `std::optional`）、`resolve_x`、`apply_rule`、以及 `*Recipe` 的 base。漏掉任何一处都会让主题作者或实例覆盖触达不到该字段。
5. **改主题层不要碰组件行为。** 配方只描述"画成什么样"；布局、事件、焦点属于控件。主题改动若需要改控件逻辑，说明是架构问题，先提出来讨论。
6. **对比度是硬门槛。** 正文文字与背景对比 ≥ 4.5:1，大字（≥ 18px 或 14px 粗体）≥ 3:1，非文本的边框/图标/指示器 ≥ 3:1。`nandina/foundation/contrast.hpp` 可用于校验。

## 三层令牌模型

与上游 skill 的 primitive → semantic → component 完全同构，落到 NandinaUI 是：

| 层 | NandinaUI 实体 | 谁改 | 何时改 |
| --- | --- | --- | --- |
| Primitive | `NanTokens`（spacing / radius / border / opacity / typography / motion）、`NanReferencePalette` 的 7×11 色阶 | 主题作者 | 很少，属于地基 |
| Semantic | `NanColorScheme`（light/dark 各一份）+ `TypographyRoles` | 主题作者 | 换肤、明暗、品牌 |
| Component | `ComponentRecipes`（`base` + 有序 `rules`） | 组件作者 | 新增/调整组件造型 |

解析链路（单向，不可逆）：

```text
ThemeFamilyDefinition / DesignSystem 快照
        │  resolve_color(system, appearance, ThemeColor, tone)
        ▼
Resolved*Style（全是具体 NanColor / float） → painter
```

`DesignSystem` 是**与外观无关的不可变快照**（内嵌 light + dark）；`NanTheme` 只是 `{tokens, 单一 palette}` 的解析视图，供遗留接口读取。控件持有 `shared_ptr<const DesignSystem>`，在 `on_theme_changed()` 里重解析。

## 语义色角色

`NanColorScheme` 的角色与用途（新增角色时按同一命名约定：底色 `x` 配前景 `x_foreground` / `on_x`）：

| 角色 | 用途 | 不该用它做 |
| --- | --- | --- |
| `background` / `on_background` | 页面底色与默认文字 | 卡片、弹层（用 `card` / `popover`） |
| `card` / `on_card` | 卡片、面板容器 | 页面底、浮层 |
| `popover` / `on_popover` | 浮层内容面（Tooltip / Select 弹层 / Menu / Dialog 面板） | 常规卡片 |
| `surface` / `on_surface` | 通用次级表面（容器、分组底） | 页面底 |
| `surface_variant` / `on_surface_variant` | 与 `surface` 区分的弱化表面、次要文字 | 正文文字 |
| `primary` / `on_primary` | 主操作、选中态、品牌强调 | 次要按钮、hover 底 |
| `secondary` / `on_secondary` | **中性次操作**（次按钮、中性 Badge） | 品牌辅色（用 `tertiary`） |
| `tertiary` / `on_tertiary` | 品牌辅色、装饰性强调 | 主操作 |
| `accent` / `on_accent` | **hover / 选中 / 下拉高亮底色** | 品牌强调（那是 `primary`） |
| `muted` / `on_muted` | 弱化底：禁用底、表头、代码块、骨架屏 | 正文文字底 |
| `outline` / `outline_variant` | 强/弱边框 | 填充 |
| `input` | 表单控件边框（与 `outline` 分档以便独立调） | 卡片边框 |
| `focus_ring` | 焦点环 | 选中指示（那是 `primary`） |
| `selection` | 文本选区底 | 焦点环 |
| `success` / `warning` / `error` / `info`（各带前景） | 语义状态 | 普通强调 |

**三个最常被混淆的点**：

- `accent` 是 hover/选中底色，**不是**品牌色。品牌色是 `primary`。
- `secondary` 是中性次操作色，**不是**品牌辅色。品牌辅色是 `tertiary`。
- `muted` 是弱化**底**，弱化**字**用 `on_muted`。次级说明文字优先 `on_muted` 或 `on_surface_variant`，不要用 `outline`。

> 代码里的既有名字：`ThemeColor::accent()` / `ThemeColor::on_accent()` 指的是 **Button tone 的强调色对**（`button_accent()`，随 tone 解析），与上面 `ColorToken::accent` 这个调色板角色**不是同一件事**。写配方时看清是哪一个。

## 尺度规范

- **间距用 4px 网格。** 组件内部 `spacing_xs/sm/md/lg`，区块之间用 `spacing_xl` 或字面量（须标注）。同一容器内的间隙要一致，不要混用 10/12/14。
- **圆角由 `--radius` 派生。** shadcn 的映射是 `sm = radius - 4`、`md = radius - 2`、`lg = radius`；NandinaUI 直接存三个 token，改主题时三者要一起调，别只改 `radius_md`。
- **字号只用排版 role。** 需要新字号时先加 `TypographyRoles` 条目，而不是在配方里散写 `ThemeScalar::literal(17.0F)`。
- **阴影要有方向一致性。** 同一界面里阴影的 `offset_y` 符号与量级要一致；浮层阴影比卡片重一档。
- **边框宽度只用 `border_thin` / `border_medium`。** 焦点环用 `border_focus_ring`。

## 交互状态覆盖清单

任何可交互控件的配方至少要让这些状态可区分（未实现的规则要在文档里标明是已知空白）：

| 状态 | 必须变化的东西 |
| --- | --- |
| normal | 基准填充 / 边框 / 文字 |
| hover | 底色切到 `accent`（或状态层 `state_layer.hover`） |
| pressed / active | 比 hover 更深一档，**不能与 hover 相同** |
| focused | `focus_ring` 可见（键盘可达性；`width` 为 0 表示该控件尚未接通） |
| disabled | alpha 统一按 `opacity_disabled` 缩放，并隐去焦点环 |
| selected / checked | 与 normal 有明确区分（底色或指示器），不能只靠文字颜色 |
| invalid | 边框/文字走 `error`，且**不要只靠颜色**传达（配图标或文案） |
| loading / pending | 保持布局尺寸不变，避免跳动 |

**禁止**：用 hover 色当 pressed 色；用颜色作为唯一的状态信号；给 disabled 元素留焦点环。

## 视觉审查流程

审查一个界面或截图时，按顺序过：

1. **语义纪律** —— 有没有硬编码颜色？有没有该用 `card`/`popover` 却借 `surface` 的地方？
2. **对比度** —— 正文、次要文字、禁用文字、边框、图标逐个量。次要文字最容易不达标。
3. **层级** —— 背景 / 卡片 / 浮层三层的明度关系是否单调递增（亮色）或递减（暗色）？相邻层级明度差是否 ≥ 2%（OKLCH L）？
4. **间距节奏** —— 同一容器内的 gap 是否同值？内外边距是否成比例（内 ≤ 外）？
5. **状态覆盖** —— 对照上表，缺哪个状态？hover 与 pressed 是否可区分？
6. **明暗对称** —— 切到 dark 后：文字是否仍 ≥ 4.5:1？纯色边框是否该改成半透明白（shadcn 暗色 `border`/`input` 用的是 `oklch(1 0 0 / 10%)`）？阴影是否仍可见（暗色下应更弱或改用边框）？
7. **对齐** —— 基线、图标中心、左右边距是否对齐；等宽高元素是否真的等宽高。

## 参考文件

- [`references/primitive-tokens.md`](references/primitive-tokens.md) —— 尺度基准值（4px 网格、字号、圆角、阴影、时长、z-index）。
- [`references/semantic-tokens.md`](references/semantic-tokens.md) —— 语义角色定义与明暗映射写法。
- [`references/component-tokens.md`](references/component-tokens.md) —— 各组件应引用哪些角色（Button / Input / Card / Badge / Alert / Dialog / Table）。
- [`references/states-and-variants.md`](references/states-and-variants.md) —— 状态与变体的组合规则。
- [`references/component-specs.md`](references/component-specs.md) —— 组件规格与使用条件。
- [`references/token-architecture.md`](references/token-architecture.md) —— 三层模型与迁移写法。
- [`references/ATTRIBUTION.md`](references/ATTRIBUTION.md) —— 上游来源与许可。

仓库内对应文档：`docs/references/design_tokens.md`（NandinaUI 主题系统真实形态）、`docs/references/component_contract.md`（组件契约）。
