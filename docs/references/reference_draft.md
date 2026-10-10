# 待决设计草案

> **这是工作稿，不是决定记录。** 三件事都已核实到代码层面，但**都还没拍板**。
> 拍板之后：决定写进对应的 reference（`component_contract.md` / `theme` 相关 / `page_and_router.md`），
> 过程补充到 stories；本文随之删除或降级为历史草案。
>
> 文件名沿用当前叫法；若想改成 `open_decisions.md` 这类更直白的名字，改索引一行即可。

三件事彼此独立，可以分开拍板、分开实施：

| # | 待决事项 | 现状 | 阻塞谁 |
| --- | --- | --- | --- |
| ~~D1~~ | ~~焦点环什么时候显示~~ → **已定案** | 见 [统一输入、焦点与文本编辑](focus_and_input.md)；几何修复已落地 | — |
| **D2** | 主题变化**发不发信号** | 只能写不能订阅；外壳无法响应外观变化 | 外壳里一切"跟随主题"的东西 |
| **D3** | 路由分组类型**要不要类型安全** | `std::string`，拼错静默多一段 | 导航分组顺序与正确性 |
| **D4** | 平台层与渲染后端**走向哪里** | raylib（GLFW + OpenGL）；接口已中立、形状是自研 SDF | Windows / Web / Linux 桌面组件（dock、panel） |

---

# D1 焦点环的显示时机 —— **已定案（2026-10-09），不要按本节旧稿实施**

本节的旧建议（按"最近输入设备"决定是否显示、把触摸/手柄归入 pointer、立即增加
`.focus_ring(false)`）**已被取代**。正式依据是
[统一输入、焦点与文本编辑](focus_and_input.md)：按**交互意图**（指向定位 / 焦点导航 / 编辑输入）
而不是设备决定呈现策略；文本输入控件是独立能力类别；第一阶段不增加通用 `.focus_ring(false)`。

保留几何修复那一半（本方框 → 同心圆角环，已落地）；过程与两次判断失误见
[story 009](../../stories/009-focus-ring-and-the-missing-input-modality.md)。

---

# D2 主题变化的订阅

## 背景

给侧边栏 footer 做亮/暗开关时发现它**单向**：能写偏好，外观从别处变化时不更新。根因不在开关，
而在 `ThemeManager` 没给应用/外壳任何可订阅的口子。凡"外壳要跟随外观变化"的需求都撞这堵墙。

## 现状核实

| 事实 | 位置 |
| --- | --- |
| 只有 `revision()`（轮询）与 `ThemeObserver`（给 `SceneTree` 用） | `theme_manager.hpp` |
| 树收到 revision 后向挂载节点广播 `on_theme_changed` | `scene_tree` |
| `ThemeManager` 由 `NanApplication` 持有，比外壳作用域活得久 | `nan_window.cpp` 的 `set_shell` |
| 初始态读取是**对的**，缺的只是后续更新 | 硬编码 `checked` 真/假两次截图相差 359 像素 |

## 选项

| 选项 | 做法 | 代价 | 风险 |
| --- | --- | --- | --- |
| **A 信号（倾向）** | `ThemeManager` 暴露 `Signal<ColorAppearance>`（"解析后的外观变了"）；`reduced_motion` 视需要另给一条 | 中：新增信号 + 与现有广播去重 | 与 `revision()` 语义混淆 |
| B 开放观察者 | 让应用也能注册 `ThemeObserver` | 小 | 把注册/注销/生命周期下推给每个消费者 |
| C 保持现状 | 谁来响应谁自己轮询 `revision()` | 0 | 每处各写一遍，且 `revision()` 语义是"快照换了"而非"外观变了" |

## 倾向

**A**，并明确语义分工。⚠️ **本文早先把 `revision()` 说成"快照换了"，这是错的**（2026-10-10 核实）：
`ThemeManager::set_system_preferences` 在**仅 reduced-motion 变化**时也会 `publish_revision()`
（`theme_manager.cpp`），所以 `revision()` 表示"内部呈现可能失效"，它既不等于"快照指针换了"，
也不等于"外观变了"。更精确的边界与四维观察模型见
[主题方案、作者入口与状态订阅](theme_authoring_and_state.md) §5.1；本节只保留问题语境，
不再作为设计依据。
两者不要合并成一条 —— 外壳想知道的通常只是后者。

