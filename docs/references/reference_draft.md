# 待决设计草案

> **这是设计工作稿。** 2026-10-20 同步：D7 的混合修复与生产 SDF（填充/描边/圆）已交付并在 Vulkan/DXIL 上验收，D8 方向已接受未实施；D2 仍待审阅，D3 方向已接受、未实施。
> 方向已接受不等于精确 API 已冻结；未定项在各批实现前定形。跨专题顺序以[项目进度与下一步](project_status.md)为准。
> 拍板之后：决定写进对应的 reference（`component_contract.md` / `theme` 相关 / `page_and_router.md`
> / `platform_backend_contract.md`），过程补充到 stories；本文随之删除或降级为历史草案。

2026-11-05 补充：D3 已复核并随 `095336b` 提交；D2 首批窄投影实现初稿待维护者复核。
当前契约与实施记录见本节 D3/D2 与 [Page / Router](page_and_router.md)。上面的日期行保留当时状态。
>
> 文件名沿用当前叫法；若想改成 `open_decisions.md` 这类更直白的名字，改索引一行即可。

各项设计彼此独立，可以分开拍板、分开实施：

| # | 待决事项 | 现状 | 阻塞谁 |
| --- | --- | --- | --- |
| ~~D1~~ | ~~焦点环什么时候显示~~ → **已定案** | 见 [统一输入、焦点与文本编辑](focus_and_input.md)；几何修复已落地 | — |
| **D2** | 主题变化**发不发信号** | 只能写不能订阅；外壳无法响应外观变化 | 外壳里一切"跟随主题"的东西 |
| **D3** | 路由元数据应保持多大开放性 | route identity 已由页面类型表达；导航分组改为开放、可选值 | 应用路由配置与可选导航分组呈现 |
| ~~D4~~ | ~~先跨平台、再扩展平台集成的后端路线~~ → **已归档** | 见 [平台与后端契约](platform_backend_contract.md)；P3 已把默认后端换成 SDL3 | — |
| **D7** | 后端替换后的**视觉质量一致性**（抗锯齿、半透明、圆角裁剪、图片解码器） | SDL 设备用 CPU 三角形扇、**无抗锯齿**，且几何绘制**未启用混合**；raylib 侧是 SDF + 解析 AA | 所有"细腻度"相关的观感结论；SDF 路线与着色器产物打包 |
| **D8** | **键码与输入码空间的归属** | P3.1a 每事件坐标/修饰键快照、P3.1b 物理/逻辑键身份与 P3.1c 跨类型保序已实施；D1 的文本会话、预编辑与密码用途已由 P3.3a/b1 接通，真实 IME/caret 区域归 P3.3b2 | 快捷键、编辑键、输入法边界；跨后端输入对照 |

**D5 已于 2026-10-11 冻结范围与目标契约，未实施**，不加入本稿的待决表：见
[资源构建、部署与运行时定位](resource_build_and_deployment.md)。解耦、作用域、身份独立交付，
不等待 D4 选型；跨平台定位与 Web 资源就绪协议仍在 D4/D5 交界按平台验收。

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

# D3 路由身份与导航分组：开放到哪一层

## 背景

本节最初把 `RouteOptions::type` 当成路由键，因而把“拼错后多出一组”列成需要框架修复的
类型安全问题。复核代码后应收窄这个问题：**它不是路由匹配身份，而是可见导航项的分组值**；
当前侧栏还直接把该值绘制为分组标题。

Nandina 的路由身份是 `PageKey`（页面类型）；`RouteOptions::address` 是静态显示地址，当前不做
path 匹配或深链接。应用可以自行把 UUID、manifest 常量、生成头文件、环境配置或其他值映射到
页面路由。路由库不应要求所有应用共享一套分组词汇，也不应将应用内部的拼写约定升级成框架白名单。

