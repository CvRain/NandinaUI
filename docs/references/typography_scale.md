# 排版尺度与单位（P4.6 Step 1 + Step 2a + Step 2b + Step 3）

> **状态（2026-11-11）**：Step 1 已实施——Tailwind 兼容的 13 档字号枚举、可主题覆盖的
> `NanTypeScale`、`Leading` 行高 token、`TextStyle` 行高乘数与 `Label` 作者入口。
> **Step 2a 已实施**：语义 `TypographyRole` 7 角色与 `Label::set_typography_role()`。
> **Step 2b 已实施（2026-11-11）**：role 表纳入 `NanTypeScale::roles` 并支持主题覆盖；
> **Label 默认 role 改为 body（16 px / normal 1.5）**，未显式设置字号/行高的文本获得中文优先
> 的正文行高；显式字号仍保留后端默认行高。新增 showcase Typography 展示页（字号/行高/role/
> body 默认四节）。**组件配方已迁移到语义 role 尺寸 token**（`ScalarToken::typography_role_*`，
> 默认配方不再直接引用 `typography_label_*`；C++ 主题覆盖 role 表会同步改变组件字号），
> styles.toml 也可引用 `$tokens.typography.role.*`；role 表可通过 styles.toml
> `[themes.<name>.roles.<role>]` 覆盖。**Step 2b 收口已实施（2026-11-12）**：默认组件配方消费
> role 的 leading/weight/slant（`TypeStyle::role` → `ResolvedTypeStyle` → 统一文本桥），
> 手工构造文本样式的组件迁移到 `widget::internal::apply_type_metrics()`；双平台各 8 条
> （共 16 条）SDF/CPU 页面 golden 重新采集并由维护者批准，label/typography 页 **0 像素差异**，
> 差异集中在按钮文字的 500 合成字重与随后的 1–2 px 宽度调整。
> **Step 3 模型已实施（2026-11-12）**：`NanTypeScale` 新增 `text_scale`（rem 语义的用户文字
> 缩放因子）与 `max_text_px`（可选 cap，0 关闭）；token/role 字号按
> `root × ratio × text_scale` 解析后再 cap，显式 `.font_size(px)` 保持 fixed；运行时宿主入口
> `ThemeManager::set_text_scaling()` 已提供（重建快照 + 发布 revision）。
> **OS 来源子批已实施（2026-11-13）**：`app/system_preferences.hpp` 的
> `ISystemPreferencesSource` 经 `IPlatformWindow::system_preferences_source()` 注入，
> `NanApplication` 负责 attach/sync/poll 与 override 仲裁；Windows SDL 平台从注册表
> `TextScaleFactor` 节流读取并在首帧前应用。Linux 首版没有 OS 来源（返回 nullptr，宿主可
> 注入自己的来源）；系统变化通知的具体实现与持久化决策仍待。
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

## 4. 作者 API（Step 1 + Step 2a）

```cpp
// 语义 role：一次给出字号 + 行高 + 字重/斜体默认值（颜色仍走组件的颜色 token）。
ui.make<widget::Label>(graph, "Title")
    .typography_role(theme::TypographyRole::heading)
    .build();

// Token 路径：跟随主题 root base/ratio，以及未来的用户文字缩放。
ui.make<widget::Label>(graph, "Title")
    .text_size(theme::TextSize::xl_3)    // 默认 30 逻辑 px
    .leading(theme::Leading::tight)     // 1.25 × 字号
    .build();

// 数值 override：覆盖 token；line_height(0) 恢复 token/role/后端默认。
label->set_line_height(1.9F);

// 绝对逻辑 px：不随 root base 变化，等价 CSS px（不是物理像素）。
label->set_font_size(28.0F);
```

优先级（与组件既有语义一致）：

1. 显式 `set_font_size(px)` / `.font_size(px)`（实例覆盖）；
2. 继承的 `StyleContext.font_size`（父级显式字号）；
3. `set_text_size(TextSize)`（组件级尺度 token）；
4. `set_typography_role(role)` 的 role 字号；
5. 组件默认（Label 默认 role = `body`：base 16 px + normal 1.5 leading）。
   行高：显式 `set_line_height()` > `set_leading()` token > role 的 leading > body 默认
   （仅当没有显式/继承/`text_size` 字号时）> 后端默认；
   字重/斜体：显式 `set_font_weight/slant` 或继承 font context > role 的 weight/slant；
   主题变化时 token/role 会重新解析，显式数值保持不变。
6. `set_leading()` / `set_line_height()` 都是布局相关变更：会重测文本（不是 paint-only）。

## 5. 语义排版 role（Step 2a）

