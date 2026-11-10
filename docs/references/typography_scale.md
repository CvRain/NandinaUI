# 排版尺度与单位（P4.6 Step 1）

> **状态（2026-11-10）**：Step 1 已实施——Tailwind 兼容的 13 档字号枚举、可主题覆盖的
> `NanTypeScale`、`Leading` 行高 token、`TextStyle` 行高乘数与 `Label` 作者入口
> （`text_size()` / `leading()` / `line_height()`）。Step 2（语义 role 消费、CJK 行高默认
> 迁移、showcase 展示页与 golden 重批）与 Step 3（rem/root 文字缩放与无障碍 cap）仍未实施。
> 本文是契约；`docs/components/label.md` 与 `docs/references/design_tokens.md` 记录用法。

## 1. 单位决定

1. **作者字号单位是逻辑像素（DIP）**，等价于 CSS px：布局、测量、命中与绘制都在逻辑空间，
   平台内容缩放（P4.5a/b 的 `display_scale`）最后一步才映射到物理像素。**OS 缩放始终跟随**，
   不为字号提供绕过通道；物理像素字号只属于测试/探针（`WindowConfig::high_dpi=false`）。
2. **不提供 pt**。pt 在 Web/桌面里通常只是 4/3 逻辑 px 的换算糖；引入两套单位语义会让
   “这个字号到底跟不跟随缩放”变得不可判定。确需印刷来源的数值时，作者自己乘 4/3 后写逻辑 px。
3. **不提供物理像素字号**。同一窗口跨 125%/175% 显示器时物理字号恒定会让文字在另一块屏上
   不可读，破坏 P4.5b 已验收的混合 DPI 行为。
4. **`TextSize` 是相对量，显式 px 是绝对量**（与 CSS 的 rem/px 分野一致）：
   - `TextSize` 通过主题的 `NanTypeScale.root_base`（默认 16）× 比例解析；主题或未来的
     用户文字缩放调整 root_base 时，所有 token 字号一起变化；
   - `Label::set_font_size(px)` / builder `.font_size(px)` 是绝对逻辑 px，不随 root_base 变化，
     即“固定 28px”的语义（在 175% 屏上仍是 28 逻辑 px，由 OS 缩放映射为 49 物理像素）。
5. **不做 vw/vh、em、字符串 class 解析**。桌面 UI 的文字不应随窗口宽高变化；em 的父字号
   相对链当前没有作者需求；字符串 `"text-sm"` 会丢失类型安全，保持 typed DSL。

## 2. 字号尺度（Tailwind 兼容）

`theme::TextSize` 与 `NanTypeScale`（`nandina/theme/typography_scale.hpp`）定义 13 档，
默认 root base = 16 逻辑 px：

| 枚举 | 名称 | ratio | 默认逻辑 px | Tailwind |
| --- | --- | --- | --- | --- |
| `TextSize::xs` | xs | 0.75 | 12 | `text-xs` |
| `TextSize::sm` | sm | 0.875 | 14 | `text-sm` |
| `TextSize::base` | base | 1.0 | 16 | `text-base` |
| `TextSize::lg` | lg | 1.125 | 18 | `text-lg` |
| `TextSize::xl` | xl | 1.25 | 20 | `text-xl` |
| `TextSize::xl_2` | 2xl | 1.5 | 24 | `text-2xl` |
| `TextSize::xl_3` | 3xl | 1.875 | 30 | `text-3xl` |
| `TextSize::xl_4` | 4xl | 2.25 | 36 | `text-4xl` |
| `TextSize::xl_5` | 5xl | 3.0 | 48 | `text-5xl` |
| `TextSize::xl_6` | 6xl | 3.75 | 60 | `text-6xl` |
| `TextSize::xl_7` | 7xl | 4.5 | 72 | `text-7xl` |
| `TextSize::xl_8` | 8xl | 6.0 | 96 | `text-8xl` |
| `TextSize::xl_9` | 9xl | 8.0 | 128 | `text-9xl` |

> **命名说明**：C++ 标识符不能包含 `-`（`xl-3` 会被解析成减法），所以 Tailwind 的 `2xl … 9xl`
> 在代码里写作 `xl_2 … xl_9`；`text_size_name()` 与 Tailwind class 仍是 `2xl … 9xl`。
> 不提供 `xl2` 这类紧凑拼写，避免两个名字并存。

- 常用档位是 xs…xl_5；xl_6…xl_9 面向展示级内容（桌面应用很少用），但一次写入，避免以后
  因缺档位而破坏 API。
- `NanTypeScale` 同时携带 `ratios[13]` 与 `leading[6]`；主题可改 `root_base`、任意单档
  ratio 或任意 leading 值。非法（非有限/≤0）的覆盖在解析时回退到标准值，不产生 NaN 布局。