Angular 的 route config 把字符串 `path`、组件、`title` 和可选 `data` 作为不同字段；这可作为
“用户声明路由元数据”的参考，但不表示 Nandina 当前 `address` 已经具备 URL 匹配语义，也不要求
Nandina 移植 Angular 的路由模型。参见 [Angular: Define Routes](https://angular.dev/guide/routing/define-routes)。

## 实施前核实

| 事实 | 位置 |
| --- | --- |
| 页面类型 `PageKey` 是路由身份 | `RouteEntry::page_key`、`route<PageT>()`、`Routes::find()` |
| `RouteOptions::address` 不参与当前匹配 | `nan_router.hpp` 注释与 `Routes` 实现 |
| `std::string type{"default"}` 只用于导航分组 | `RouteOptions`、`Routes::nav_sections()` |
| 分组值同时成为当前 UI 分组标题，且当前无空值语义 | `NavSection::type`、`showcase/components/sidebar.cpp` |
| 分组顺序稳定，为首次出现顺序 | `Routes::nav_sections()` 与 `[app][router][nav]` 测试 |

## 已接受契约（2026-11-05 已实施、复核并随 `095336b` 提交）

- 不把分组改成封闭枚举，不做框架级已知名字白名单，也不因陌生分组值拒绝 route。
- 应用可以继续传任意字符串；其来源可以是 `constexpr`、用户自己的类型/UUID 序列化、manifest、
  生成的 `env.hpp` 或外部配置。是否拼写一致属于应用自己的配置治理。
- `RouteOptions::type` 改名为 `nav_group`，不保留兼容字段；空字符串表示未分组，非空值按完全相同的值合并。
- 暂不增加单独的标题字段：非空 `nav_group` 同时是分组身份和值标题。未来若真实需要 UUID/manifest key
  搭配本地化标题，再新增可选 `nav_title`，默认回退到 `nav_group`。
- 未分组 route 仍进入侧栏，不生成空标题；所有未分组 route 因空键相同而全局合并成一个无标题分组。
  该分组放在第一个未分组 route 的位置，后续未分组项加入该组；这是按值分组的既有语义，不是保留
  它们在原路由表中的交错位置。
- 非空分组仍按首次出现顺序排列，段内保留路由顺序；不增加独立排序表。
- `nav_sections()` 继续按值扫描，不需要 `"default"` 特判或组名校验。sidebar 在 `section.group.empty()`
  时跳过标题 Label；组与组之间的分隔线仍按 section 边界绘制，不能跟着标题一起消失。

## 实施与验收

本批按以下清单落地；实现与自动验证、维护者复核和视觉验收分别记状态：

1. `RouteOptions::type` → `nav_group`、`NavSection::type` → `group`；不留旧字段/别名。更新构建器、注释、
   测试、`showcase/main_window.cpp` 中包括注释掉的占位 route，以及 Story 011 / 页面路由文档。
2. 未设置与显式空串语义相同：route 仍可见、不画分组标题；非空值精确分组并作为标题。
3. 多个未分组 route，即使在声明表中被有名分组穿插，也合并成一个无标题 section，位置由首个未分组项决定。
4. 测试 section 分隔线仍只出现在 section 边界、不出现空标题或相邻重复分隔线。
5. 已有非空分组首次出现顺序和段内顺序保持不变；隐藏 route 不进入分组。

本轮暂不把“字符串拼写可能产生意外分组”视为库级 bug；如果应用想防 typo，可在自己的 constants、
manifest、route factory 或构建生成步骤中完成，而无需所有应用承担同一套机制。

### P4.3 D3 实施记录（2026-11-05）

- `RouteOptions::nav_group` 默认空，`NavSection::group` 为借用的组名视图；移除旧 type 字段。
  `Routes` 继续按可导航条目首次出现的精确组值分组，组名不参与 PageKey 或 address 匹配。
- 已迁移 Showcase 注册表/Sidebar、桌面页面 probe 和本地 playground bench；Sidebar 仅省略
  空组标题，section 之间的分隔线不依赖标题。未增加层间依赖或路由历史能力。
- `router` 回归覆盖默认/显式空组跨有名组全局合并、隐藏项不建立组顺序、需要参数的页面
  不进入导航，以及大小写/空白/`default`/应用自定义组名精确保留。
- 新增 `sidebar-showcase` unit 测试，直接构建真实 Sidebar；通过语义树检查标题、条目顺序和
  分隔线，并用语义 activate 与 typed navigate 验证无标题组不会改变页面身份。
- 专项：`router` 38 用例 / 393 断言，`sidebar-showcase` 4 用例 / 31 断言，均先编译再运行通过。
- 故障注入均重新编译对应目标后触发 Catch2 exit 42，恢复后两个专项全绿：
  1. 将 nav_group 默认值恢复为 `"default"`，省略/显式空串合并用例两处断言失败。
  2. 禁止空组加入已存在 section，交错空组合并用例断言失败。
  3. 无条件创建标题，三个真实 Sidebar 用例检测到多余空标题。
  4. 将组间分隔线限制为非空组，未分组段两侧分隔线用例失败。
- 首轮实现验证（每个配置先编译再测试；Linux，受限沙箱）：

  | 范围 | 结果 |
  | --- | --- |
  | buildDir 全量 | 115 OK / 2 FAIL / 1 SKIP；两个 FAIL 均为 SDL 无法访问显示设备的 SDF 测试，详见下文 |
  | buildDir-asan unit | 95 OK / 1 SKIP；ASan/UBSan 启用，构建与测试使用 `LSAN_OPTIONS=detect_leaks=0` |
  | buildDir-no-rtti unit | 96 OK / 1 SKIP |
  | buildDir-nosqlite unit | 92 OK / 1 SKIP |
  | buildDir-tools（build_ui=false） | 8 OK |
  | buildDir-probe 页面 probe | 重新配置与目标编译通过；未执行桌面渲染 |
  | release_check --ci | 8/8，无 SKIP；clang-format 23.1.1；新增测试文件另行 dry-run 通过 |

  SKIP 均为 window-lifecycle。主套件的 `sdf-device` / `sdf-state-probe` 在 SDL_Init 阶段
  报 `No available video device`；SDK 子项目打包和 vendored 下游消费者均通过。尝试沙箱外补验时，
  自动审批服务因模型不受支持返回 404，请求未执行，不能据此声称桌面/GPU 或视觉验收通过。
  构建时字体嵌入工具也需 `LSAN_OPTIONS=detect_leaks=0`，避免受限环境的 LSan ptrace 失败；
  ASan/UBSan unit 已完成，未发现该配置下的内存或未定义行为错误。审批服务恢复后，
  桌面会话补跑 `meson compile -C buildDir && meson test -C buildDir sdf-device sdf-state-probe
  window-lifecycle --print-errorlogs --logbase d3-desktop`；在真实成功前保留上述限制。
- Windows、桌面/GPU 补验和人工视觉验收尚未执行。P4.2c 与 D3 已随维护者统一提交 `095336b`
  推送；D2 首批窄投影为此后新开的子批，方案身份/选择与作者颜色入口仍是后续独立子批。

### 维护者独立复核（2026-11-05）

以下结果来自维护者提供的独立复核报告，不作为本次文档同步重新执行的测试：逐条核对 API
迁移、分组/过滤/顺序、真实 Sidebar 标题与分隔线、语义激活和 typed navigation，未发现实现
缺陷；维护者复核期间未修改工作树。旧字段仅在历史记录与迁移说明中保留文字。

| 范围 | 维护者报告结果 |
| --- | --- |
| router / sidebar-showcase | 38 用例 / 393 断言、4 用例 / 31 断言，全绿 |
| buildDir 全量 | 116 OK / 1 FAIL / 1 SKIP；sdf-state-probe 因无显示设备/GPU 失败，sdf-device 为 skip 77 |
| buildDir-asan unit | 95 OK / 1 SKIP（window-lifecycle） |
| buildDir-no-rtti unit | 97 OK；本机 window-lifecycle 未跳过 |
| buildDir-nosqlite unit | 92 OK / 1 SKIP（window-lifecycle） |
| buildDir-tools | 8 OK |
| buildDir-probe | nandina_sdl_page_probe 已是最新，当前路由字段迁移可编译 |
| release_check --ci | 8/8；clang-format 23.1.1，docs 87 篇；独立 header/suite/format 门禁通过 |

两轮 SDF 失败/跳过与 window-lifecycle 的差异来自验证环境，分别保留实际结果；均不代表
桌面/GPU 或视觉验收已经完成。probe 的 Label 路由仍使用历史基准元数据：`form/label` / `表单`，
Showcase 使用 `content_and_display/label` / `内容与展示`。复核后的跟进仅修正“逐字一致”的
不实注释，未改变路由值、渲染行为或 golden；今后若对齐值，须单独审阅并重新批准基准。

P4.2c 与 D3 已随维护者统一提交 `095336b` 推送；D2 首批窄投影另起子批（见下）。

---

### P4.3 D2 首批窄投影实施记录（2026-11-05）

按 [主题方案、作者入口与状态订阅](theme_authoring_and_state.md) §5.5 与 §8 第 1–2 步交付，
不等待方案身份/选择解析/作者颜色入口定形：

- 新增 `theme/theme_presentation.hpp/.cpp`：`ThemePresentationState { ColorAppearance appearance;
  std::uint64_t revision; }` 为 §5.5 两项窄载荷；`ThemePresentation` 是只读投影，由 `ThemeManager`
  的观察者回调驱动，内部 `Signal` 桥接 reactive（theme → reactive 为既有合法向下依赖）。
- 读取参与依赖追踪（`get()`/`appearance()`/`revision()`），`readonly()` 返回 `ReadSignal`
  供子组件消费，`subscribe(handler)` 返回 RAII `Subscription`；订阅本身不伪造初始变化。
- 等价状态去重：观察值未变化不通知；`revision` 保留“不透明呈现修订”语义（动效变化也前进）。
- 生命周期：`ThemeManager` 先销毁 → 投影 `valid()` 转 false，保留最后发布载荷、后续安全；
  作用域先销毁 → 投影注销观察者，之后变化不再回调。任何一端先销毁都不悬垂访问。
- 作用域归属：`ReactiveScope::own<T>()` 新增“作用域拥有的非响应式服务”（按类型唯一，
  在 effects/computeds 之后、signals 之前销毁）；`BuildContext::theme_state()` 通过页面
  回调作用域暴露同一实例，派生区域共享。
- Showcase 侧边栏亮暗开关改为跟随投影：外部外观变化（系统外观、其他 `set_preference`）
  经 `computed` 回写 `checked`，`Switch::set_checked` 静默写入，不会冒充用户 `on_change`。

- 首版专项（维护者提供）：`theme-presentation` 10 用例 / 60 断言；`sidebar-showcase` 5 用例 / 41 断言
  （新增外部外观变化跟随用例）。
- 故障注入（重新编译后触发 Catch2 失败，恢复后全绿）：
  1. 删除观察者注册 → 投影不再跟随，`theme-presentation` 9 个用例失败（20 断言）。
  2. 删除侧边栏 `checked` 绑定 → 外部变化不再回写开关，新增侧边栏用例失败。
  3. `ReactiveScope` 清理时不释放作用域服务 → 作用域销毁后仍回调，生命周期用例失败。
- 首版发布点审计覆盖 `apply`/具名 `activate`/家族/系统偏好/动效入口；全快照族“同指针
  只改 appearance”路径正常发布。复核发现具名同快照切外观与活动家族替换仍有遗漏，补修见下。
- 未包含：方案身份/目录/选择状态、纯主题内容事件、`ThemeObserver` 退役、作者颜色入口、
  颜色合成与校验（§8 第 3–5 步）；选择与身份待维护者定形后再扩展载荷。
- 首版验证矩阵（维护者提供，Linux 无显示/GPU 沙箱）：buildDir 全量 117 OK / 1 FAIL（sdf-state-probe，
  无显示设备）/ 1 SKIP（sdf-device 77）；asan 96 OK/1 skip（window-lifecycle）；no-RTTI 98 OK；
  no-SQLite 93 OK/1 skip；tools 8 OK；release_check --ci 8/8（clang-format 23.1.1；headers 193；
  docs 87 篇含新增锚点）。
- 本轮保持未提交，等待维护者复核；D2 其余步骤与作者颜色入口另行排期。

### D2 首批复核补修（2026-11-06，未提交）

基线先编译并通过 `theme-presentation` / `sidebar-showcase` / `theme-switch`。随后新增边界用例，
首轮五个非生命周期用例全部失败（32 断言中 16 失败），确认以下实现缺口：

- 具名主题沿用同一快照时，外观已改变但 `commit` 指针短路，theme view 与投影未发布。
- 活动 legacy 家族替换在移走 name 后才比较，未同步新映射；全快照家族替换仅刷新旧内容，
  未提交新快照。两条路径均用独立 tokens 数值证明实际内容替换与 revision 更新。
- 订阅重入可在同一次通知中再次改主题，载荷与 manager 事实不一致；订阅抛错会阻断后续
  订阅和其他页面投影。内部广播/多个投影写入未分阶段，effect 可看到旧场景样式或另一投影旧值。

补修使用独立观察登记身份和四段通知：内部广播 → 全部投影写入 → effect 提交 → 外部订阅。
manager 不依赖 Graph；低层 `ThemeObserver` 增加有默认实现的 `prepare_theme_presentation`
阶段，返回只拥有临时提交/通知任务的 `ThemeObserverUpdate`。投影在任务中保留内部状态，
scope 清理仅断开后续通知；支持通知期间 scope 或 manager 销毁。完整协议见
[D2 §11](theme_authoring_and_state.md#11-首批实施记录p43-d2-窄投影2026-11-05)。

故障注入（每项重新编译，结束后恢复源码）：

1. 恢复具名同快照短路：专项 exit 42，3 处断言失败。
2. 去掉发布期间的写入守卫：重入专项 exit 42，3 处断言失败。
3. 提前提交 effect 延迟令牌：多投影/场景一致性专项 exit 42，2 处断言失败。
4. 把通知任务的内部状态强引用改为裸指针：清理 scope 用例触发 ASan heap-use-after-free
   （`Event::emit` 的继续派发路径），exit 1；恢复保活后 ASan 专项通过。
5. 新增等价家族注册/同生效快照切家族/只改未生效暗色映射的回归，修复前 exit 42，5 处
   断言失败。外观变更只在生产者实际改变 appearance 且快照提交没有发布时补发，保留普通
   同快照 commit 的幂等性；系统外观与显式偏好两条入口均覆盖。

另补 `ReactiveScope::own<T>()` 的类型/代次唯一性、构造失败后重试、服务清理早于 signals 且
晚于 scope reactors 的独立回归；只读视图追踪、manager 析构中移除观察者、同地址重新登记、
异常隔离、全部写入口守卫与外层窗口 deferred effects 均有专项覆盖。Sidebar 补充系统驱动切换
保持 preference=system 的断言，避免把程序同步冒充用户事件。

第 3 步身份/选择 API 的具体建议定形建议见 [D2 §12](theme_authoring_and_state.md#12-第-3-步-api-建议2026-11-06-定形实现见-13)，
本批未实施下一步。

最终代码树验证（2026-11-06，每套先编译再运行，测试使用 `--no-rebuild`；未改写门禁）：

| 范围 | 本轮实际结果 |
| --- | --- |
| theme-presentation | 26 用例 / 163 断言，通过 |
| reactive（含独立 own 回归） | 44 用例 / 193 断言，通过 |
| sidebar-showcase | 5 用例 / 45 断言，通过 |
| buildDir 全量 | 116 OK / 2 FAIL / 1 SKIP；两个 FAIL 均为 SDL_Init 无显示设备的 sdf-device / sdf-state-probe |
| buildDir-asan unit | 96 OK / 1 SKIP；构建与测试均设 LSAN_OPTIONS=detect_leaks=0，ASan/UBSan 启用 |
| buildDir-no-rtti unit | 97 OK / 1 SKIP |
| buildDir-nosqlite unit | 93 OK / 1 SKIP |
| buildDir-tools（build_ui=false） | 8 OK；后续等价发布补修不涉及此构建闭包 |
| buildDir-probe | 最新投影/Sidebar 的 nandina_sdl_page_probe 编译通过，未运行桌面渲染 |
| release_check --ci | 8/8，无 SKIP；新增未跟踪的头/源码/测试另外使用 clang-format 23.1.1 dry-run |

三套 unit 的 SKIP 与主套件的 SKIP 均为 window-lifecycle；与维护者先前在其他环境中的
97/98 项结果差异分别保留，不把跳过写成通过。主套件的 SDK 子项目打包与 vendored 下游消费
均通过。Windows、桌面/GPU 与人工视觉验收仍未执行，不由此矩阵代替。

复核发现的行为缺口已补修并验证；保留未提交供维护者复验本轮新增通知协议和边界测试。
复验后可按原约定统一提交本批，不将首版与补修拆开；本轮没有 commit/push。

### P4.3 D2 第 3 步方案目录与选择实施记录（2026-11-06，初稿待维护者复核）

维护者起草后由复核补齐：`ThemeSchemeId`/`ThemeFamilyId` 规范身份与 typed parse、
`ThemeSchemeContent` + `ThemeSchemeCatalog` 值目录、`ThemeSelection`（fixed/appearance +
匿名来源）与失败原子提交、legacy 写入口移除及仓库/测试迁移。补齐内容：

- ID 空段 offset 语义统一（指向空段起始分隔符，`theme_scheme_tests` 断言修正）。
- `theme_test::Manager`（测试兼容层）复用 `require_idle` 通知守卫；active_name/
  active_family/preference 从目录与选择推断（style_document 编译 ID 解码）。
- `StyleDocument::apply` 把文件家族注册进目录并标记 scheme 归属，使选择后的家族查询
  可解码回文件级名字。
- 通知协议（分阶段、拒绝重入、完成必要通知后重抛首个异常）与窄投影在迁移后保持通过。

- 维护者补齐后首版专项：`theme-scheme` 9 用例 / 84 断言；`button` 33 / 221；`theme-presentation` 26 / 163；
  `sidebar-showcase` 5 / 45；`theme-switch` 2 / 24。
- 维护者补齐后首版矩阵（提供的独立记录，非本轮重跑）：buildDir 全量 118 OK / 1 FAIL（sdf-state-probe，
  无显示设备）/ 1 SKIP（sdf-device 77）；asan 97 OK / 1 skip；no-RTTI 99 OK；
  no-SQLite 94 OK / 1 skip；tools 8 OK；release_check --ci 8/8（clang-format 23.1.1；
  headers 194；docs 87）。
- 本轮保持未提交，等待维护者复验新增补修；作者颜色入口与第 4/4a 步仍待后续。

#### 第 3 步再次复核（2026-11-06）

先编译并通过 theme-scheme/theme-presentation/theme-switch/sidebar-showcase/theme-families/
theme-design-system 六个专项基线。保留维护者的 ID 空段 offset=17、兼容查询与通知守卫补充；
新增生产 ThemeManager 直接驱动的回归，不把测试兼容层代替新 API 验证。

补修的失败证据：

1. 长文件别名转换、首次导入完整通知、非激活家族校验/共享主题归属三组用例，补修前全红：
   34 断言中 11 失败（含 .value() 抛 bad_expected_access）。现在暂存目录与选择并同 style
   一次提交；所有家族登记，家族成员 ID 稳定且各自独立，共享不可变配色内容。
2. 替换与已选外观槽位冲突的错误分类曾返回 invalid_id；新增断言先失败，改为明确的
   appearance_mismatch 后通过。
3. 固定/匿名方案下点击 Sidebar 开关，主题不变而 checked 改变：新增用例 1 处断言先失败。
   现在保留固定方案并恢复实际外观，不从家族猜测备用槽位。
4. 故障注入均重新编译后 exit 42：关闭 255 字节上限（1 断言失败）、关闭已选项移除保护
   （2 断言失败）、删除 identity 对呈现变化的判定（1 断言失败）；恢复后专项全绿。

生产 API 覆盖还包括：跨家族亮暗配对、独立暗色固定选择、匿名身份、目录复制与销毁、
未生效槽位替换保持快照、同 ID 新内容发布/同快照幂等、同内容不同 ID 发布、失败原子性、
全部 canonical 写入口重入守卫、异常后交付和通知中销毁。

专项最终：theme-scheme 19 用例 / 230 断言，button 33 / 221，theme-presentation 26 / 163，
sidebar-showcase 6 / 55，theme-switch 2 / 24。

本轮最终树验证（每套先编译再测，meson test 使用 --no-rebuild）：

| 范围 | 实际结果 |
| --- | --- |
| buildDir 全量 | 117 OK / 2 FAIL / 1 SKIP；两个 FAIL 为 sdf-device/sdf-state-probe，SDL_Init 报 No available video device |
| buildDir-asan unit | 97 OK / 1 SKIP；ASan/UBSan 开启，构建与测试设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 98 OK / 1 SKIP |
| buildDir-nosqlite unit | 94 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| headers / suites | 194 headers classified（12 internal）；suite 门禁通过 |
| release_check --ci | 8/8，无 SKIP，clang-format 23.1.1；未跟踪的新头/源码/测试另行 dry-run 通过 |
| buildDir-probe | nandina_sdl_page_probe 与本地 playground 均编译成功；未执行窗口或 GPU 渲染 |

本轮 unit 和主套件的 SKIP 均为 window-lifecycle；与维护者环境 sdf-device=skip/no-RTTI
窗口测试通过的差异分别保留。SDK 子项目双次打包与 vendored 下游消费者通过。本地未跟踪
playground 编译只作为迁移检查，不列为 CI 证明。Windows、GPU/桌面与人工视觉验收仍待补齐。

已同步主题系统当前说明与索引，避免文档继续推荐移除的 apply/activate_family 等入口。
全部保持未提交，待维护者复验后统一提交首版与补修；未启动第 4/4a 步。

### P4.3 D2 第 4 步观察通道与作者入口实施记录（2026-11-07，初稿待维护者复核）

按 §8 第 4 步与 §14 定形实施；第 3 步批次已随 `995e6ae` 提交推送。

- **观察通道**：新增 `theme/theme_observation.{hpp,cpp}`，提供选择投影
  `ThemeSelectionPresentation`（`ThemeSelectionState = ThemeSelection`）、目录投影
  `ThemeCatalogPresentation`（`ThemeCatalogState {version, schemes, families}`）与明确变化事件
  `ThemeChangeHub`（`ThemeChange {before, after}`，端点为可选 scheme ID + 外观 + 有效
  DesignSystem 快照）。三者沿用四段通知、拒绝重入、回调异常隔离与双端销毁安全。
- **发布分类**：有效内容/身份/外观/style/动效变化仍走 revision 发布并广播组件；仅选择或
  仅目录变化走 staged-only 发布，不递增 revision、不让组件失效。目录事件以单调
  `ThemeSchemeCatalog::version()`（仅目录项增删改推进）为依据。
- **作者入口**：`derive_content`（深拷贝）、`derive_scheme`（独立内容、继承外观/家族）、
  `validate_scheme`（结构校验：ID/显示名/内容/冲突/家族）、`ThemeSchemeError`；
  `ThemeSchemeCatalog::contains_family`。注册入口与文件导入共用目录/选择提交语义。
- **作用域便利**：`ReactiveScope::keep(Subscription)`；`BuildContext::theme_selection()/
  theme_catalog()/theme_changes()/on_theme_change()`；文件导入范围写入 `design_tokens.md`。
- **内置主题迁移**：目录级回归确认 `nandina:<family>/light|dark` 规范方案与家族存在、
  两个外观内容独立、可固定/按外观选择，重复注册幂等。

故障注入（重新编译后恢复，均红转绿）：关闭 staged-only 发布（2 断言）、目录版本不推进
（1 断言）、变化事件去掉端点去重（3 断言）、派生共享内容指针（1 断言）、结构校验不拒空
内容（1 断言）。

首版验证矩阵（维护者提供的独立记录，Linux 无显示/GPU 沙箱；非本轮复核重跑）：

| 范围 | 实际结果 |
| --- | --- |
| theme-scheme / theme-observation | 23 用例 / 333 断言、6 用例 / 124 断言 |
| theme-presentation / families / design-system | 26 / 163、9 / 69、25 / 637 |
| buildDir 全量 | 119 OK / 1 FAIL / 1 SKIP；FAIL 为 sdf-state-probe（无显示设备），sdf-device skip 77 |
| buildDir-asan unit | 98 OK / 1 SKIP（window-lifecycle） |
| buildDir-no-rtti unit | 100 OK（本机窗口测试未跳过） |
| buildDir-nosqlite unit | 95 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8；headers 195；docs 87；clang-format 23.1.1 |
| probe | buildDir-probe 编译通过；未执行窗口/GPU |

未提交、未推送；未启动 4a（合成器/对比度/色域与真实后端读回）与内部广播收窄。
Windows、GPU/桌面与人工视觉验收仍待补齐。

#### 第 4 步复核记录（2026-11-07，保持未提交）

Git 历史确认 `995e6ae` 已在 HEAD 与 origin/main，第 3 步已统一提交；当前第 4 步首版为
19 个已跟踪修改 + 3 个新增文件。先编译并通过 theme-observation/theme-scheme/
theme-presentation 三个专项基线，未发现 RED-FIRST 残留。

发现并补修：等价 replace_scheme 仍递增目录 version，产生重复订阅与 effect；新增回归
先有 8 处断言失败，再在 ID/显示名/外观/家族/内容指针均相等时幂等返回。真实元数据或
内容快照变更继续通知，不深比较配方。顺带将目录版本判断前移，避免无目录变化时复制载荷。

新增边界覆盖：

- 三个通道分别在回调中清理 scope、销毁 manager、断开其他订阅和写入重入；后续订阅
  被失效时跳过，异常完成其他交付后重抛。ASan/UBSan 由完整矩阵复验。
- 真实 SceneTree 计数守住仅选择/目录变化不广播，实际呈现变更只广播一次。
- selection readonly/catalog computed 依赖追踪，多通道 effect 和 notify 一致性，文件导入
  一次提交目录+选择+主题，以及窗口外层 effect 延迟不被投影提前提交。
- ThemeChange 的 style/匿名来源端点、motion-only 去重、销毁后端点快照所有权；
  页面 clear 后用真实生效变更证明 on_theme_change 的连接已注销。
- 作者派生的嵌套配方隔离、继承家族与已注册派生不被基础替换影响，invalid_id 诊断，
  三个新通道没有写入/发布权限的编译期检查。

独立故障注入（对应目标每次重编译；恢复后三专项全绿）：

1. 去掉等价替换短路 → 8 断言失败，Catch2 exit 42。
2. 选择/目录投影提前提交 deferred token → 文件导入全通道一致性 10 断言失败，exit 42。
3. 订阅守卫只保留 weak.lock、忽略 connected/manager → scope clear 三通道测试
   6 断言失败，exit 42。

专项：theme-observation 16 用例 / 296 断言，theme-scheme 25 / 357，theme-presentation
26 / 163。

本轮最终树验证（每套先编译，再 meson test --no-rebuild）：

| 范围 | 本轮实际结果 |
| --- | --- |
| theme-observation / theme-scheme | 16 用例 / 296 断言、25 用例 / 357 断言，全绿 |
| theme-presentation | 26 用例 / 163 断言，通过 |
| buildDir 全量 | 118 OK / 2 FAIL / 1 SKIP；sdf-device 与 sdf-state-probe 均在 SDL_Init 报 No available video device |
| buildDir-asan unit | 98 OK / 1 SKIP；启用 ASan/UBSan，构建与测试设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 99 OK / 1 SKIP |
| buildDir-nosqlite unit | 95 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 195，docs 87，clang-format 23.1.1 |
| buildDir-probe | nandina_sdl_page_probe 编译通过，未执行窗口/GPU 渲染 |

本轮四套的 SKIP 均为 window-lifecycle；与首版维护者环境 sdf-device=skip、no-RTTI
窗口测试通过的差异分别保留。SDK 子项目双次打包与 vendored 下游消费者均通过。
未跟踪的新观察头/源码/测试额外执行 clang-format 23.1.1 dry-run，文档收尾另查链接与空白。

复核缺口已补修并验证，待维护者复验后统一提交首版与补修；本轮未 commit/push，
未启动 4a 或作者颜色入口。Windows、桌面/GPU、真实像素读回和人工视觉验收仍待补齐。

### P4.3 D2 第 4a 步 CPU 侧实施记录（2026-11-07，随后已随 `fef829a` 提交）

第 4 步已随 `4746173` 提交推送。本批按 §6.5–§6.7 落地 CPU 侧，真实后端读回与注册门槛
接线仍待有 GPU 的环境（§6.8 步骤 1–5 未执行）。

- **foundation 唯一合成器**：`foundation/composition.hpp` 显式
  `CompositeSpace::encoded_srgb` 策略，`nan_composite_over`（不透明背景 source-over）、
  `nan_composite_background_stack`（由近及远、首个不透明终点提前终止）、
  `nan_composite_over_stack`；错误以 `CompositionError` 区分不支持空间/非有限颜色/
  无终点，不猜测底色。
- **theme 诊断**：`theme/contrast_diagnostics.{hpp,cpp}` 的 `ContrastRule`/`ContrastProfile`/
  `evaluate_contrast`/`ContrastDiagnostic`，显式规则（前景 + 背景栈 + 状态 + 阈值 +
  required 级别），输出方案/外观/规则/角色路径/实测比值/所需阈值/解析后 OKLCH 与转换后
  sRGB；`passed`/`failed`/`insufficient_context` 三态，必需失败与上下文不足分别查询。
- **首批档案**：15 条与组件默认配方对应的显式规则；不做隐式整表断言与风格建议。
- **helper 收敛**：两个既有测试的局部 `composite_over` 改为消费共享实现。

故障注入（重新编译后恢复）：线性空间合成（3 断言失败）、背景栈不提前终止（1 断言）、
上下文不足误报为通过（2 断言）。

最终验证矩阵（Linux，无显示/GPU 沙箱）：composition 6/35、theme-contrast-diagnostics
6/181、theme-palette-contrast 4/129、command-palette 15/105；buildDir 全量
121 OK / 1 FAIL（sdf-state-probe 无显示设备）/ 1 SKIP（sdf-device 77）；
ASan 100 OK / 1 skip；no-RTTI 102 OK；no-SQLite 97 OK / 1 skip；tools 8 OK；
release_check --ci 8/8（headers 197、docs 87、clang-format 23.1.1）。
当时保持未提交；本批随后已随 `fef829a` 提交推送。真实像素读回、注册门槛、
Windows/桌面/人工视觉验收当时仍待补齐。


#### 第 4a 步 CPU 复核记录（2026-11-07，随后已随 `fef829a` 提交）

HEAD 与 origin/main 同为 `4746173`，第 4 步已提交。先编译并通过 composition /
theme-contrast-diagnostics / theme-palette-contrast / command-palette 四个专项基线。

补修四处：tone 引用缺少上下文时不再静默采用 primary，规则和诊断显式携带 tone；
变换角色路径保留操作及操作数；解析/采样在首个不透明背景停止；诊断记录合成配置并声明
现行 linear RGB 通道裁剪、浮点 sRGB、无 u8 量化策略。尚未解析的前景为可选 sample。
新增测试先于补修运行，四类分别 6 / 2 / 2 / 3 断言失败（总计 13），日志：
`buildDir/meson-logs/step4a-review-red.txt`。临界值显示的初步疑点经实际测试排除，现行输出
能区分相邻 float；边界回归保留，未修改阈值或显示实现。

补充基础错误分支、近不透明 alpha、NaN alpha、四种 tone 的亮暗解析、背景/变换 rhs
缺少 tone 与遮挡终止、非法阈值和规范化 OKLCH / 裁剪后 sRGB 独立保留。合成器本体未改。
专项：composition 7 用例 / 52 断言，theme-contrast-diagnostics 15 / 338，
theme-palette-contrast 4 / 129，command-palette 15 / 105。

独立故障注入：分别去掉前景 tone 守卫、退化 with_alpha 角色名、取消 theme 采样终止、
删除报告中的合成空间标记，重编译后分别 5 / 1 / 2 / 1 断言失败（Catch2 exit 42）；
全部恢复后四个专项全绿（step4a-review-restored 日志）。

本轮最终代码树矩阵（每套先编译，再测试；主套件运行期间未修改工作树）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | 120 OK / 2 FAIL / 1 SKIP；sdf-device 与 sdf-state-probe 均在 SDL_Init 报 No available video device |
| buildDir-asan unit | 100 OK / 1 SKIP；ASan/UBSan 启用，构建与测试均设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 101 OK / 1 SKIP |
| buildDir-nosqlite unit | 97 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 197，docs 87，clang-format 23.1.1 |

四套窗口 SKIP 均为 window-lifecycle（exit 77）；与首版维护者环境 sdf-device 为 skip、
no-RTTI 窗口测试通过的差异分别保留，不冒充硬件验收。SDK 双次打包与 vendored 下游
消费者均通过。主日志为 `buildDir/meson-logs/step4a-review-full.txt`；三个变体目录的
`meson-logs/step4a-review-unit.txt` 及 tools 的 `step4a-review-tools.txt` 保留实际结果。
ASan 的本地 Ninja 构建记录恢复触发较大范围重编译，最终构建和单元验证均完成。

五个未跟踪 C++ 文件另做固定版本格式 dry-run；故障注入源码与恢复备份逐字一致，无
RED-FIRST 残留。文档结果收尾后复查 87 篇链接/锚点与 git diff --check。
当时保持 15 项已跟踪修改 + 5 个新增文件；随后首版与补修已统一提交并推送为 `fef829a`。
本批不启用注册配色门槛；后端像素读回、最终色域/量化定形、Windows、桌面/GPU 与人工
视觉验收在当时仍未完成。

### P4.3 D2 第 4b 步作者颜色入口实施记录（2026-11-07，初稿待维护者复核）

第 4a 步已随 `fef829a` 提交推送。本批按 [主题方案、作者入口与状态订阅](theme_authoring_and_state.md)
§17 落地 C++ 作者颜色入口，并与既有文件导入路径做相等性回归。

- **primitive 色阶访问**：`ReferenceColorFamily` + `reference_scale`（const/非 const）+
  `reference_color` + `reference_family_name` / `color_shade_name`，与
  `$palettes.<name>.<scale>.<shade>` 字符串寻址一一对应；`ThemeSchemeContent` 新增可选
  `reference` 来源记录（palette 仍是唯一权威语义事实）。
- **显式 `NanColor` 覆盖**：`SemanticColorOverride`/`ThemeColorError`、
  `canonical_color_token`（on_* / outline* / focus_ring → shadcn 规范名）、
  `semantic_color`、`set_semantic_color`（同步兼容别名）、`apply_semantic_colors`
  （先验证全部再一次性提交）。作者入口只拒绝非有限 OKLCH 分量：`NanColor` 构造已归一化
  light/hue/alpha 并归零负 chroma，入口不重复裁剪也不把色域处理混进写入。
- **构造入口**：`ThemeSchemeContent::from_reference`（色阶派生并记录来源）、
  `from_semantic_colors`（显式语义声明，不伪造来源）；`derive_content` 深拷贝并保留
  `reference`。
- **文件导入收敛**：TOML 直接色值等价于逐角色 `set_semantic_color`，`$palettes.*` 等价于
  `reference_color`；回归按全部规范角色比较，并验证 `StyleDocument::apply` 注册出的规范
  方案 palette 与 C++ 构造一致。

故障注入（重新编译后恢复，均红转绿）：别名不同步（1 断言）、批量边验证边写入破坏原子性
（1 断言）、neutral 族错映 primary（1 断言）、派生丢弃来源（1 断言）、别名不规范化
（2 断言）。

专项：theme-color-entry 7 用例 / 432 断言；theme-palette-contrast 4/129、
theme-contrast-diagnostics 15/338、theme-scheme 25/357 保持通过。

首版维护者矩阵（复核前；每个配置先编译再测试；Linux 无显示/GPU 沙箱）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | 122 OK / 1 FAIL / 1 SKIP；sdf-state-probe 在 SDL_Init 报无显示设备，sdf-device 为 skip 77 |
| buildDir-asan unit | 101 OK / 1 SKIP；ASan/UBSan 启用，LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 103 OK / 0 SKIP（本机窗口测试未跳过） |
| buildDir-nosqlite unit | 98 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 197，docs 87，clang-format 23.1.1 |

未提交、未推送，待维护者复核；Shell 多方案设置、注册配色门槛、后端像素读回与视觉验收
仍待补齐。


#### 第 4b 步复核记录（2026-11-07，保持未提交）

Git 基线 HEAD 与 origin/main 为 `fef829a`，确认 4a CPU 批次已提交。先编译并通过
四个专项基线。新增文件/C++ 非有限颜色一致性回归，原实现 5 断言失败：四类非有限
OKLCH 存储分量与 `$palettes` 非有限引用在 TOML 仍能解析成功。

补修 `style_document.cpp`：将选中的角色/颜色先收集为 `SemanticColorOverride`，复用
`apply_semantic_colors` 统一验证、同步别名并提交；TOML 规范名优先于别名的规则保留。
新入口不替作者另做裁剪，也不启用配色注册门槛。

新增边界：7 色族 × 11 档 const 访问及文件寻址、34 角色与既有组件颜色解析器一致、
14 别名双向消费及规范名优先、重复覆盖声明顺序、失败时兼容字段不变、空批次、来源复制
与语义色权威性。§17 删除不可达 alpha 错误项，修正实施记录编号并明确有效枚举前提。

独立故障注入：去掉文件批量校验的错误传播，重新编译后 5 断言失败（Catch2 exit 42）；
恢复后六个专项全绿，日志为 `step4b-review-restored`。最初红测记录为
`buildDir/meson-logs/step4b-review-red.txt`。

专项：theme-color-entry 14 用例 / 1243 断言，button 33 / 221，theme-scheme 25 / 357，
theme-design-system 25 / 637，theme-contrast-diagnostics 15 / 338，theme-palette-contrast
4 / 129。

本轮最终代码树矩阵（先编译再测试，完整套件期间工作树保持冻结）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | 121 OK / 2 FAIL / 1 SKIP；sdf-device 与 sdf-state-probe 均在 SDL_Init 报 No available video device |
| buildDir-asan unit | 101 OK / 1 SKIP；ASan/UBSan 启用，构建和测试均设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 102 OK / 1 SKIP |
| buildDir-nosqlite unit | 98 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 197，docs 87，clang-format 23.1.1 |

四套的 SKIP 均为 window-lifecycle（exit 77）。首版维护者环境 sdf-device 为 skip、
no-RTTI 窗口测试通过，本轮差异分别保留；SDL 初始化错误不作为配色代码失败，也不计为
硬件验收通过。SDK 双次打包与 vendored 下游消费者通过。
ASan 因本地 Ninja 记录恢复触发较大范围重编译，最终构建与单元验证均完成。

主日志为 `buildDir/meson-logs/step4b-review-full.txt`；变体目录使用
`meson-logs/step4b-review-unit.txt`，tools 使用 `step4b-review-tools.txt`。
四个 UI 配置的 theme-color-entry 均为 14 用例 / 1243 断言；恢复源码与备份逐字一致，
新测试文件另做固定版本格式 dry-run，文档结果收尾后复查 87 篇链接/锚点与空白。

当前为 15 项已跟踪修改 + 1 个新增测试文件，未提交、未推送，HEAD 仍为 `fef829a`。
待维护者复验后统一提交首版与补修；未启动 Shell 多方案设置、真实后端像素读回、注册
配色门槛、广播收窄或第三方配方注册；Windows 与人工视觉验收未完成。

### P4.3 D2 第 4c 步 foundation 命名颜色字面量实施记录（2026-11-08，初稿待维护者复核）

4b 已随 `3e2542a` 提交推送。本批按 [主题方案、作者入口与状态订阅](theme_authoring_and_state.md)
§18 落地 foundation 命名颜色字面量，TOML 简写继续单独排期。

- **范围快照**：CSS Color 4 候选推荐草案 2026-11-03 §6.1 的 148 个命名颜色
  （SVG 1.1 的 147 个 + `rebeccapurple`），逐条采用规范固定 sRGB 整数原值；
  `transparent` 是透明黑，`currentColor` 依赖上下文，均不属于本批不透明色值。
- **API**：`NanNamedColor`（148 项小写字母序）+ 类型化 `nan_named_color(NanNamedColor)`、
  字符串 `nan_named_color(std::string_view) -> optional`、规范名
  `nan_named_color_name`；公开头只有枚举与三个声明，唯一 `constexpr` 数据表在
  `.cpp` 内部，不给 `NanColor` 堆成员。别名各保留身份，枚举序号不承诺持久化稳定。
- **查询规则**：ASCII 大小写不敏感且只折叠 `A–Z`（不依赖 locale）；拒绝空串、
  首尾/内部空白、显式长度嵌入 NUL、非 ASCII 与未知名称；不支持完整 CSS 颜色语法。
- **定位**：命名颜色只是显式 `NanColor` 的一种写法，不是新运行时覆盖顺序；默认配方
  仍不得硬编码颜色，主题作者可把命名颜色赋给既有语义角色。

故障注入（重新编译后恢复）：`green` 误写成 `lime` 值（3 断言失败）、表缺
`rebeccapurple`（计数/往返多处失败）、查询放宽为 trim 空白（2 断言失败）。

专项：named-color 5 用例 / 1095 断言；既有四专项保持通过。

首版维护者矩阵（复核前，每个配置先编译再测试；Linux 无显示/GPU 沙箱）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | 123 OK / 1 FAIL / 1 SKIP；sdf-state-probe 在 SDL_Init 报无显示设备，sdf-device 为 skip 77 |
| buildDir-asan unit | 102 OK / 1 SKIP；ASan/UBSan 启用，LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 104 OK / 0 SKIP（本机窗口测试未跳过） |
| buildDir-nosqlite unit | 99 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 198，docs 87，clang-format 23.1.1 |

本批保持未提交，待维护者复核。


#### 第 4c 步复核记录（2026-11-08，保持未提交）

HEAD 与 origin/main 同为 `3e2542a`，确认 4b 已提交。先编译通过 named-color 和
作者颜色入口基线，再以独立离线参考清单对照生产表与测试期望表的 148 个名称/RGB，
未发现数值或查询实现缺陷。私有表恰为 148 个唯一名称，按字母序排列。该轮尚未取得
固定规范网页原文；随后已完成本地 SVG 原文与 CSS Color 4 固定快照核对，见下文补验记录。

补齐一处测试盲点：旧类型化测试只把整数强转成枚举，交换 `green`/`grey` 真实枚举名称后
仍有 5 用例 / 1095 断言全绿（`/tmp/nandina-step4c-enum-baseline.log`）。现独立期望表
直接引用所有枚举成员，去掉自查 contains，并验证名称与身份唯一、严格字母序和真正的
别名成员身份；同样交换后 6 断言失败。生产查询/转换实现保持原样，仅修正头文件说明。

补充全部色值回转 RGB 字节与 alpha、全表大小写、非 NUL 结尾 string_view 切片、每个
0–255 字节前后/内部插入均拒绝、长未知名和排除关键词。完整 named-color 为
6 用例 / 2916 断言；composition 7/52、theme-color-entry 14/1243、theme-scheme
25/357、theme-contrast-diagnostics 15/338、theme-palette-contrast 4/129 通过。

独立故障注入每次重编译，均 exit 42：

| 注入 | 失败断言 |
| --- | --- |
| green/grey 枚举名称互换 | 6 |
| green RGB 错用 lime | 3 |
| rebeccapurple 表名失配 | 1 |
| trim 首尾空白 | 2 |

源码恢复后六专项全绿，日志 `buildDir/meson-logs/step4c-review-restored.txt`。
本轮最终代码树矩阵（先编译再测试，完整套件运行期间未修改工作树）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | 122 OK / 2 FAIL / 1 SKIP；sdf-device 与 sdf-state-probe 均在 SDL_Init 报 No available video device |
| buildDir-asan unit | 102 OK / 1 SKIP；启用 ASan/UBSan，构建和测试均设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 103 OK / 1 SKIP |
| buildDir-nosqlite unit | 99 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 198，docs 87，clang-format 23.1.1 |

四个 UI 配置的 named-color 均为 6 用例 / 2916 断言；各套 SKIP 均为 window-lifecycle
（exit 77）。与首版维护者环境 sdf-device=skip、no-RTTI 窗口测试通过的差异分别保留。
SDK 双次打包与 vendored 下游消费者通过。ASan 本地 Ninja 记录恢复触发较大范围重编译，
最终构建与单元验证均完成。

主日志：`buildDir/meson-logs/step4c-review-full.txt`；变体目录为
`meson-logs/step4c-review-unit.txt`，tools 为 `step4c-review-tools.txt`。
`named_color.cpp` 与故障注入前备份逐字一致；头文件只澄清说明。三个未跟踪 C++ 文件
额外做固定版本格式 dry-run，文档收尾后重查链接/锚点与 git diff --check。

**规范来源复验边界（当时记录，已由下文补验关闭）**：该轮网页工具未返回固定日期规范页的可用正文；沙箱内只读抓取遇到
DNS 解析失败，沙箱外只读抓取因自动审批服务 `codex-auto-review` 不受支持而报 404，
动作未执行（不是 W3C 页面返回 404）。因此本轮仅确认离线固定清单、生产表与测试表
一致，当时固定日期规范原文的在线复核未完成；前述进度中的原文核对表述在此更正。
后续环境恢复后的实际原文核对见下文，不再将此前服务故障列为当前阻塞。

保持 12 项已跟踪修改 + 3 个新增文件，HEAD 仍为 `3e2542a`，未提交、未推送；
待维护者复验，不启动下一子批。TOML 简写、注册配色门槛、真实后端像素读回、Shell
设置页、广播收窄和 Windows/人工视觉验收仍在本批范围外。


#### 第 4c 步规范原文补验（2026-11-08，来源证据缺口已关闭；保持未提交）

维护者提供 `/tmp/vscode` 中的 W3C SVG 1.1 Second Edition HTML。实际保存内容为
SVG 1.1 §4.4，并非 CSS Color 4；从两张原文颜色表提取 147 个唯一名称与十进制 RGB，
逐项与生产表和测试期望清单比较，全部一致。该文档没有 rebeccapurple，不能单独证明
第 148 项或 CSS Color 4 快照版本。

环境恢复后，另外通过 HTTPS 只读取得约定的
[CSS Color 4 固定快照](https://www.w3.org/TR/2026/CRD-css-color-4-20261103/#named-colors)
（HTTP 200）。HTML 自报 CRD、日期 `2026-11-03`、§6.1 Named Colors 与对应 This version
地址均相符；解析原文 named-color-table 得到 148 项，逐项检查十六进制与十进制 RGB 一致。

| 比较 | 实测结果 |
| --- | --- |
| SVG 1.1 原文与 CSS Color 4 共有项 | 147 项名称/RGB 全同 |
| CSS Color 4 相对 SVG 1.1 的新增项 | 仅 rebeccapurple，#663399 |
| CSS Color 4 原文与生产私有表 | 148 项键值完全一致，无遗漏/多余/重复 |
| CSS Color 4 原文与独立测试清单 | 148 项键值完全一致 |
| CSS Color 4 原文与 NanNamedColor | 148 个成员名称齐全，顺序与规范名字母序一致 |

原文文件 SHA-256：

- SVG：`7d8a1f6beb02f70c6a46be491b3f875a1fd06a3cb18eb1b5bf51492e74f31eea`
- CSS Color 4：`b66ad2af540a7550fd34e0d95f8dce3f49b9d39e255fdfed56f0ec1de9c78000`

补验脚本：`/tmp/nandina-step4c-w3c-source-audit.py`，结构化结果：
`/tmp/nandina-step4c-w3c-source-audit.json`，CSS 快照副本：
`/tmp/nandina-step4c-css-color4-20261103.html`。脚本直接从两份 HTML 表格读取名称和值，
没有拿生产表或测试表反向生成期望。临时证据文件不作为仓库构建依赖。

**更新结论**：规范原文来源核对通过，先前离线验证的证据边界已补齐；运行实现无需修改。
本次仅补充证据与文档，未重跑无变化的 C++ 矩阵；前述主套件/变体/门禁结果保留原日期和
环境差异。文档链接/锚点与 git diff --check 重新检查。HEAD 仍为 `3e2542a`，整批未提交。

### P4.3 D2 第 4d 步 TOML 命名颜色字面量实施记录（2026-11-08，复核完成并随本批提交）

4c 已随 `f383201` 提交推送；状态文档同步在本批一并完成。按
[主题方案、作者入口与状态订阅](theme_authoring_and_state.md) §19 把命名颜色接到文件导入侧，
范围保持「增加一种字面量写法，保留既有引用能力」。

- 颜色字面量字段（palette 覆盖、参考色阶档位、Button/TextField 规则颜色）接受
  `"SteelBlue"`，统一经 `foundation::nan_named_color`；大小写/空白/NUL/非 ASCII/未知名称
  规则与 C++ 入口一致。
- `$palette.*` 继续语义 token、`$palettes.*` 继续参考色阶取值；匹配失败立即报错、
  不回退成色名；不扩展其他引用形式。规范名优先与 C++ 批量覆盖顺序不变。
- 错误信息带字段路径（`themes.author.palette.primary`、`palettes.brand.primary.50`）；
  解析失败不产生可 apply 文档，不修改活动目录/选择/样式。

故障注入（重新编译后恢复）：palette 覆盖拒绝命名颜色（3 断言失败）、配方字段拒绝命名
颜色（1）、错误丢失字段路径（2）、别名优先破坏规范名优先（1）。

专项：style-document-named-color 5 用例 / 50 断言；named-color 6/2916、
theme-color-entry 14/1243 等既有专项保持通过。

首版维护者矩阵（复核前，Linux 无显示/GPU 沙箱；每套先编译再测）：buildDir 全量
124 OK / 1 FAIL（sdf-state-probe，无显示设备）/ 1 SKIP（sdf-device 77）；
ASan 103 OK / 1 SKIP；no-RTTI 105 OK / 0 SKIP；no-SQLite 100 OK / 1 SKIP；
tools 8 OK；release_check --ci 8/8（headers 198、docs 87、clang-format 23.1.1）。
本批保持未提交，待维护者复核；随后按约定转向 Shell 多方案使用闭环。


#### 第 4d 步复核记录（2026-11-08，补修后随本批提交）

HEAD 与 origin/main 同为 `f383201`，先编译通过文件色名、foundation 色名、作者颜色入口、
Button 四专项。字面量/引用分派未见行为缺陷；新增错误定位回归先有 5 断言失败：
配方错误丢失规则及下标（2），palette 非有限输入（规范名/别名/引用值）丢失主题路径（3）。
现在配方色错误携带 0 起始的 `styles.button[1].foreground` / `styles.text_field[1].placeholder`
路径，批量校验错误携带完整输入路径，保留别名拼写。未增加引用形式、注册配色门槛或裁剪。

新增边界：148 色在四种位置与 C++ 同值、10 个配方槽位的命名值/数组/token 及 token 延迟
解析、四种位置的空白/NUL/非 ASCII/保留名/错误类型拒绝、引用的上下文范围与畸形分段。
原失败状态检查在 parse 失败后才创建 manager，现替换为实际 parse→apply 导入路径；
先激活具名色方案，再触发 parse 与两种 apply 错误，检查目录内容/版本、选择、revision、
样式和生效快照身份不变。

独立故障注入每项重编译：Button foreground 丢路径 1 断言失败，palette 校验丢路径
3 断言失败，token 提前烘焙 1 断言失败（Catch2 exit 42）；源码恢复后六专项全绿。
初始红测日志 `buildDir/meson-logs/step4d-review-red.txt`，恢复日志
`buildDir/meson-logs/step4d-review-restored.txt`。

专项：style-document-named-color 12 用例 / 1895 断言，named-color 6/2916、
theme-color-entry 14/1243、button 33/221、theme-scheme 25/357、theme-contrast-diagnostics
15/338。本轮最终代码树矩阵（每套先编译再测试，全量套件运行期间工作树冻结）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | 126 OK / 0 FAIL / 0 SKIP |
| buildDir-asan unit | 103 OK / 1 SKIP；ASan/UBSan 启用，构建和测试均设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 105 OK |
| buildDir-nosqlite unit | 100 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 198，docs 87，clang-format 23.1.1 |

本轮运行环境与首版维护者无显示设备的记录不同：现有 sdf-device、sdf-state-probe 和主
配置 window-lifecycle 均真实运行、exit 0。sdf-state-probe 记录 Vulkan，设备为 AMD
Radeon RX 6750 GRE 12GB（RADV NAVI22），SDL=3004018；三档缩放、参数状态交错及
故障注入分支通过。此证据仅属于既有 SDF 验收，不自动证明 4a 共享合成器与软件/GPU
路径的专项读回一致，也不代表页面级人工视觉验收完成。ASan/no-SQLite 的 SKIP 均为
各自配置下的 window-lifecycle（exit 77），no-RTTI 窗口检查实际通过。

SDK 双次打包和 vendored 下游消费者通过。四个 UI 配置的文件色名专项均为
12 用例 / 1895 断言。ASan 本地 Ninja 记录恢复触发较大范围重编译，最终编译和测试完成。
主日志 `buildDir/meson-logs/step4d-review-full.txt`，变体目录
`meson-logs/step4d-review-unit.txt`，tools 使用 `step4d-review-tools.txt`。

故障注入源码恢复后与备份逐字一致；新测试另做固定版本格式 dry-run，代码无
RED-FIRST 残留，文档收尾再检查 87 篇链接/锚点与 git diff --check。
复核时基线为 `f383201`，共 11 项已跟踪修改 + 1 个新增测试文件；复核完成后随本批统一
提交推送。未启动 Shell 或软件读回批次。

后续顺序沿用维护者约定：本批复验/统一提交后转 Shell 多方案闭环，页面由维护者编写；
原始计划曾将 4a 软件路径读回放在 Shell 之后；实际已另批先行完成软件与
Linux/Hyprland+RADV 离屏 GPU 读回，但仍不代替默认 swapchain 或 Windows 路径，
也不挂接注册门槛。Windows、GPU 和人工视觉验收仍单独记录。

### P4.3 D2 Shell 多方案框架准备实施记录（2026-11-08，已随 `f821f40` 提交）

4d 已随 `5061b39` 提交推送。本批只处理设置页依赖的框架准备，不修改 showcase 页面正文，
也不新增 `ThemeSettingsState`；页面仍以 manager 为唯一写入口，从 selection/catalog/
presentation/change 四个现有通道读取状态。

- **目录错误主体**：`ThemeCatalogErrorKind` 改为 operation-specific。错误携带可选
  `ThemeSchemeId scheme` 与 `ThemeFamilyId family`；方案引用未知家族时两者同时存在。
  `scheme_in_use` 与 `family_in_use` 分别表示当前选择引用方案、家族仍含方案。
  `describe()` 输出真实主体，不再把 family ID 强转成 scheme ID。
- **BuildContext 便利入口**：新增 `on_theme_selection_change()` 与
  `on_theme_catalog_change()`。它们与 `on_theme_change()` 一样将 RAII subscription 放入
  页面回调作用域；初始值仍通过投影 `get()` 读取。
- **Shell 组合回归**：新增 `tests/theme_shell_flow_tests.cpp`，按设置页动作顺序验证
  fixed/pair/system 切换、未生效槽编辑、同外观换方案、当前/非当前替换、双页面外部同步、
  scope 解绑、动态目录、typed failure 和失败状态不变。

独立故障注入（每次重编译，恢复后六专项全绿）：

1. selection 便利入口不保存 subscription → 生命周期用例 2 断言失败。
2. catalog 便利入口不保存 subscription → 生命周期用例 3 断言失败。
3. family-in-use 错误省略 `family` 主体 → 错误主体用例 1 断言失败。
4. selection-only 路径改用 revision 发布 → Shell 组合用例 3 断言失败。

专项：theme-shell-flow 5 用例 / 125 断言，theme-scheme 26 / 408，
theme-observation 16 / 306，sidebar-showcase 6 / 55，build-context-header 1 / 1。

本批首次矩阵在无显示/GPU 的工具会话中运行，之后 `d9d234c` 修复了环境分类。组合最终树
矩阵（每套先编译再测试，完整套件运行期间工作树冻结）：

| 范围 | 本轮实际结果 |
| --- | --- |
| buildDir 全量 | **128 OK / 0 FAIL / 0 SKIP** |
| buildDir-asan unit | 104 OK / 1 SKIP；ASan/UBSan 启用，构建和测试均设 LSAN_OPTIONS=detect_leaks=0 |
| buildDir-no-rtti unit | 106 OK / 0 SKIP |
| buildDir-nosqlite unit | 101 OK / 1 SKIP |
| buildDir-tools | 8 OK |
| release_check --ci | 8/8，无 SKIP；headers 198，docs 87，clang-format 23.1.1 |

主配置已实际运行 SDF、window-lifecycle 与 `sdf-display-skip`；ASan/no-SQLite 的 SKIP
均为既有 window-lifecycle 配置。SDK 双次打包与 vendored 下游消费者通过；6 条 Linux
approved golden 在 Hyprland/Wayland + RADV 下逐像素 0 差异。环境证据不自动完成 Shell
页面的人工视觉验收。

本批已随 `f821f40` 提交推送；页面正文、持久化、注册配色门槛、真实后端像素读回及人工
视觉验收均在后续独立批次。

### 环境更正记录：显示/GPU 可用性与测试分类（2026-11-08 晚，已随 `d9d234c` 提交）

此前多个会话的主套件显示 `sdf-device` SKIP、`sdf-state-probe` FAIL，早期记录写作
「SDL 无显示设备」。本轮在操作者授予完全权限后实测，确认真正原因并修复测试分类：

- **实际原因**：受限工具会话没有把宿主显示/GPU 资源暴露给测试进程。旧会话中
  `/dev/dri` 无 `card*/renderD*` 节点，Vulkan 只能枚举到 0 个物理设备；部分会话即便
  `DISPLAY`/`WAYLAND_DISPLAY` 环境变量存在，compositor socket 也不可达。两个 SDF 探针
  用「环境变量是否存在」判断桌面可用，于是残留变量导致 `SDL_Init` 失败被记为 FAIL，
  而 `SDL_CreateGPUDevice` 失败只被 `sdf-device` 记成 SKIP。GNOME 的屏幕共享/portal
  授权只影响门户截图工具，**不影响** SDL 视频初始化、GPU 设备创建或页面探针的 GPU
  读回，不是本问题的原因。
- **完全权限后的实测**：`/dev/dri` 出现 `card1`、`renderD128`；`vulkaninfo` 枚举到
  AMD Radeon RX 6750 GRE 12GB（RADV NAVI22，Mesa 26.2.6）；SDL 探针 current video
  driver = `wayland`，`SDL_CreateGPUDevice` 成功；主套件全绿（见下）。
- **测试分类修复**：`tests/sdf_device_test.cpp` 与
  `probe/cross_platform/desktop/sdf_state_probe.cpp` 不再用环境变量猜测显示可用性，
  改为实际调用 `SDL_Init(SDL_INIT_VIDEO)` 与 `SDL_CreateGPUDevice`；不可用会话按平台
  契约输出 `SKIP` 并以 77 退出（`--require-gpu` 时仍视为错误 1）。新增 integration
  测试 `sdf-display-skip`（`tests/display_availability_skip_test.py`）用不存在的 SDL
  视频驱动确定性模拟不可用环境，断言 77/1 分类与 SKIP 诊断；故障注入恢复验证通过。
- **环境复验矩阵**（本机 Hyprland/Wayland + RADV，完全权限会话）：

  | 范围 | 结果 |
  | --- | --- |
  | buildDir 全量（含 sdf-device、sdf-state-probe、window-lifecycle、sdf-display-skip） | **128 OK / 0 FAIL / 0 SKIP** |
  | buildDir-asan unit | 104 OK / 1 SKIP（window-lifecycle，驱动侧 LSan 的既有配置） |
  | buildDir-no-rtti unit | 106 OK / 0 SKIP |
  | buildDir-nosqlite unit | 101 OK / 1 SKIP |
  | buildDir-tools | 8 OK |
  | release_check --ci | 8/8；headers 198，docs 87，clang-format 23.1.1 |

- **页面 golden 复现**：对 manifest 中 6 条 Linux approved 基线
  （home/label/anchors × cpu/sdf）重新渲染并比较，全部 **0 个差异像素、0 通道差**；
  说明当前 Hyprland/Wayland + RADV 环境与批准基线环境逐像素一致。记录环境时应同时
  标注 compositor/协议（Hyprland/Wayland），不要沿用 GNOME 会话的描述。

**更正结论**：此前「沙箱无显示设备」应表述为「工具会话未暴露显示/GPU 资源，且探针用
环境变量误判可用性」；权限调整后资源可用，SDK 与 golden 路径均恢复正常。文件/GPU
权限之外的系统与仓库代码没有对应改动。

### P4.3 D2 第 4a 步软件/GPU framebuffer 读回核对实施记录（2026-11-09 复核完成）

Shell 框架准备与环境分类修复已分别提交为 `f821f40`、`d9d234c`（状态收尾 `32fc7ac`）。
本批按 §6.8 步骤 1–3 补上真实后端像素读回（页面正文仍由维护者负责，本批不修改
showcase）：

- 软件路径（`sdl-visual`，unit）：真实 `SdlRenderDevice` + 软件 renderer 绘制单层/叠层
  半透明矩形，读回 framebuffer，期望值来自 foundation 唯一合成器；容差 ±2，并含
  「线性空间必须被拒绝」的负向对照。目标为 `SDL_PIXELFORMAT_RGBA32`、8 位/通道，
  既有本地 `over` 收敛到共享实现。
- GPU 路径（`sdf-device`，integration）：真实 GPU renderer + `SdlRenderDevice` 绘制
  同样输入，读回 `SDL_PIXELFORMAT_RGBA32` 离屏 render target（8 位/通道）与共享合成器
  比较；容差 ±4，负向对照同样生效。入口颜色/alpha 为 8 位量化值；无显示/无 GPU 会话
  沿用 77 跳过分类。
- 默认 window swapchain（`sdf-device`，2026-11-09 补充）：在真实窗口的默认目标上绘制同一组
  单层/叠层输入并读回；8 位 sRGB 目标必须与共享合成器一致且拒绝线性空间，读回不可用或
  目标不是 8 位 sRGB（HDR/浮点）时打印 SKIP 原因、不把未覆盖目标伪装成通过。
- 实测：Hyprland/Wayland + RADV（RX 6750 GRE）软件与 GPU 路径都与编码 sRGB 合成一致
  （线性空间偏差约 27/25 通道，远超容差）。
- Windows 复验（2026-11-09，Windows 11 26200 + AMD Radeon 660M / D3D12，驱动
  31.0.21024.2004）：桌面会话内 `sdf_device_test --require-gpu --expect-format dxil`
  通过并输出 `composite readback matches the shared compositor (encoded sRGB, tolerance 4)`
  与 `default swapchain readback matches the shared compositor (encoded sRGB, tolerance 4)`；
  同一构建的软件路径 `sdl-visual` 读回用例 1 用例 / 11 断言通过。该结论覆盖 D3D12 离屏
  目标与默认 swapchain，仍不外推到 Linux 默认 swapchain 或其他驱动。
- 故障注入：共享合成器改线性 → 软件用例失败、GPU 读回报
  `actual=(123,102,108) expected=(150,106,133)`；复核时把设备混合改为
  `SDL_BLENDMODE_NONE` → 软件首层采样与 GPU 既有半透明采样均失败；默认目标读回删掉第二层
  绘制 → `actual=(45,83,157) expected=(123,101,108)` 并退出 1。恢复后源码与 HEAD 逐字节
  一致，重新编译后全部路径全绿，无 RED-FIRST 残留。

专项：`sdl-visual` 6 用例 / 219 断言；`sdf-device`（含离屏与默认 swapchain 读回）通过。
最终矩阵（本机 Hyprland/Wayland + RADV）：主套件 128 OK / 0 FAIL / 0 SKIP；ASan 104 OK /
1 SKIP（window-lifecycle，既有驱动 LSan 配置）；no-RTTI 106 OK / 0 SKIP；no-SQLite 101 OK /
1 SKIP；tools 8 OK；release_check --ci 8/8（headers 198、docs 87、clang-format 23.1.1）。
Windows 侧同日完成全量套件复验（125 OK / 0 FAIL，修复 shader 产物 CRLF 归一化、
`compile_commands` Windows 引号/反斜杠解析、format-gate 的 POSIX 假路径、`.obj` 目标文件
后缀与字节精确 fixture 检出这 5 项工具链可移植性缺陷后；并新增 `header-flag-parsing`
回归）；GPU 读回证据见
[Windows 验收清单](windows_acceptance.md#9-全量套件与工具链-windows-复验2026-11-09)。
本批复核完成并随当前统一提交推送。注册配色门槛、Linux 默认 swapchain/其他驱动读回
与色域策略最终定形仍待后续。

---

# D4 跨平台优先的窗口与渲染后端路线

> **本节主体保留迁移前的历史讨论，不再作为当前排期或代码现状。**
> 截至 2026-10-17，编入 SDL3 时默认走 SDL3，`IPlatformWindow` 和设备包装入口已存在。
> 正式状态与剩余验收见[平台与后端契约](platform_backend_contract.md)；D7/D8 处理视觉和输入收尾。
> 下文“候选”“NanWindow 仍持有”“待审阅”均指当时阶段，不能据此回退已落地设计。

## 目标优先级（迁移前讨论）

1. **首要目标：** 让基于 Nandina 的应用能在 Linux、Windows 和 Web 构建、启动并具备可用的
   输入、文本、资源和绘制路径。Web 不只是“SDL 能创建 canvas”，而是应用级可交付目标。
2. **平台方向候选：** SDL3 是主要候选，因为它同时提供桌面平台抽象和适用于 Emscripten 的
   2D Render API；这仍需用本项目的窗口、输入、文本和 SDF 案例验证，不据单个示例宣称全框架支持。
3. **后续可能目标：** 以本框架开发 Wayland dock/panel，甚至与 wlroots compositor 集成。它们是
   未来可组合的平台接入需求，不是首个跨平台交付的前置，也不要求框架现在依赖 wlroots。
4. **ThorVG 与窗口/图形 API 选型分离。** 当前 SDF 原语继续作为基线；只有出现 SVG、任意路径等
   具体能力缺口时，再评估引入独立矢量渲染能力。它不是 SDL3 或跨平台路线的必选项。

## 历史代码边界（迁移前）

| 事实 | 位置 / 含义 |
| --- | --- |
| `IRenderDevice` 是渲染 primitive 的抽象，公开接口不暴露 raylib 类型 | `render/render_device.hpp`；这是可复用边界，不等于完整平台适配已抽象 |
| 生产 GPU 实现是 raylib/OpenGL + 自研 SDF | `render/backends/raylib_device.*`、SDF geometry；迁移需核验 shader、裁剪、圆角、纹理和像素一致性 |
| `NanWindow` 仍持有窗口、设备、输入轮询与帧驱动职责 | `app/nan_window.*`；新平台不仅实现 `IRenderDevice`，还要接窗口生命周期、事件、时间和 redraw |
| key code 目前与 raylib/GLFW 取值有关，NanWindow 直接把 `GetKeyPressed()` 原值放入 `KeyEvent` | `widget/key_codes.hpp`、`NanWindow::poll_and_dispatch_input()`；SDL/wlroots 原始码必须先归一化 |
| 当前 glyph atlas 的 alpha CPU buffer 在 FontPipeline 存活期间保留，且 raylib texture 用 RGBA8 | `FontPipeline::atlases_` + `textures_`；`raylib_device.cpp::create_alpha_texture()` 展开为 RGBA8 |
| font cache 中的 5 bytes/pixel 是保留内存估算，不是像素寻址布局 | `FontPipelineCache::estimate_bytes()` 返回值用于 `retained_bytes_` 预算与 trim |
| 图片 decoder 与资源定位已有部分抽象；系统剪贴板和字体发现仍带平台差异 | `render/image_decoder.*`、`resource/platform_resource_locator.*`、`app/detail/linux_clipboard.*`、font pipeline |
| recording/offscreen 测试替身验证的是渲染调用契约 | 不证明窗口、Web 主循环、GPU shader 或输入法行为已跨平台 |

因此“实现约 25 个虚函数”不足以估算完整后端迁移：至少要分别验收 platform/window、input、
frame scheduling、render device、image/resource、font/clipboard 与构建/打包。scene 的平台中立性
有价值，但不能把它误读为现有 `NanWindow` 已可随意替换。

## SDL Renderer、GPU shader 与 Web 边界

SDL 官方 Emscripten 说明指出，SDL 2D Render API 在 Web 上使用 GLES2/WebGL；给出的
[`renderer/01-clear` 示例](https://examples.libsdl.org/SDL3/renderer/01-clear/)验证的是 SDL Renderer
路径，不是 `SDL_GPU`。但“SDL Renderer API 完全没有用户 shader”已过时：自 SDL **3.4.0** 起，
`SDL_CreateGPURenderer` / `SDL_GPURenderState` 可为 GPU-backed Renderer 配置自定义 fragment shader。
这使桌面 SDF shader 有了具体候选实现，不代表每个 renderer/backend 都提供该能力。参见
[SDL_CreateGPURenderState](https://wiki.libsdl.org/SDL3/SDL_CreateGPURenderState) 与
[SDL_GPURenderStateCreateInfo](https://wiki.libsdl.org/SDL3/SDL_GPURenderStateCreateInfo)。

截至本草案的核对日期（2026-10-12），SDL_GPU 设备创建 API 列出的驱动为
Vulkan、D3D12、Metal，未列出 WebGPU/Emscripten 驱动。参见
[SDL3 Emscripten notes](https://wiki.libsdl.org/SDL3/README-emscripten) 与
[SDL_CreateGPUDevice](https://wiki.libsdl.org/SDL3/SDL_CreateGPUDevice)。后端状态可能发展，
真正选型或升级 SDL 时应重新核对。

因此，SDL3 平台层、SDL 的 2D Renderer API 与 GPU renderer 是相关但不同的路径：

| 路径 | shader 能力 | 本项目要验证的内容 |
| --- | --- | --- |
| SDL GPU-backed Renderer（桌面候选） | SDL 3.4+ `SDL_GPURenderState` 提供自定义 fragment shader 状态 | 现有 SDF 几何/参数/clip 能否表达；Vulkan、D3D12、Metal 的 shader 编译/加载与像素结果；API 和驱动限制 |
| SDL 2D Renderer 的 Emscripten/WebGL 路径 | 可用 SDL 2D 绘制 API；不能假定具备 SDL GPU renderer 的自定义 fragment shader 路径 | SDF 是否需改为细分几何/其他 AA；若需 Web 专用 shader renderer，是否接受一份额外 backend |
| 桌面 GPU renderer + Web 专用 renderer | 可按平台保留 shader 能力 | 多 `IRenderDevice` 实现、视觉回归、shader 与 geometry 行为的一致性、维护和 CI 成本 |

**P1 门槛：** 分开回答两题，而不是以“SDL 支持 shader”一笔带过：

1. 桌面 GPU Renderer 的 shader path 能否表达并匹配现有 SDF primitive、AA、圆角/阴影和 clip 行为？
2. 目标 Web renderer 能否运行同一类 fragment shader？若不能，是采用独立 WebGL/WebGPU `IRenderDevice`，
   还是有意识地接受 Web 光栅化差异？

`SDL_GPURenderState` API floor 为 SDL 3.4.0。项目应声明最低 SDL 版本，并在 CI/SDK 锁定文件中固定
具体受测版本；升级时重新核对 GPU driver/shader 能力与 Emscripten 路径。Web driver 列表是截至
2026-10-12 的观察，不是永久保证。

### Shader 构建工具是 P1 的构建机依赖

GPU Renderer 的各目标后端需要对应的 shader binary。现有 SDF shader 是 GLSL 330；应先选定 canonical
shader source 与到 SPIR-V/DXIL/MSL 等目标格式的转换链。若使用
[SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross)（及 GLSL→SPIR-V 的前置编译器），
它们是**构建机运行的 host tools**，不是目标应用的隐式系统依赖，也不应依赖开发者机器上“刚好装好”的工具。

P1 通过条件至少包括：

- 明确工具及其依赖的准确版本、获取/构建方式和运行平台；跨编译时执行 host 版本工具；
- shader outputs 在 build tree 中生成或作为明确的 checked-in source artifact 管理，生成输入/命令/后端均进入
  build graph；增量构建不会漏掉源 shader 变化；
- 安装/打包应用时带上所需 shader binaries，或者有清楚的生成式嵌入流程；不在运行时偷偷要求构建机工具；
- shader 编译失败报告源文件、目标 backend/format、工具版本与诊断输出；CI 对每个承诺支持的目标格式实际生成并加载。

不能假设一条 GLSL 源码可由 SDL 自动转成所有格式；若采用 shadercross，需把 source frontend 到 SPIR-V/其他中间格式
的步骤也纳入依赖和版本契约。

`SDL_GPURenderState` 从 SDL 3.4.0 起可用。项目对外声明的 minimum version 不得低于 3.4.0；CI/SDK
再固定精确受测版本，并在升级时复验。该 API 较新，P1 先交付“目标 driver 上可用 + 已知限制清单”，不直接等同
生产就绪。诸如 [SDL issue #15774](https://github.com/libsdl-org/SDL/issues/15774) 的 shader 使用报告应作为
验证线索；issue 的存在本身既不证明功能普遍不可用，也不能替代项目自己的 renderer/driver 测试。

### Alpha atlas 的缓存预算边界

当前 `FontPipeline` 同时持有 CPU `GlyphAtlas` 与 `GlyphAtlasTexture`；`GlyphAtlasTexture` 引用 atlas，更新与扩容路径
会继续读取 CPU alpha 数据，因此 pipeline 存活期间并未释放 alpha atlas。结合 raylib backend 将 alpha 展开为 RGBA8，
现有 5 bytes/pixel 可读作约 1 byte CPU alpha + 4 bytes RGBA backing。它是缓存 eviction 用的估算值，不保证等于
driver 的精确 VRAM 分配（例如对齐、mipmap 或内部格式可能不同）；`maximum / 5` 只防止估算乘法溢出，不是纹理寻址边界。

预算低估会令 retained font pipelines 超过目标内存；高估可能提前淘汰并增加重建/抖动。不要把该值当作
buffer 实际尺寸或越界检查。

后续抽象应把**CPU atlas 常驻字节**与**backend texture memory estimate**分开：CPU 部分由 atlas 格式拥有，GPU 部分由
render device/texture format 报告或保守估计；text cache 合并两项预算。P1/P2 应测试 atlas 增长、纹理重建、缓存 trim
与预算变化，并验证 backend 报告是估算语义，而非精确显存承诺。

## Wayland shell / wlroots：未来集成，不是当前阻塞

原稿曾把“SDL 只创建 xdg_toplevel”写成 layer-shell 硬阻塞；SDL3 当前文档说明，可为其他协议
创建 custom roleless Wayland surface，并取回 `wl_display` / `wl_surface`，同时继续让 SDL 处理输入
与绘制。该模式下除 `SDL_SetWindowSize()` 外，许多通用窗口状态调用都是 no-op；SDL 也不会收到
surface 尺寸变化通知，因此应用要处理协议 configure 并同步尺寸。“技术上可接入”仍不等于
“dock/panel 已验证可靠”。参见
[SDL3 Wayland notes](https://wiki.libsdl.org/SDL3/README-wayland)。

本项目不需要现在实现 layer-shell。将来可分别评估：

- SDL custom surface + layer-shell，是否满足普通 dock/panel 应用；
- 面向 wlroots compositor 的独立渲染/输入宿主接入；
- 若现有 `NanWindow` 所有权模型不适配，再增加窄的 host/platform 接口。

不能假定 compositor 集成只需传入一个 `IRenderDevice`：当前 `NanWindow` 也拥有事件轮询、时钟、
帧提交和窗口状态。未来应先定义 host/platform 的**行为契约**，再比较 SDL custom-surface adapter 与
wlroots host；两个 adapter 可以用不同机制满足同一组行为，不要求实现细节相同。

该契约至少规定：

- **尺寸与缩放：** host 报告 surface 的物理/逻辑尺寸及 scale；变化进入与普通窗口 resize 相同的应用/layout
  路径，不允许 adapter 各自调用节点 `layout_to()` 或直接写控件尺寸。
- **输入：** host 输入先转换为 Nandina 规范键值/指针/文本事件；scene/widget 看不到 SDL、GLFW、wlroots 原始 key code。
- **帧时序：** 明确由 host callback 或 Nandina scheduler 其中一方拥有 frame tick/时间推进，禁止两者同时驱动。
- **redraw：** 统一应用请求重绘的语义，并规定它如何合并、调度到下一帧；不能由每个 adapter 私自用 timer/vsync 改变语义。
- **生命周期：** surface/host attach、resize/configure、暂不可绘制、detach 与 shutdown 有唯一顺序，已排队事件不能在 host 销毁后进入应用。

满足该行为契约后，Nandina 才能复用 scene/layout/widget 并以 adapter 隔离 surface 技术。此工作属于未来 P4，
不阻塞 P0–P3 跨平台应用交付。

## 历史交付草案（已由正式 D4 契约取代）

| 阶段 | 目标 | 通过条件 |
| --- | --- | --- |
| P0 SDL3 应用级探测 | 建立最小 SDL3 window/event/frame loop，Linux、Windows、Emscripten 各自配置/构建/运行 | 必须实际经过 Nandina 的 SDF/clip/shadow、字体 atlas 上传、resize/DPI 与键盘路径；记录文本、资源、剪贴板和 Web 主循环能力，不能把 SDK sample 通过当成 Nandina 应用通过 |
| P1 渲染与 shader 管线门槛 | 分别在 SDL GPU-backed Renderer 桌面路径和目标 Web Renderer 验证 SDF expressibility；建立 shader binary 生成链 | SDL minimum >= 3.4.0、CI 精确 pin 测试版；shader source/front-end/target formats/tool versions/output location/依赖边/错误诊断齐全；能表达时比像素行为和性能；不能表达时显式选择 Web 专用 backend 或接受可见差异；先交付已知限制清单 |
| P2 平台能力与消费者回归 | 按 P0 暴露的问题补平台服务、资源与字体策略、输入归一化、Emscripten 主循环/打包 | alpha atlas + GPU texture 的实际驻留与预算估算符合策略；SDL raw key 不穿透 backend，映射测试与全量 widget tests 通过；逐平台真实构建和运行 |
| P3 默认后端决策 | 新后端达到生产应用验收后，决定是否切换默认、保留 raylib 或支持多 backend | Showcase、playground、文本/输入和集成测试都走正式入口；移除旧后端另作显式决定 |
| P4 host 行为契约与 shell/compositor adapter | 有具体应用需求后先定义 host 行为契约，再分别评估 SDL custom surface 或 wlroots host | 尺寸/scale 进入普通 resize-layout 路径；输入归一化；帧时钟单一所有者；redraw 语义统一；生命周期有序。先契约、后选 host；不阻塞 P0–P3 |
| 独立能力：ThorVG | 有真实 SVG/任意路径需求时评估，不与 SDL 迁移捆绑 | 证明当前 SDF/三角形路径不能以更小成本满足用例；明确软件/硬件后端和维护成本 |

## 待审阅问题

1. Linux、Windows、Web 三个平台的首发验收等级分别是什么（CI build、自动测试、人工运行哪些必需）？
2. SDL3 是否先作为 window/input/frame-loop 层接入，渲染暂沿用 OpenGL/现有 SDF；还是 PoC 同时比较 SDL_Renderer？
3. 若采用桌面 SDL_GPU，是否接受 Web 使用独立 `IRenderDevice` 实现及其长期双路径成本？
4. P0 应用级样本需要覆盖哪些 Nandina 功能，才能称为“应用跑起来”，而不只是 SDK 示例？
5. P4 的 host 行为契约由哪个窄接口承载，哪些部分留在外部 adapter 而不进入 framework core？至少涵盖尺寸/scale、输入、帧时钟所有权、redraw 与生命周期。
6. `NanWindow` 与平台服务的可插拔改造何时值得做，哪些具体消费者会触发它？

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
上表保留为当时的缺口证据；P3.3a/P3.3b1 已接通 scene→SDL 的合成事件、预编辑显示与密码用途，
剩余 caret 区域/真实 IME/软键盘验收归 P3.3b2。
正式依据见 [焦点与输入 §5](focus_and_input.md#5-文本输入会话与软键盘)：分别记录协议/模拟测试、
指定平台组合输入、指定平台软键盘请求/反馈/遮挡三个状态，不以一个“阶段二完成”代替真实验收。

---

# D7 渲染质量与后端视觉一致性

> 方向已接受、部分实施；2026-10-18 修订。采用独立批次修混合、交付生产 SDF、共享图片解码和裁剪，
> 不再用“一行修复”或“搬一下 shader”概括验收范围。
> 已完成：混合修复（§5 证据）、Vulkan 上的参数/状态最小实验（64 字节参数、4 KiB 边界、
> 三档缩放与纹理/字形交错、两条失败注入、分项计时）。未完成：生产 SDF 接入、真实页面验收、
> 多平台产物与共享图片解码——见[渲染设备路径契约](render_device_path_contract.md) §8。

## 背景与当前实现

SDL 默认路径已经可运行，但按钮按下时白底白字、涟漪像实心圆盘、Dialog 遮罩近黑。
`render/backends/sdl_device.cpp` 只设置纹理混合、未设置 `SDL_SetRenderDrawBlendMode`，
这解释了半透明非纹理图元失真。**已于 2026-10-17 修复**：设备构造（初始化边界）启用
`SDL_BLENDMODE_BLEND`，并由 `tests/sdl_visual_tests.cpp` 的解析像素断言钉住
（删除该设置会让 4 个用例全红；GPU 路径证据见[视觉回归](visual_regression.md) §5）。

| 项 | raylib | 当前 SDL |
| --- | --- | --- |
| 图元 | SDF fragment shader | CPU 几何细分 |
| 抗锯齿 | 基于导数的解析 AA | 无解析 AA；圆形细分有段数上限 |
| 混合 | 已有混合路径 | 缺少非纹理绘制混合设置 |
| 图片 | raylib CPU 解码适配器 | 平台未提供解码器，Image 消费者明确降级 |
| 裁剪 | 专用圆形/圆角矩形交集图元 | 能力查询为 false，涟漪使用矩形裁剪回退 |

现有交互回归看状态位，静止帧像素比较往往不包含有透明度的状态层。因此这些通过记录
不能证明按下/涟漪/遮罩的视觉正确性。

## 已接受方向与证据边界

1. **混合先修且独立验收。** 先定义已知背景和前景颜色下的预期像素，再加交互态回归；
   raylib 对照是补充，不能成为唯一正确性依据。
2. **SDF 是桌面高质量路径。** 使用 `SDL_GPURenderState`，不投入另一套长期 CPU AA 算法。
   当前宿主 GPU Renderer 失败会回退普通 SDL Renderer，基础路径需保留并如实报告能力；
   不按半径大小在两种视觉语义间随意切换。
3. **trace 证明选定帧的上层调用一致，不证明所有渲染结果等价。**
   既有 Home/Label/Anchors 的逐图元对照支持把改动主要限制在设备层，
   但新增 capability 或服务边界仍可能需要消费者调整。
4. **探测 shader 与生产 shader 分开验收。** 当前探测写死形状且
   `num_uniform_buffers = 0`；生成 SPIR-V/DXIL/MSL 不证明生产多参数图元已经可用。
   spirv-val 只校验 SPIR-V，DXIL/MSL 目标设备加载仍待验证。
5. **质量目标为达标且差异有界可解释。** 不要求所有 GPU/后端逐像素完全相同。
   先定义背景、字体、缩放、动画时刻与容差，不能看结果失败后才放宽标准。

SDL 官方依据：
[创建 GPU render state](https://wiki.libsdl.org/SDL3/SDL_CreateGPURenderState)、
[fragment uniforms](https://wiki.libsdl.org/SDL3/SDL_SetGPURenderStateFragmentUniforms)、
[设置/恢复状态](https://wiki.libsdl.org/SDL3/SDL_SetGPURenderState)。
接口可用性不替代本项目在具体 Renderer 和 driver 上的运行证据。

## 图片解码器：删除平台钩子，改为应用服务（2026-10-17 定）

`IImageDecoder` 的契约是 **CPU-only、允许后台线程调用**、输入字节输出 RGBA，与窗口创建、图形上下文和
present 无关。把它留在 `IPlatformWindow`——即使改成"由应用注入"——仍然让窗口接口承担不属于它的职责。
因此**删除 `IPlatformWindow::make_image_decoder()`**。

目标所有权与调用关系：

```text
NanApplication
  └─ 持有默认或应用注入的 IImageDecoder
       └─ 窗口 TextureCache 用它做 CPU 解码
IPlatformWindow
  └─ 创建窗口关联的 IRenderDevice
       └─ TextureCache 通过设备上传 RGBA
```

规则：

- 普通应用无需配置（应用服务提供默认解码器）；自定义解码器在创建窗口前注入；
- **首批不支持运行时热替换**（会引出缓存失效与"正在执行的解码任务切换"问题）；
- 异步任务持有服务的**有效所有权**；退出时完成取消或等待，不能只靠裸指针；
- 平台专有解码器将来也作为 `IImageDecoder` 实现注入，不恢复窗口虚函数；
- **CPU 解码能力与设备 RGBA 上传能力分别验收**。当前 `TextureCache` 只经
  `device_->create_rgba_texture(...)` 上传，而 SDL 设备**尚未实现该入口**——
  所以"移走解码器"只是第一步，`Image` 在 SDL 上要真正可用还需要补上传链。

**破坏性变更按 [SDK 表面](sdk_surface.md) §5 记录**：不保留一个仍可 override 但已不再被调用的虚函数
（那是最难发现的静默失效）；库内实现、测试替身与下游样例一次改完并要求重新编译。

### 三项能力分开表达（2026-10-17 定）

`supports_image_loading()` 一个布尔值概括不了三件独立的事，**取消这种概括**：

| 能力 | 归属 | 当前取值 | 表达方式（名称首批定形） |
| --- | --- | --- | --- |
| 解码服务可用 | 应用服务 / `TextureCache` | **已落地（2026-10-20）**：`NanApplication` 默认注入 `make_cpu_image_decoder()`（stb），两条后端路径都可用 | 服务查询（例如 `TextureCache::can_decode()`） |
| 设备**直接**加载文件/字节 | `IRenderDevice::load_texture_from_*` | **已落地**：raylib true；SDL false（`supports_direct_image_loading()`） | 设备查询（`supports_image_loading()` 已改名收窄） |
| 设备 **RGBA 上传** | `IRenderDevice::create_rgba_texture` | **已落地**：raylib / SDL 均为 true（SDL 补齐 `create_rgba_texture`） | 设备查询（`supports_rgba_texture_upload()`） |

**标准图片路径是否可用 = 解码服务可用 且 设备支持 RGBA 上传。** `Image` 的解析顺序：

1. 两者都可用 → 走**服务路径**（解码在服务、上传在设备）——这是所有后端的目标标准路径；
2. 服务不可用但设备支持**直载** → 用直载（保留 raylib 现有能力，不让"服务未装"退化成不可用）；
3. 都不可用 → 明确降级：一次性诊断 + 空内容，**不逐次重试**，并报告是哪一项缺失。

规则：

- 三项能力**分别报告**，禁止再用一个布尔值表示"图片能用"；
- 三项能力可以独立为真/假，组合结果必须能从报告里读出来（这是排查"图片为什么不显示"的第一手信息）；
- **已实施（2026-10-20）**：`Image` 守卫按三项能力选择服务路径/直载；旧
  `supports_image_loading()` 与 `IPlatformWindow::make_image_decoder()` 已删除，
  `make_raylib_image_decoder()` 改为 `make_cpu_image_decoder()`；`TextureCache` 同步路径
  优先"解码 + 上传"，服务失败不静默改道。

## 实施顺序与验收

| 批次 | 交付 | 验收与故障注入 |
| --- | --- | --- |
| 混合（**已完成** 2026-10-17） | 在设备初始化边界设置混合；`check_sdl` 处理失败 | 已知背景（**测试自绘**）上的半透明矩形/圆/描边内部像素有独立预期；固定动画时间的按钮按下、涟漪中间态、Dialog 遮罩；读回在 present 之前；注册为 integration 测试；移除设置必须失败（已注入验证：4 用例全红）；golden/容差规则见[视觉回归](visual_regression.md) |
| SDF 最小生产实验 | 同帧绘制多个不同尺寸/半径/颜色/描边图元，交错文字及上传纹理 | uniform 更新与布局正确，图元参数互不污染；退出自定义状态后文字/纹理正确；故意跳过恢复或参数更新会红 |
| 产物交付 | 源、固定工具信息、生成记录与 shader 产物进入可重复交付链 | 推荐交付受控产物，普通消费者无 shadercross 也能构建；维护关卡检查源与产物一致；每种格式分别记录生成、校验、加载 |
| 生产 SDF | 圆角、圆、线/描边、圆弧、阴影按现有设备契约逐项接通 | 静止与交互像素、图元 trace、分数缩放/小半径/细描边/半透明重叠；**每帧状态切换次数**与总帧时间、内存一起报告（按[性能预算](performance_budget.md)的协议采样，新路径经审核建立自己的基线）；驱动级逃生舱可用；基础回退独立验收 |
| 图片解码 | 删除平台钩子、应用服务注入、`create_rgba_texture` 上传链、实际 Image 消费者与预处理行为 | PNG/JPEG、透明图片、错误输入与 crop/resize/tint；**解码与上传分别验收**；两后端一致，解码失败可定位；删除钩子后无遗留 override |
| 专用圆角交集 | 涟漪圆形与圆角矩形的交集 | 边角不外溢；缺能力时回退仍正确；不顺带宣称支持任意子树裁剪 |
| 通用圆角裁剪 | 有实际图片/子树用例后定形 clip 协议、嵌套和状态恢复 | 图像与嵌套内容、缩放、恢复、绘制/命中契约及能力缺失路径分别验证 |

SDF 只等待混合正确、SDL3 来源与 shader 交付定形，不等待整个 submodule 迁移完毕。
第一批混合回归可以复用当前固定环境，不能把依赖工程变成修复缺陷的前置。

## 实施前仍需定形

- shader 参数结构、资源绑定、alpha 约定、AA 外扩边界与坐标空间（契约草案见
  [渲染设备路径契约](render_device_path_contract.md)）；
- **fragment uniform 预算**：`SDL_SetGPURenderStateFragmentUniforms` 的数据按 slot 经
  `SDL_PushGPUFragmentUniformData` 推送（16 字节对齐），参数集必须装进目标驱动的 push uniform 上限；
  装不下就要把一部分参数移到顶点属性或纹理，这属于参数布局决策，不能等到图元接通后才发现；
- 受控产物的嵌入/安装方式、源与产物一致性检查，以及各目标 driver 的选择；
- 普通 SDL Renderer 回退的质量说明和诊断；不能假设所有 Renderer 均可使用 GPU render state；
- **状态切换成本**：每次 `SetGPURenderState` 都可能让 2D 批次 flush。Label 页每帧上千次纹理绘制、
  填充/描边与文字交错，所以必须统计"每帧状态切换次数"并给出预算，不能只报总帧时间；
- **驱动级逃生舱**：SDF 在某个驱动上出错时，需要编译期或运行期开关退回 CPU 几何，并在能力报告里
  写明当前用的是哪条路径（不能让"看起来一样"掩盖退化）；
- 专用交集与通用 clip 的能力命名及缓存/嵌套语义；
- 解码库与预处理实现；与 D5 资源域缓存的交界保持可迁移；
- **像素回归的 harness 约定**（不满足则结果不可复现）：
  1. 测试**自己画背景**，不依赖宿主的 clear 色 —— 跨后端像素对照曾经因为两侧清屏色不同产生 4.51 的平均差，
     与渲染能力无关；
  2. 读回发生在 `SDL_RenderPresent` **之前**（探测里 present 之后 `SDL_RenderReadPixels` 曾读到空帧）；
  3. golden 图的存放位置、容差、再生脚本与"谁能改 golden"写进同一处，并**注册为 integration 测试**，
     不能只留在手工探测命令里；
- Windows D3D12/Metal 的真实加载不在 Linux 验证中代为勾选。

---

# D8 键码与输入码空间

> P3.1a 事件快照、P3.1b 键身份/生命周期与 P3.1c 跨类型保序均已实施（2026-10-29/31）。
> 分清物理位置、逻辑含义与提交文本，再迁移平台映射与消费者；键身份的精确类型/签名另批定形。

## 当前实现

- `scene::KeyEvent` 使用 `foundation::LogicalKey` + `PhysicalKey`（P3.1b 已实施）；
  旧 int 键码、`keycode()` 与 `widget/key_codes.hpp` 已删除。
- SDL 转发 press/repeat/release；键事件用 `SDL_KeyboardEvent.mod`，按钮/滚轮用队列修饰键
  快照。unknown 身份照样派发，两者皆未知时计入 `unknown_key_count()`。
- P3.1a 已修复字段丢失：`NanWindow` 用按钮/滚轮各自坐标经 viewport 映射后派发，
  不再以帧末 `frame_input_.pointer` 替换。同帧“在 A 按下、移到 B”仍命中 A。
- `FrameInput` 的事件已有坐标和修饰键字段；独立的帧级 `pointer` / `modifiers` 是当前状态，
  不得用它们替换历史事件字段。
- `FrameInput::events` 按平台队列顺序保存跨类型事件，`NanWindow` 顺序派发（P3.1c）。
- 提交文本单独使用 UTF-8 `TextInputEvent`；组合输入会话仍属 D1 第二阶段。

## 键身份与事件契约

| 概念 | 用途与边界 |
| --- | --- |
| 物理键位置 | 布局无关的位置控制；由 SDL scancode、Web code 等明确物理来源映射 |
| 逻辑键含义 | 命令与编辑导航；字符键随布局解释，不能用 ASCII 算术冒充所有布局 |
| 提交文本 | 实际插入内容，继续走文本事件；不能从键身份反推输入法结果 |
| 原生信息 | 可选诊断/录制数据，带来源或明确上下文；不成为组件的跨平台判断依据 |

框架拥有这些值类型，平台 mapper 不泄漏 SDL/GLFW 类型。目标放在低层 input 值头，
不让 scene 反向依赖 widget。物理/逻辑两种身份允许分别未知；首批确定具体类型名与
逻辑字符表示，不要求把所有 Unicode 字符枚举成键。

快捷键必须明确匹配哪种身份：桌面编辑命令默认考虑逻辑键语义，位置绑定必须显式；
平台主修饰键、布局以及组合期间行为需要契约，不能宣称“一种 Key 足够所有场景”。
`native_code` 不代替上述任一身份。

SDL 官方 [KeyboardEvent](https://wiki.libsdl.org/SDL3/SDL_KeyboardEvent) 同时提供
scancode、key、mod、down 和 repeat。mapper 应消费事件时快照，不能把稍后的全局状态
套到更早的事件。raylib/GLFW 可获得的字段必须实际核对，不能凭供应商键码名字推断语义一致。

### P3.1a：事件快照（2026-10-28）

- 不改公开事件类型。按钮和滚轮按各自 `screen_pos()` 经窗口 viewport 映射后派发；
  move 的位置和 delta 继续按同一映射转换。帧末指针不能决定先前点击或滚轮的命中目标。
- SDL 键事件取 `event.key.mod`。鼠标事件没有 mod 字段，使用窗口按 SDL 队列顺序维护的
  修饰键状态；所有 key-down/up（含未映射键和被过滤的 repeat）都先更新该状态。
- 创建时初始化状态，跨帧保留；失焦清空，重新获得焦点时与平台当前状态同步。
  `FrameInput::modifiers` 仍是平台帧级当前状态，不回填历史事件。
- 保留 move → buttons → wheel → keys → text 分组派发，不宣称跨类型保序；本批仍只转发
  非 repeat 的 key-down（P3.1b 同轮随后补上 release/repeat/unknown，此处保留 P3.1a 批次快照）。
- 验收：真实 SDL 队列覆盖同帧修饰键转换、跨帧和过滤事件；真实 NanWindow tick 覆盖
  “A 处点击/滚轮后移动到 B”及缩放/letterbox 映射。故障注入须使对应回归失败。

#### 实施与验证记录（2026-10-29）

- 实现：`app/nan_window.cpp`、`app/detail/sdl_platform_window.*`；公开事件类型未变。
- 回归：`tests/sdl_platform_input_tests.cpp` 和 `tests/window_input_tests.cpp`，注册为
  unit `sdl-platform-input`；SDL dummy/software 环境运行真实 poller 与 NanWindow tick，
  4 用例/160 断言。不是实际键鼠设备或 Windows 实机验收。
- 红测：旧宿主使无缩放/2× 两支点击目标断言失败（A 收到 0 个按钮事件而非 2 个）；
  将键事件状态更新改回 `SDL_GetModState()` 后，2 用例/16 断言失败，恢复后全绿。
- 独立 smoke：临时无 Catch 可执行程序运行真实 SDL → NanWindow tick，
  2× + letterbox 下 A 收到按下/释放/滚轮，帧末指针位于 B；退出 0，临时文件已清理。
- Linux 验证：主套件 107/107；ASan/UBSan unit 84 通过 + window-lifecycle SKIP
  （`LSAN_OPTIONS=detect_leaks=0`）；no-RTTI unit 86/86；no-SQLite/headless unit
  81 通过 + 1 SKIP；纯工具 8/8；release check 8/8，无 SKIP。
- 工具环境：Meson 1.12.1、clang-format 23.1.1；不是 CI 固定 Meson 1.7.2 复验。
  Windows 本轮未验证；实际键鼠、跨类型保序和多布局键身份留给相应后续验收。

### P3.1b：键身份与生命周期（2026-10-29 已实施）

- 低层值归属 `foundation/input.hpp`：`PhysicalKey` 是具名物理位置，`LogicalKey` 是
  Unicode scalar 值及独立命名键（命名键值在 Unicode 范围外）；unknown 为 0。
  字符逻辑身份取布局结果，不以物理位置或 ASCII 大小写算术代替；提交文本仍只走 UTF-8 文本事件。
- `scene::KeyEvent(LogicalKey, Action, KeyModifiers = {}, PhysicalKey = unknown)`；
  查询 `logical_key()` / `physical_key()`，删除旧 int 构造与 `keycode()`，不保留兼容别名。
  `Action` 为 press/release/repeat；`is_pressed()` 仅初次 press，新增
  `is_press_or_repeat()` 供可重复编辑/导航消费，release 不触发组件动作。
- 左右修饰键物理/逻辑身份各自保留；`KeyModifiers` 仍为四个聚合布尔值。
  小键盘物理身份独立；Enter/数字/运算符的逻辑含义与主键区相同，NumLock 导航含义由 SDL 提供。
- 两种身份分别可 unknown；保留任一已知身份。两者均未知也派发 unknown，不伪造其他键，
  SDL 的缺口计数只计两者均未知的事件，旧 skipped 计数入口随清理删除。
- SDL 按 scancode 映射物理位置，按 keycode 映射逻辑身份；已知物理键从初次 press 到
  repeat/release 保留初始逻辑身份，防止按住期间布局/修饰键变化造成释放身份不匹配。
  没有先前 press 的 release 仍按事件自身字段派发；失焦清除身份记录，不合成 release。
- 桌面编辑快捷键按逻辑键匹配，本轮 Linux 主修饰键为 Ctrl，不把 Super 当作 Ctrl；
  Alt/AltGr 组合不触发 Ctrl 编辑快捷键。平台文本组合/抑制规则仍归 P3.3，不承诺 IME。
- 编辑删除/光标、方向导航与 slider 支持 repeat；激活、开合、提交、撤销重做和焦点 Tab
  仅初次 press。typeahead 继续使用提交文本，不从键身份生成字符。
- SDL poller 仍消费进程级事件队列，现有实现只对焦点事件按 windowID 过滤；单窗口宿主是当前
  前提，多窗口同时打开需要后续批次统一按 windowID 分发，不在本批改变。
- 全部框架消费者与测试一次迁移，删除 `widget/key_codes.hpp` 和局部 GLFW 魔数。
  验收用真实 SDL 合成 US/非 US 布局事件、左右修饰键、小键盘、未知身份和键生命周期；
  明确这是 mapper/宿主行为验证，不冒充真实布局设备验收。跨类型保序仍留给 P3.1c。

#### 实施与验证记录（2026-10-29）

- 实现：`foundation/input.hpp`（新公开头，experimental）、`scene/input_event.hpp`、
  `app/detail/sdl_platform_window.*`、`app/nan_window.cpp`；全部 widget 消费者与测试迁移，
  `widget/key_codes.hpp` 删除并从 Meson/header-surface 清单移除。
- 映射：scancode → 物理位置；keycode/NumLock → 逻辑键。NumLock 关闭的小键盘 5 保留
  `physical_keypad_5`，逻辑 unknown（逻辑枚举没有 Clear）；Enter 主键区与小键盘逻辑相同、
  物理不同。AltGr 的 `SDL_KMOD_MODE` 聚合进 Alt，因此不触发 Ctrl 编辑命令。
- 生命周期：只派发有身份区分的 press/repeat/release；初次 press 到 release 之间按物理键
  保留初始逻辑身份；失焦清空且不合成 release。
- 回归：`sdl-platform-input` 10 用例/348 断言覆盖同帧快照、布局差异、Unicode 逻辑字符、
  独立 unknown、左右修饰键、小键盘、身份稳定与失焦清理，以及真实窗口 tick 把重复删除、
  提交文本与释放送进聚焦编辑器；新增 unit `key-lifecycle`
  （5 用例/14 断言）覆盖删除/光标/方向/滑杆接受重复，而激活、开合、提交与撤销只接受
  初次按下，以及 AltGr 不触发命令。
- 红测：编辑删除改回 `is_pressed()`、Pressable 改为接受 repeat 时 `key-lifecycle` 分别失败；
  恢复后通过。
- Linux 验证：主套件 108/108（含新 `key-lifecycle`）；ASan/UBSan unit 85 + window-lifecycle
  SKIP；no-RTTI unit 87/87；no-SQLite/headless unit 82 + 1 SKIP；纯工具 8/8；release check
  8/8，无 SKIP。实际物理键鼠、非 US 实机布局与 Windows 仍未验收。

### P3.1c：跨类型事件保序（2026-10-31 已实施）

- `FrameInput` 不再按类型分数组，而是一个 `std::variant` 事件序列，平台按
  `SDL_PumpEvents` 顺序追加。宿主 `NanWindow` 按同一顺序派发，不重新分组。
- 每事件仍携带自己的坐标、修饰键与键身份；帧级 `pointer` / `modifiers` 只作当前状态。
  viewport 映射在每个指针事件派发前应用，move 的 delta 仍按映射后坐标计算。
- 文本事件不再延迟到所有指针/键事件之后：同帧“点击 A → 文本 → 点击 B”中，文本到达
  点击 A 时的焦点；点击 B 之后才改变焦点。
- 不在本批：`MouseEnter` / `MouseLeave` 仍由场景命中测试派发，不是平台事件；IME 组合
  会话仍属 P3.3。
- 验收：SDL 队列测试断言跨类型顺序；新增真实 `NanWindow::tick()` 用例记录两个可聚焦
  目标的 press/release/text 顺序，要求文本只到达第一个目标。故障注入把文本改成末尾
  分组派发时该用例失败。

#### 实施与验证记录（2026-10-31）

- 实现：`app/platform_window.hpp`（`FrameEvent` 变体与 `events`）、
  `app/detail/sdl_platform_window.cpp`（按队列追加）、`app/nan_window.cpp`（`std::visit`
  顺序派发）。
- 回归：`sdl-platform-input` 11 用例/366 断言，含跨类型顺序断言与同帧焦点/文本/点击顺序；
  红测见上一节。
- Linux 验证：主套件 108/108（`notebook-data` 在作者命名空间迁移后通过）；ASan/UBSan
  unit 85 + window-lifecycle SKIP；no-RTTI 87/87；no-SQLite/headless 82 + 1 SKIP；
  纯工具 8/8；release check 8/8。
- Windows 验证（2026-10-31，bundle 同步到 `2d01fb0`）：`meson setup --reconfigure` 与
  `ninja` 0 错误；`--suite unit` **85/85**（含 `notebook-data`）；`key-lifecycle`、
  `sdl-platform-input`（11 用例，dummy 视频 + 软件渲染器）、`notebook-data` 全部通过。
  这次干净检出暴露并修复了 notebook 的三个构建缺陷（扫描根、测试命名空间、`std::println`
  终端符号），单独提交，不改变本批输入语义。


## 实施与验收

| 批次 | 交付 | 验收 |
| --- | --- | --- |
| 语义定形 | 物理/逻辑身份、修饰键、press/release/repeat、unknown 与原生诊断归属 | US 与至少一种非 US 布局的映射期望表；明确左右修饰键、小键盘及快捷键匹配策略 |
| 平台映射 | SDL 适配现有可取得的信息；其他后端若新增须独立验收 | 表驱动测试及错误映射故障注入；按下/释放身份一致；重复按键不丢失、不重复触发单次动作 |
| 事件与控件迁移 | KeyEvent 使用框架类型；删除 widget 键码定义及局部魔数 | 编辑、菜单、导航、typeahead 消费者回归；文本继续走提交事件 |
| 输入保序 | 单独小批将跨类型事件保存并按捕获顺序分发；保留每事件坐标/修饰键 | 同帧“点击 A → 文本 → 点击 B”中正文到达 A；按键/文本/释放顺序不被分组重排 |
| 真实后端对照 | 同布局下比较规范事件与用户行为 | 明确自动/模拟/实际设备覆盖；不同布局的逻辑差异应符合契约，不盲目要求所有字段相等 |

未映射输入不得伪造成另一个键。某一种身份未知但另一种有效时保留可用信息并计数；
两者都未知时采用明确的 unknown 或跳过策略，实施前写死。文本事件不因键映射失败而被丢弃。
平台 API 无法提供完整顺序或字段时记录限制，不制造时间戳、release 或物理身份来声称等价。

## 与 D1 的边界

D8 负责键身份和事件基础；D1 负责输入意图、焦点提示、文本会话及组合期间的路由/抑制规则。
D8 不增加 IME 能力承诺；D1 第一阶段默认在 D8 后接入以减少返工，但技术上不需要等待完整
D8。D1 第二阶段必须复用已有 host seam，并在实际平台上分别验收组合输入与软键盘。

---

# 各项之间的关系与排序建议

完整顺序统一见[项目进度与下一步](project_status.md#推进顺序与批次关口)，本稿不另列竞争排期。

- D7 混合首先修复；SDL3 单项来源和 shader 交付稳定后即可推进生产 SDF。
- D8 与渲染无硬依赖；先定身份和事件语义，再迁移消费者，保序另批。
- D1/D2 共用状态通知问题，但焦点提示偏好与主题外观不是同一状态；D1 不等待完整 D2。
- D3 为独立元数据批次；D5 解耦不等待身份算法或全部依赖迁移。
- D6 先定 Label 能力和绑定/测量契约，再扩展 Section 等组合入口。
- D4 平台接口已存在，后续复用它；Windows/Web 的应用支持仍需逐平台证据。

# 决定与证据归档

| 议题 | 权威归属 |
| --- | --- |
| D1 | [统一输入、焦点与文本编辑](focus_and_input.md)，设计已接受，未实施 |
| D2 | [主题方案、作者入口与状态订阅](theme_authoring_and_state.md)：首批 `8920f3a`、第 3 步 `995e6ae`、第 4 步 `4746173`、4a `fef829a`、4b `3e2542a`、4c `f383201` 已提交；4d TOML 色名复核完成并随本批提交，见本稿实施记录 |
| D3 | 已复核并随 `095336b` 提交；正式契约见 [Page / Router](page_and_router.md)，两轮验证见本稿 D3 实施记录 |
| D4 | [平台与后端契约](platform_backend_contract.md)，默认切换与 seam 已落地，逐平台验收继续 |
| D5 | [资源构建、部署与运行时定位](resource_build_and_deployment.md)，目标契约已冻结 |
| D6 | [作者 DSL 与内容测量](authoring_dsl_and_content_measurement.md)，详细契约待审阅 |
| D7 | 本稿分批方向；实现前将参数/能力契约和证据写回平台与后端 reference |
| D8 | 本稿分批方向；实现前将事件/类型契约归入输入 reference 或独立专题 |
| 依赖工程 | [第三方依赖策略](dependency_policy.md)，来源策略已接受，最小实验与机制未落地 |

故事保留当时的认识，不改写成“早已知道”；当前状态由 reference 与源码承担。