## 待决问题

1. 信号载荷：`ColorAppearance`，还是整个 `SystemPreferences`（外观 + reduced motion）？
2. 与 `SceneTree` 已有广播的**次序与去重**（一次切换不能通知两轮）。
3. `reduced_motion` 是否同通道（它是同一类"偏好驱动的解析结果"）。
4. 补上之后，侧边栏开关的"已知边界"注释可以删掉（story 010 记录的遗留）。

---

# D3 分组类型的类型安全

## 背景

侧边栏按 `RouteOptions::type` 分段。分组实现已落地（`Routes::nav_sections()`，顺序 = 各 type
首次出现的顺序，注入 `unordered_map` 版本会当场变红）。留下的是类型问题：**`std::string`，
拼错不报错，只会在导航里多出一个带错字的分组**。

## 现状核实

| 事实 | 位置 |
| --- | --- |
| `std::string type{"default"}` | `nan_router.hpp` 的 `RouteOptions` |
| 分组是稳定有序的（不依赖哈希表） | `Routes::nav_sections()` + `[app][router][nav]` 测试 |
| 未声明 type 的条目自成一段（标签是 `"default"`） | 同上 |

## 选项

| 选项 | 做法 | 拼错时 | 扩展性 |
| --- | --- | --- | --- |
| **A 枚举（倾向）** | `enum class RouteSection { manual, form, button, data, panel, overlay, menu, dialog, misc }` | 编译不过 | 需要决定第三方分组怎么办 |
| B 字符串 + 校验 | `validate()` 里检查非空 / 白名单 | 声明期报错（若白名单） | 最好 |
| C 字符串 + 集中常量 | `namespace route_section { inline constexpr auto form = "表单"; }` | 仍不报错，但避免重复字面量 | 好 |

## 倾向

**A**：把拼写错误挡在编译期，和项目"不要让错误变成静默的 UI 变形"一致。唯一的顾虑是扩展性 ——
若需要"库外自定义分组"，用"枚举 + 可选自定义标签"折中，而不是退回纯字符串。

## 待决问题

1. 是否需要库外自定义分组？（决定 A 还是 A+自定义标签）
2. 枚举命名：标识用英文（`form`）而标签用中文（"表单"）？还是标识本身就是显示名？
3. 分组**顺序**是否需要一个显式声明处（现在是"首次出现顺序"，隐含在声明位置）；
4. 声明期校验的落点：`Routes::validate()` 是现成的关口，适合放 B 的检查。

---

# D4 平台层与渲染后端：从 raylib 走向哪里

## 背景（作者的目标，原话归纳）

- **跨平台**：Linux 已在跑；Windows 想要；**Web 更佳**；
- **目标场景**：做 QuickShell 那一类东西 —— 用本框架直接做 Wayland 合成器步子太大，
  先做 **Hyprland 上的 dock / panel**；
- **对现状的不满**：raylib 是 GLFW + OpenGL；在 Linux/Wayland 上 SDL3 + Vulkan 更好 ——
  显式同步/更低延迟、dmabuf 零拷贝与合成器协作、摆脱 GL 驱动差异、以及作为通用渲染器的长期可移植性；
- **计划**：现有开发工作完成后，考虑切换到 **SDL3 + ThorVG**，不直接写 Vulkan 代码，
  同时保住跨平台能力，库足够轻。

## 现状核实（代码层面）

这一节是本文最有价值的部分：**换后端的真实成本，比"换掉 raylib"这个说法听起来小得多**。