`theme::TypographyRole` 在尺度/行高之上定义 7 个用途角色。role 只描述类型度量（字号、行高、
字重、斜体），**颜色不属于 role**，仍由组件的语义色 token（如 `foreground`）决定，避免角色与
配色耦合。默认映射（root base 16）：

| role | TextSize | 默认逻辑 px | Leading | weight | 典型用途 |
| --- | --- | --- | --- | --- | --- |
| `display` | xl_4 | 36 | tight (1.25) | 600 | 页面主视觉标题 |
| `heading` | xl_2 | 24 | snug (1.375) | 600 | 区块标题 |
| `title` | xl | 20 | snug (1.375) | 600 | 卡片/列表标题 |
| `subtitle` | lg | 18 | normal (1.5) | 500 | 次级标题 |
| `body` | base | 16 | normal (1.5) | 400 | 正文（中文优先行高） |
| `label` | sm | 14 | tight (1.25) | 500 | 控件标签 |
| `caption` | xs | 12 | snug (1.375) | 400 | 说明/元数据 |

- role 的 px 与 leading 仍经主题 `NanTypeScale` 解析：主题改 `root_base`、ratio 或 leading
  factor，role 结果随之变化（见 `[widget][label][typography]` 的自定义主题用例）。
- **Step 2b：role 表可通过 `NanTypeScale::roles` 主题覆盖**（例如
  `scale.roles[typography_role_index(TypographyRole::heading)] = {.size = TextSize::xl_5, ...}`）；
  未覆盖的 role 保持 canonical 表。role 的 weight/slant 自 P4.7 起可见（合成回退）。
- **Label 默认 role = body**：没有显式字号/`text_size`/继承字号时，Label 使用 16 px + normal
  1.5 leading；`set_font_size()`、`set_text_size()` 或 StyleContext 字号会保留后端默认行高，
  除非作者显式给 leading/line-height。现有页面 golden 不受影响（showcase 文本大多显式指定
  字号）；Typography 展示页演示了这条默认。
- **组件配方消费 role 尺寸与度量（2026-11-12 收口）**：默认 `Button`/`TextField`/`TextArea`/
  `Alert`/`EmptyState`/`Dialog`/`Tabs`/`Tooltip`/菜单与选择器族等配方改用
  `ScalarToken::typography_role_*` 并给文本槽位带上语义 `role`；解析后 role 的 leading/
  weight/slant 经 `TypeStyle::role` → `ResolvedTypeStyle` → 统一文本桥（手工构造路径用
  `widget::internal::apply_type_metrics()`）进入控件。紧凑控件映射 label role（1.25 / 500），
  多行 `TextArea` 用 body role（1.5）；字号 token 不变。显式样式上下文字体与组件级显式
  weight/slant（如 `Button::set_font_weight()`）保持优先。
- **豁免**：Slider 的数值标签沿用字面 12 px，没有配方 `TypeStyle`，本批不接入 role 度量；
  正文行高的完整 CJK 页面审计按需扩展。
- Step 2a 只新增 API/测试，不改任何组件默认；Step 2b 的 Label 默认变化不改变已批准的
  8 条页面 golden（Windows/Linux 复测 0 差异）。

## 6. 主题覆盖入口

- C++：`NanTokens::typography.scale`（`root_base` / `ratios` / `leading` / **`roles`**）随
  `DesignSystem`/`ThemeSchemeContent` 快照进入主题；控件在 `on_theme_changed` 重新解析 token。
- 文件主题：`styles.toml` 的配方字段可以引用 `$tokens.typography.role.display…caption`
  （解析到当前 role 尺寸），见 `design_tokens.md`“令牌的引用形式”。**role 表可按主题覆盖**：

```toml
[themes.dark.roles.label]
size = "xl"        # TextSize 名称
leading = "loose"  # Leading 名称
weight = 700       # 1..1000
slant = "italic"   # normal / italic / oblique
```

  未给出的字段保留默认 canonical 值；未知 role/枚举值会作为解析错误返回。C++ 仍可直接改
  `NanTypeScale::roles`。

## 7. 验证（Step 1 + Step 2a + Step 2b）