- 现有 `NanTypographyTokens::label_sm/md/lg` 与 `TypographyRoles` 保留；Step 2 迁移时
  `label_sm/md/lg`（14/16/18）与 `TextSize::sm/base/lg` 数值一致，迁移只改归属不改视觉。

## 3. 行高（Leading）

`theme::Leading` 定义 6 档乘数（相对字号）：

| 枚举 | 名称 | 乘数 |
| --- | --- | --- |
| `Leading::none` | none | 1.0 |
| `Leading::tight` | tight | 1.25 |
| `Leading::snug` | snug | 1.375 |
| `Leading::normal` | normal | 1.5 |
| `Leading::relaxed` | relaxed | 1.625 |
| `Leading::loose` | loose | 2.0 |

- **中文优先**：Step 2 起 body/paragraph 等正文角色默认 `normal`（1.5）；紧凑控件标签可用
  1.25–1.375。当前所有既有文本仍走 `TextStyle::line_height = 0` 的历史默认（字体度量；无度量
  后端回退 `1.2×字号`），所以 Step 1 **不改变现有视觉**；Step 2 迁移并重批 golden 后才切换默认。
- `TextStyle::line_height` 语义：`0` = 后端默认；`>0` = `font_size × 该乘数`（HarfBuzz 与
  deterministic 后端都已消费）。数值必须是有限非负，非法值抛 `std::invalid_argument`。

## 4. 作者 API（Step 1）

```cpp
// Token 路径：跟随主题 root base/ratio，以及未来的用户文字缩放。
ui.make<widget::Label>(graph, "Title")
    .text_size(theme::TextSize::xl_3)    // 默认 30 逻辑 px
    .leading(theme::Leading::tight)     // 1.25 × 字号
    .build();

// 数值 override：覆盖 token；line_height(0) 恢复 token/后端默认。
label->set_line_height(1.9F);

// 绝对逻辑 px：不随 root base 变化，等价 CSS px（不是物理像素）。
label->set_font_size(28.0F);
```

优先级（与组件既有语义一致）：

1. 显式 `set_font_size(px)` / `.font_size(px)`（实例覆盖）；
2. 继承的 `StyleContext.font_size`（父级显式字号）；
3. `set_text_size(TextSize)`（组件级 token）；
4. 组件默认（Label 为 `typography.label_lg` = 18）。
   行高：显式 `set_line_height()` > 最近一次 `set_leading()` token > 后端默认；主题变化时
   token 会重新解析，显式数值保持不变。
5. `set_leading()` / `set_line_height()` 都是布局相关变更：会重测文本（不是 paint-only）。

## 5. 主题覆盖入口

- C++：`NanTokens::typography.scale`（`root_base` / `ratios` / `leading`）随
  `DesignSystem`/`ThemeSchemeContent` 快照进入主题；控件在 `on_theme_changed` 重新解析 token。
- 文件主题：`styles.toml` 目前只支持参考色阶/调色板/主题族/字体与遗留 Button/TextField 规则，
  **尚不支持任意 token 覆盖**（见 `design_tokens.md`“尚未支持”）；P4.6 Step 2 再决定是否
  把 scale 提到文件层。

## 6. 验证（Step 1）

- 单测：`[theme][typography]`（13 档映射/名称/ratio、自定义 root base 与单档覆盖、非法值回退、
  leading 乘数、默认 DesignSystem 携带标准尺度）；`[widget][label][typography]`
  （Label 默认 18、token 解析、主题变化重解析、显式字号优先、行高 token/数值优先级）；
  `[text][layout][line-height]`（deterministic 后端在 `line_height=2` 时行高与总高翻倍，
  `0` 恢复 1.2 默认）；`[authoring][typography]`（builder 链）。
- 红先记录：临时把 `text_layout_backend.cpp` 的行高改回 `font_size * 1.2`，`[line-height]`
  失败 `24.0f == Approx(40.0)`；恢复后通过。
- 全量 Windows 套件与 release gates 在提交时运行；本批不改 showcase 页面与既有 golden。

## 7. 后续（未实施）

- **Step 2**：`TypographyRoles` 扩展为 display/heading/title/body/label/caption；组件配方消费
  role（title/body 等），正文默认 leading 切到 1.5；新增 showcase 排版展示页（新路由会改侧栏
  命中所有页面 golden，需一次集中重批）；文件主题 token 覆盖视需要开放。
- **Step 3**：rem/root 文字缩放——把 `root_base` 暴露为受“用户文字缩放”影响的量，并提供
  `fixed` / `cap` 策略（“最大 28px”的语义落在这里）；`em` 父字号相对链仍非目标。
- **P4.7**：默认字体族只有 Caskaydia Cove Regular 一个 face，`medium/bold/italic` 无可见差异；
  需要多 face 资源或合成回退，Step 2 的 role/字重视觉验收依赖它。