| 事实 | 证据 |
| --- | --- |
| 设备接口**本来就是中立**的 | `render/render_device.hpp:5-6`：*"Backends submit primitives to a concrete renderer (raylib / offscreen / recording test double). **No raylib types appear in this header.**"* |
| 形状不是 raylib 画的，是**自研 SDF** | `backends/raylib_device.cpp` 走 `SdfPrimitiveMode::fill / outline / segment / clipped_circle` + `sdf_*_geometry` + 自定义 shader（`draw_aa`） |
| 窗口层很薄且自认独占 | `app/nan_window.cpp:4`：*"raylib is included ONLY here"*；文件里 raylib 相关调用只有约 9 处 |
| raylib 的**真实**触点只有 5 个文件 | `nan_window.{hpp,cpp}`、`backends/raylib_device.{hpp,cpp}`、`render/image_decoder.*`、`widget/key_codes.hpp`、`text/font_pipeline.cpp`（只剩一个 `raylib_bytes_per_atlas_pixel = 5` 的常量） |
| 图片解码已经是可替换的接口 | `image_decoder.hpp` 暴露 `IImageDecoder` + `make_raylib_image_decoder()` |
| 平台特化已经隔离 | `app/detail/linux_clipboard.*`、`resource/platform_resource_locator.cpp`、`app/application_config.cpp` |
| 接口形状**已被大量消费者验证** | 唯一生产实现是 `raylib_device.cpp`，另有约 30 个测试替身（`tests/*` 的 `RecordingDevice`）与 `playground/shell.cpp` |
| `scene` 内核明确不依赖 raylib | `scene/control.hpp:12`：*"这一层是纯 scene 语义, **不依赖 reactive / raylib**, 保持内核独立"* |

**结论**：新后端 = **实现约 25 个虚函数 + 一层窗口/输入**，而不是重写框架。几何、布局、
主题、动画、文本整形、语义都在接口之上，不随后端改变。

## 但这**不是一件事，是四件**

把它们捆成"切到 SDL3 + ThorVG"会让它看起来像一次大爆炸重写，而四件事的成本、风险、
甚至目标之间是**互相冲突**的：

| # | 层次 | 现状 | 目标 | 我的判断 |
| --- | --- | --- | --- | --- |
| D4.1 | **平台层**（窗口/输入/剪贴板/DPI/字体发现） | GLFW（经 raylib） | SDL3 | **赞成**。触点小（`nan_window` + `key_codes` + 一个解码器 + 一个常量），跨平台面更宽 |
| D4.2 | **图形 API** | OpenGL | Vulkan 级能力 | **赞成方向，但走 `SDL_GPU`**（Vulkan/Metal/D3D12/WebGPU 统一抽象），不自己写 Vulkan。现有 SDF shader 可移植 |
| D4.3 | **2D 几何引擎** | 自研 SDF 原语（已测） | ThorVG | **建议拆出去，倾向暂不引入**（理由见下） |
| D4.4 | **合成器集成**（dock / panel） | 无 | `wlr-layer-shell` | **必须独立立项**，而且它与 D4.1 的 SDL3 选择存在冲突 |

## 三个必须说清的矛盾

### (1) SDL3 与"Wayland 一等公民"是两条路，不是同一条

作者列的四个好处里，有两个 SDL3 **反而更远**，因为 SDL 把 Wayland surface 的所有权收走了：

- **显式同步 / 更低延迟 / dmabuf 零拷贝**：这些要靠拿到 `wl_surface` / swapchain 参与
  `linux-drm-syncobj` 与 dmabuf 交换。SDL 自己管理 presentation，不一定把这些暴露出来。
- **`wlr-layer-shell`**：dock / panel 必须是 layer-shell surface，而 SDL3 只创建
  xdg_toplevel / xdg_popup。**这是 D4.4 的硬阻塞**，也是"用本框架做 Hyprland 组件"这个目标的
  真正门槛 —— 它买不到，只能自己实现。

> 所以"SDL3 省力"与"成为桌面组件"是要**分期**的：近期用 SDL3 拿跨平台，
> 中期为 dock/panel 做一条 Wayland 专用后端（`wayland-client` + layer-shell + EGL/Vulkan），
> 复用同一个 `IRenderDevice`。它只跑 Linux，只在需要"成为桌面组件"时启用。

**需核实**：SDL3 是否允许从窗口取到 `wl_display` / `wl_surface`（历史上 SDL2 的
`SDL_SysWMinfo` 可以）并自行挂 layer-shell role；即便能取到，把 role 从 xdg_toplevel 换掉
也不是 SDL 支持的用法。这一条要**先验证再决定 D4.1 是否"单后端就够"**。

### (2) ThorVG 会引入第二套几何模型，而且买的是§7 里刻意推迟的能力