- 单测：`[theme][typography]`（13 档映射/名称/ratio、自定义 root base 与单档覆盖、非法值回退、
  leading 乘数、默认 DesignSystem 携带标准尺度、7 个 role 的映射/名称、**role 表主题覆盖与
  越界 role 回退 body**）；`[widget][label][typography]`
  （**Label 默认 body 16/1.5**、显式字号/`text_size` 后的行高行为、token 解析、主题变化重解析、
  显式字号优先、行高 token/数值优先级、role 字号/行高/字重、**主题覆盖 role 表**）；
  `[theme][typography][recipe]`（默认 `Button` 配方引用 role 尺寸 token；覆盖 label role
  为 xl 后解析字号 20 px）；`[theme][toml][typography]`（styles.toml 引用
  `$tokens.typography.role.subtitle` 解析为 18 px）；`[theme][typography][scaling]`（用户缩放
  1.25× 下 base 16→20、caption 12→15、display 36→45；cap 28 后 display 被限制；非法因子/cap
  回退）；`[widget][label][typography][scaling]`（Label 默认 body 随 1.25× 变为 20 px，
  显式 21 px 保持 fixed，token 字号缩放，cap 18 生效）；`[theme][manager][typography][scaling]`
  （运行时 `set_text_scaling(1.25,28)` 发布 revision、DesignSystem/theme 同步缩放，Button
  标签 14→17.5 px；外观切换后保持；相同值不发布；NaN/-1 重置为 1.0/0）；
  `[theme][typography][role-metrics]`（`TypeStyle::role` 把 leading/weight/slant 带入
  `ResolvedTypeStyle`，role 表覆盖后 700/italic/2.0 生效）与
  `[widget][typography][role-metrics]`（统一文本桥把行高/字重/斜体写入 `TextStyle`；
  显式样式上下文字体仍优先）；
  `[theme][toml][typography][roles]`
  （styles.toml 的 `[themes.dark.roles.label]` 覆盖 size/leading/weight/slant，Button 标签
  随之变为 20 px；未知 role/尺寸名返回解析错误）；
  `[text][layout][line-height]`；`[authoring][typography]`。
- 红先记录：Step 1 行高后端、Step 2a display role size；Step 2b role 表覆盖/越界回退；组件
  role 消费用例把 Button 配方临时改回 legacy 字号 token 后失败（`*token` 是 label_sm、不是
  role_label），恢复后通过；styles.toml role 表用例临时跳过 `parse_role_overrides()` 后
  label role 仍为 sm、测试失败；Step 3 临时让 `text_size_px()` 直接返回 `root × ratio` 后，
  `[theme][typography][scaling]` 的 `16 == 20` 失败；运行时入口用例把 `set_text_scaling()`
  临时改成 no-op 后 revision 未变化而失败；role metrics 用例把解析结果临时改回默认
  0/400/normal 后行高断言失败，文本桥用例把映射临时改回默认后同样失败；恢复后均通过。
- **2026-11-12 B1 收口验证**：`[theme][typography][recipe][role-metrics]`（默认配方带 role；
  覆盖 role 表同时改变字号/行高/字重/斜体）、`[widget][button][typography][role-metrics]`、
  `[widget][text-field][typography][role-metrics]`。红先：默认配方去掉 `.role` 后断言失败，
  组件侧移除 `apply_type_metrics()` 调用后行高/字重断言失败。Windows 全量 125/125（unit
  104/104）、Linux 全量 129/129；矩阵 ASan unit 104（+1 预期 SKIP）、no-RTTI unit 106/106、
  no-SQLite unit 101（+1 预期 SKIP）、纯 tools 8/8，`release_check --ci` 8/8；双平台各 8 条
  页面 golden 按原始协议重采（CPU = GPU 设备 + SDF off，SDF = GPU 设备）后由维护者批准：
  label/typography 8 条 0 像素差异，home/anchors 差异只落在按钮文字与 1–2 px 宽度调整。
- **2026-11-13 OS 来源子批验证**：`[app][system-text-scale]` 覆盖 percent 映射边界（100/125/
  150/225/0/99/226/0xFFFFFFFF）、override > OS > 1.0 仲裁、同值不重复发布 theme revision、
  非法值回退、首个来源生效与 detach；`[app][windows][text-scale]` 在 Windows 上真实读取注册表
  并只断言文档范围 1.0–2.25。**2026-11-13 端到端验收**：临时将注册表设为 150 后以
  `NANDINA_EXPECT_TEXT_SCALE=1.5` 运行真实来源用例，OS→`WindowsSystemTextScaleSource`→
  `NanApplication`→`ThemeManager` 全链路通过（39 断言），随后恢复原值并复跑通过（详见
  [Windows 验收](windows_acceptance.md) §10）；窗口内文字重排的人工目检仍待。红先：临时忽略
  override 后 23 条断言中 1 条失败；临时放宽 percent 上界后 32 条断言中 1 条失败，恢复后通过。
  Windows 全量 129/129；Linux 全量 133/133，矩阵 ASan unit 108（+1 预期 SKIP）、no-RTTI 110、
  no-SQLite 105（+1 预期 SKIP）、纯 tools 通过，`release_check --ci` 8/8。