- 现在的能力是 **SDF 原语**：圆角矩形、描边、阴影、裁剪圆、圆弧，带 AA，已被
  `arc_painter_tests` / `render_tests` 等固定。ThorVG 是**矢量路径光栅器**（SVG 语义：
  路径、渐变、描边）—— 另一套几何模型。
- 两者并存 = 项目里同时有两套光栅化机制，正是这个项目反复吃亏的形状（同一个概念两处定义）。
  要避免它，就得让 ThorVG **替换** SDF 管线 —— 那等于把已经做对、已经测过的东西扔掉，
  并重新解决 AA / 圆角 / 阴影 / 裁剪的质量。
- 它真正买到的能力（**多边形 / 任意路径 / SVG 资源**）在
  [节点表现层 §7](node_presentation.md) 里是**明确的非目标**，理由是"用到再列"。
- 它**不解决**文本整形（已有 FreeType/HarfBuzz/FriBidi 管线）、**不解决**窗口/输入/合成器 ——
  对作者列的四个目标一个都不直接命中。
- 若将来确实需要路径：更小的步子是把路径细分成三角形喂给现有管线，或对路径做 SDF，
  而不是整包换引擎。

**判据**（什么情况下它才值得）：出现"必须渲染 SVG 图标/资源"或"必须路径渐变"的**具体 case**。
在那之前，"轻量、跨平台、不写 Vulkan"这三点都不由 ThorVG 提供 —— 它们由 D4.1/D4.2 提供。

> 说明：以上是**架构层面**的判断（第二套几何模型 + 买到的是推迟项），不依赖 ThorVG 内部细节。
> 它的具体能力（软件光栅/GL 引擎/文本 API 覆盖度）在真要引入时再核实。

### (3) 跨平台的真正阻碍大概率不是渲染器

raylib **本身就支持 Windows 与 Emscripten**。也就是说：如果目标是"能跑在 Windows / Web"，
用现在的后端就能试 —— 而且**应该先试**，因为它会把真正的阻碍暴露出来，而那些阻碍换后端也不会消失：

- 剪贴板（现在只有 `linux_clipboard`）；
- **系统字体发现**（Windows 注册表 / Web 无文件系统）；
- 资源后端（SQLite / 内存 / 目录）与可移植打包（`nanres` 流程）；
- 线程 / 文件系统 / 主循环（Emscripten 的 rAF 模型）；
- 构建系统（MSVC / mingw / emscripten 下的 Meson 与 subprojects）。

"换后端能拿到跨平台"这个因果链是**反的**：跨平台由这些平台适配层决定，渲染器只是其中一格。

## 建议（分期）

| 阶段 | 做什么 | 为什么这个顺序 |
| --- | --- | --- |
| **P0 能力探测**（小） | 用**现有 raylib 后端**各试一次 Windows 与 emscripten 构建，产出一张"真实阻碍清单" | 最便宜、信息量最大。它决定后续优先级，也可能改变结论（比如发现 Web 的阻碍无法接受） |
| **P1 第二后端**（中） | SDL3 + `SDL_GPU` 作为**并存**后端：复用 `IRenderDevice`，移植 SDF shader；与 raylib 跑同一套测试，通过后切默认 | 接口已经中立、已被 30 个替身验证；并存能把风险关在开关后面 |
| **P2 桌面组件**（中） | Wayland 专用后端 + `wlr-layer-shell`，实现 dock / panel | 它要的是**协议**能力，与 P1 的性质不同；不先做这个，Hyprland 组件永远只是"一个普通窗口" |
| **P3 矢量引擎**（待定） | ThorVG：等"路径/SVG 资源"有真实 case 再评估 | 见矛盾 (2)：现在引入是拿两套几何模型换一个非目标 |

## 待决问题

1. **Windows / Web 是"必须"还是"更好"？** 这决定 P0 的优先级，也决定是否值得为 Web 的
   限制妥协（例如放弃某些系统集成）。
2. **是否接受"两条后端"**（SDL3 跨平台 + Wayland 专用）？如果坚持单后端，就必须在
   "layer-shell 桌面组件"与"跨平台"之间做取舍 —— 这两个目标目前没有单解。
3. **SDL3 能否参与合成器**（拿到 `wl_surface`、显式同步、dmabuf）？需先核实；若不能，
   P2 必须自己写 platform 层，且 D4.1 的"省力"只对 Windows/Web 成立。
4. `SDL_GPU` 的成熟度与 shader 工具链是否可接受（版本、许可证、Wayland 后端质量）—— 需核实。
5. **键码迁移**：`widget/key_codes.hpp` 现在是 GLFW 值（raylib 直接透传），SDL3 的值不同。
   迁移要一次性映射并补测试（好在它已经收敛到单一定义处）。
6. ThorVG 的引入判据（见矛盾 2 的"判据"）—— 什么 case 会让你觉得"必须它"？
7. **dock/panel 的产品形态**：它是本框架的一个"应用"，还是一个独立的目标平台？
   这会决定 layer-shell 支持是进 `app` 层还是进新的 `platform` 模块。

## 与 D1 的交集（修正本文早先的"完全独立"说法）

D1 已定案，它的**第二阶段**（文本输入会话、组合输入、软键盘）依赖相应平台能力接通，
但不等于必须等待整个 D4 后端迁移完成：

| 事实 | 证据 |
| --- | --- |
| app 层没有文本编辑/组合事件通路 | `grep -rn "TextEditing\|IME\|composition" nandina/app/` 只命中 `runtime_error` 之类的假匹配 |
| `EditableText` 只有状态存储与操作接口 | `set_composition` / `clear_composition` / `composition()` — 没有平台来源 |
| 当前应用输入链路只转发已提交文本 | `NanWindow::poll_and_dispatch_input()` 消费 `GetCharPressed()`；这证明本项目的通路缺口，不直接证明第三方库所有扩展路径不可行 |

**结论（按本次审阅修正）**：未接通对应平台能力时，该能力的交付上限是**协议 + 模拟测试**；
能力可以来自现有后端扩展、独立平台适配或 D4 的新平台层，不能由“当前无通路”推导“只有换后端”。
正式依据见 [焦点与输入 §5](focus_and_input.md#5-文本输入会话与软键盘)：分别记录协议/模拟测试、
指定平台组合输入、指定平台软键盘请求/反馈/遮挡三个状态，不以一个“阶段二完成”代替真实验收。

---

# 四者之间的关系与排序建议

- **D1 已定案**（见 [focus_and_input.md](focus_and_input.md)），本文不再保留它的选项表。
  它留给 D2 的交集只剩一条：D1 的"用户始终显示焦点提示"偏好需要一个运行时更新通道，
  形状与 D2 相同（但偏好不是外观，两者不要合并成一条信号）。
- **D3 最独立**，纯作者 API 收紧，影响面局限在路由表与测试。
- **D4 独立定案，但与 D1 第二阶段的平台接入存在交集**；它不是 D1 第一阶段的前置，也不是文本输入能力的唯一来源。
  如果 P0 探测发现 Web/Windows 的阻碍无法接受，D4 的结论会变成"先把平台适配补完"。
- D2/D3/D4 之间的建议顺序：**D2 → D3 →（D4 的 P0 探测可随时并行）**。项目下一步仍是 D1 第一阶段，设计已定案、实现尚未开始。
  D2 有现成的使用场景在等，D3 是收尾；D4 的 P0 探测成本低、信息量大，适合插空先做。
- ⚠️ D1 第二阶段以相应平台能力验收为准，不写死依赖 D4/P1；D4 其他后端选型仍是待验证工作稿，
  本次仅修正交付依赖，不代表这些选型已获确认。

# 拍板之后

| 决定 | 归档到哪里 |
| --- | --- |
| D1 | ✅ 已归档为 [focus_and_input.md](focus_and_input.md)（并已同步 `component_contract.md` / `component_roadmap.md` / `project_status.md`） |
| D2 | `theme` 相关 reference（或 `design_tokens.md` 里的解析链路一节）+ stories/010 |
| D3 | `page_and_router.md` + stories/011 |
| D4 | 新增平台/后端 reference（或并入 `build_system_scope.md`）+ 一篇 story 记录取舍 |
| 本文 | 三件都拍板后删除；若中途放弃某项，把"为什么不做"补进对应 story |