- 全量 Windows/Linux 套件与 release gates 在提交时运行；**2026-11-11 中文版重批**仍是
  TextField hover/菜单高 DPI/图集修复后的历史基线记录。

## 8. Step 3：用户文字缩放与 cap（已实施 2026-11-12）

`NanTypeScale` 新增两个字段，默认值保证现有视觉不变：

| 字段 | 默认 | 语义 |
| --- | --- | --- |
| `text_scale` | `1.0` | 用户/无障碍文字缩放因子（rem 语义）；非有限/非正回退 1.0 |
| `max_text_px` | `0.0` | 可选上限（逻辑 px），0 关闭；正有限值在缩放后对 token/role 结果取 min |

解析规则（唯一入口 `text_size_px()`）：

```text
text_size_px(scale, size)
  = min(root_base × ratio[size] × text_scale, max_text_px if > 0)
```

- **fixed**：显式 `.font_size(px)` / `set_font_size(px)` 是绝对逻辑 px，不乘 `text_scale`、
  不受 cap 影响（CSS px 语义）；`TextSize`/role 派生字号才缩放。这保留“作者明确指定像素”
  的稳定性，同时让正文/标题等 rem 语义文本跟随用户设置。
- **cap**：`max_text_px` 是唯一 cap 入口。例如主题设 `text_scale=1.5, max_text_px=28`，
  display role 的 36 px 结果被限制在 28 px；普通 body 16×1.5=24 px 不受影响。
- **行高**：`leading` 仍是字号倍数，缩放过字号后行距自然跟随；role 的 leading/weight 消费
  限制同 §7。
- **宿主运行时入口**：`ThemeManager::set_text_scaling(scale, max_px)` 接受宿主/应用传入的用户
  或无障碍文字缩放；它重建带缩放 token 的有效 `DesignSystem` 快照并发布一次 theme revision，
  已挂载控件因此在 `on_theme_changed` 中重解析。覆盖值是宿主状态，切换外观/方案后仍保持；
  相同值不重复发布，非法值重置为 1.0/0。应用可通过 `NanApplication::theme_manager()` 调用。
  显式 `.font_size(px)` 仍 fixed。
- **OS 来源子批（2026-11-13）**：`nandina/app/system_preferences.hpp` 定义
  `SystemPreferencesSnapshot`、`ISystemPreferencesSource`（`snapshot()` + 自节流的
  `poll_changes()`）与纯函数 `text_scale_from_percent()`（Windows 100–225 → 因子，越界
  视为无来源）。`IPlatformWindow::system_preferences_source()` 默认返回 nullptr；
  `NanWindow::open()` 在首帧构建前 attach 平台来源并 sync 一次，`tick()` 在输入阶段前
  `poll_system_preferences()`。仲裁顺序为 **应用 override > OS 快照 > 1.0**，非法值按 1.0；
  同一有效值/ cap 不重复发布 theme revision。Windows SDL 平台读取
  `HKCU\Software\Microsoft\Accessibility\TextScaleFactor`（REG_DWORD，约 500 ms 节流）；
  Linux 首版没有来源，宿主可注入自己的 `ISystemPreferencesSource`。持久化归应用（框架不写
  配置/注册表）；显式 `.font_size(px)` 仍 fixed。
- `em` 父字号相对链、每组件独立 fixed 列表不是本批目标；“fixed/cap”指上述显式 px 与全局上限。

## 9. 后续（未实施）

- **Step 2b 已收口（2026-11-12）**：默认组件配方消费 role 的尺寸与 leading/weight/slant，
  手工文本样式组件已迁移到统一 bridge；仅 Slider 数值标签按豁免保留字面字号。
  正文行高的完整 CJK 页面审计按需扩展。
- **Step 3 剩余**：OS 来源在 Windows 已接（2026-11-13 子批），Linux 平台来源、系统设置变化
  通知的具体实现（当前为轮询）与持久化决策仍待；如需要，再评估每角色独立 cap 或组件级
  fixed 列表。`em` 父字号相对链仍非目标。
- **P4.7**：默认字体族只有 Caskaydia Cove Regular 一个 face；合成回退已于 2026-11-10
  实施，因此 role 表的 500/600 字重现在可见。**2026-11-12 决策：本轮不投资真实多 face
  资产**（许可、包体积、CJK 覆盖与全部页面 golden 重批成本），合成回退继续作为默认路径；
  若后续要做真实 Medium/Bold/Italic，按 P4.7 后续增强单独立批。
