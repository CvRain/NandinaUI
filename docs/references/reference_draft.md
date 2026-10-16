# 待决设计草案

> **这是工作稿，不是决定记录。** D2、D3、D7、D8 都仍在讨论或"方向已接受、尚未实施"，
> 不能把本稿的候选形状当作已冻结契约。
> 拍板之后：决定写进对应的 reference（`component_contract.md` / `theme` 相关 / `page_and_router.md`
> / `platform_backend_contract.md`），过程补充到 stories；本文随之删除或降级为历史草案。
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
| **D8** | **键码与输入码空间的归属** | 框架键码就是 GLFW 码（`widget/key_codes.hpp`）；SDL 平台临时把它翻译过来 | 快捷键、编辑键、输入法边界；跨后端输入对照 |

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

## 现状核实

| 事实 | 位置 |
| --- | --- |
| 页面类型 `PageKey` 是路由身份 | `RouteEntry::page_key`、`route<PageT>()`、`Routes::find()` |
| `RouteOptions::address` 不参与当前匹配 | `nan_router.hpp` 注释与 `Routes` 实现 |
| `std::string type{"default"}` 只用于导航分组 | `RouteOptions`、`Routes::nav_sections()` |
| 分组值同时成为当前 UI 分组标题，且当前无空值语义 | `NavSection::type`、`showcase/components/sidebar.cpp` |
| 分组顺序稳定，为首次出现顺序 | `Routes::nav_sections()` 与 `[app][router][nav]` 测试 |

## 初步决定（方向已接受，尚未实施）

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
- `nav_sections()` 继续按值扫描，不需要 `"default"` 特判或组名校验。sidebar 在 `nav_group.empty()`
  时跳过标题 Label；组与组之间的分隔线仍按 section 边界绘制，不能跟着标题一起消失。

## 实施与验收

按以下清单落地后，D3 可从初步决定转为已实施：

1. `RouteOptions::type` → `nav_group`、`NavSection::type` → `group`；不留旧字段/别名。更新构建器、注释、
   测试、`showcase/main_window.cpp` 中包括注释掉的占位 route，以及 Story 011 / 页面路由文档。
2. 未设置与显式空串语义相同：route 仍可见、不画分组标题；非空值精确分组并作为标题。
3. 多个未分组 route，即使在声明表中被有名分组穿插，也合并成一个无标题 section，位置由首个未分组项决定。
4. 测试 section 分隔线仍只出现在 section 边界、不出现空标题或相邻重复分隔线。
5. 已有非空分组首次出现顺序和段内顺序保持不变；隐藏 route 不进入分组。

本轮暂不把“字符串拼写可能产生意外分组”视为库级 bug；如果应用想防 typo，可在自己的 constants、
manifest、route factory 或构建生成步骤中完成，而无需所有应用承担同一套机制。

---

# D4 跨平台优先的窗口与渲染后端路线

## 目标优先级（当前讨论方向）

1. **首要目标：** 让基于 Nandina 的应用能在 Linux、Windows 和 Web 构建、启动并具备可用的
   输入、文本、资源和绘制路径。Web 不只是“SDL 能创建 canvas”，而是应用级可交付目标。
2. **平台方向候选：** SDL3 是主要候选，因为它同时提供桌面平台抽象和适用于 Emscripten 的
   2D Render API；这仍需用本项目的窗口、输入、文本和 SDF 案例验证，不据单个示例宣称全框架支持。
3. **后续可能目标：** 以本框架开发 Wayland dock/panel，甚至与 wlroots compositor 集成。它们是
   未来可组合的平台接入需求，不是首个跨平台交付的前置，也不要求框架现在依赖 wlroots。
4. **ThorVG 与窗口/图形 API 选型分离。** 当前 SDF 原语继续作为基线；只有出现 SVG、任意路径等
   具体能力缺口时，再评估引入独立矢量渲染能力。它不是 SDL3 或跨平台路线的必选项。

## 当前代码边界

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

## 初步交付顺序（待审阅，不是迁移批准）

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
正式依据见 [焦点与输入 §5](focus_and_input.md#5-文本输入会话与软键盘)：分别记录协议/模拟测试、
指定平台组合输入、指定平台软键盘请求/反馈/遮挡三个状态，不以一个“阶段二完成”代替真实验收。

---

# D7 渲染质量与后端视觉一致性

## 背景

D4 把默认后端换成 SDL3 之后，作者在 showcase 里立刻看到两个现象：

1. **边缘明显锯齿**，圆形尤其明显，不如 raylib 细腻；
2. **按钮"激活后白底白字"**，涟漪过渡不自然。

第 2 条后来定位为**缺陷**而不是设计取舍（见下），第 1 条是当前 SDL 设备实现方式的必然结果。

## 现状核实（2026-10-16，读代码）

| | raylib 设备 | SDL 设备（当前） |
| --- | --- | --- |
| 图元实现 | SDF fragment shader | CPU 三角形扇 |
| 边缘 | `fwidth` + `smoothstep` **解析抗锯齿**（任意半径平滑） | 硬边多边形，`segments_for(r)=clamp(ceil(r*0.75), 6, 48)` |
| MSAA | 不需要（解析 AA） | **没有**：SDL 2D GPU 渲染器不公开 sample count |
| 混合 | 正常 | **几何绘制未启用混合** |

**缺陷（必修，1 行）**：`render/backends/sdl_device.cpp` 只给**纹理**设了
`SDL_SetTextureBlendMode(..., SDL_BLENDMODE_BLEND)`，**从未**调用 `SDL_SetRenderDrawBlendMode`。
于是所有几何图元的 alpha 不参与混合，一律按不透明画。三处症状同一个根因：

| 症状 | 路径 | 不混合时 |
| --- | --- | --- |
| 按钮"激活后白底白字" | `Button::on_draw`：容器 → **状态层**（filled 时是 `on_accent` 白 @0.10/0.20）→ 标签 | 状态层被画成实白盖住深色容器，标签仍是白色 |
| 涟漪过渡不自然 | `RipplePainter`：`style.color.with_alpha(alpha)` 随进度衰减 | 低透明度圆被画成实心硬边圆盘，不随进度淡出 |
| 对话框遮罩"近黑" | `DismissLayer::on_draw` → `BoxPainter::paint(scrim)` | 半透明遮罩变不透明（实测 mean 5.9） |

**为什么现有验证没抓到**：交互回归断言的是**状态位**（hovered/pressed/treatment/route），不看像素；
跨后端像素对照取的是**静止帧**，那时状态层 alpha=0 而 `BoxPainter::paint_fill` 对 `alpha<=0` 直接
return（`box_painter.hpp:38`）。→ **D7 的验收必须包含"按下态"的像素对照**，不能只对静止帧。

涟漪还有**第二条**差异（非缺陷）：SDL 设备 `supports_rounded_clip()=false`，涟漪退化成"矩形裁剪 +
未裁剪圆"（`ripple_painter.hpp` 的降级分支），圆角处按方角溢出。混合修好之后观感会明显改善，
但要完全对齐 raylib 仍需 SDF 或真正的圆角裁剪实现。

## 选项

| 选项 | 质量 | 成本 | 判断 |
| --- | --- | --- | --- |
| **A. 把现有 SDF 搬到 `SDL_GPURenderState`** | 与 raylib 同级（解析 AA） | 中；需要 per-platform 着色器产物 | **推荐**：这是生产路径 |
| B. CPU 侧 analytic AA（羽化 / 超采样） | 一般；细描边与阴影做不好 | 中高，且随面积增长 | 只作过渡，不做长期投入；SDF 落地后作废 |
| C. 让 2D GPU 渲染器开 MSAA | — | — | **不成立**：SDL3 无公开 API |

选项 A 现在成本最低的三个理由：

1. **逐图元 trace 已证明两个后端的调用序列逐项相同**（Home 675/675 texture、Label 2555/2555 texture +
   10/10 圆角填充 + 11/11 圆角描边、Anchors 81/81）：要换的只是 `IRenderDevice` **之下**的实现，
   组件、布局、painter 一行都不动；
2. **着色器产物链已打通并验证**：`tools/verify_shadercross.sh` 生成 SPIR-V/DXIL/MSL，且 SPIR-V 被工具链里
   独立的 `spirv-val` 接受 —— 正好是 `SDL_GPURenderState` 需要的格式（本机头文件确认该 API 自 SDL 3.4 起，
   接受三种着色器格式）；
3. **顶点格式兼容**：SDL_GPURenderState 走 `SDL_Vertex`（position + color + uv），几何路径已经在用。

## 图片解码器：与后端解耦，而不是给 SDL 单独补一个

SDL 平台现在 `make_image_decoder()` 返回 nullptr，所以 `Image` 在 SDL 上不加载图片。这不是 SDL 的限制：
解码是 **CPU 侧服务**，早已抽象为 `render::IImageDecoder` + `TextureCache`。

**候选决定**：实现**一个**与后端无关的解码器（stb_image 单文件，覆盖 PNG/JPEG/BMP/GIF/TGA），
raylib 与 SDL 共用；把 `Image` 现有的 `supports_image_loading()` 能力查询保留为"消费者侧明确降级"。
只有当确实需要 WebP/SVG 时才评估 SDL_image。这样"图片加载"不再是任何后端的特性。

## 初步决定（方向已接受，尚未实施）

1. **先修混合模式缺陷**（1 行 + 一条**按下态**会红的回归）。它污染所有半透明绘制，也会污染后续 AA 的验收；
2. **抗锯齿走选项 A**（SDF + `SDL_GPURenderState`），不做 CPU AA 的长期投入；
3. **图片解码器与后端解耦**，一个解码器两个后端共用；
4. **SDL 上补真正的圆角裁剪**（涟漪、图片/头像溢出），排在 AA 之后。

## 实施顺序与验收

| 步 | 内容 | 验收（会红的标准） |
| --- | --- | --- |
| 1 | 启用几何混合 | 按下态像素对照：探针在 hover/pressed 帧截图，与 raylib 基线比；把 `SDL_SetRenderDrawBlendMode` 那行删掉必须变红。同时修正探测记录里"遮罩近黑"的旧解释 |
| 2 | 依赖与着色器产物打包（依赖 [依赖策略](dependency_policy.md)） | 三种格式产物随包可用；无 shadercross 的机器也能构建 |
| 3 | SDF + `SDL_GPURenderState` | 同页像素对照的**角部 AA 差异消失**（当前 Anchors max 55、Label max 9 都来自硬边）；逐图元 trace 不变 |
| 4 | 圆角裁剪 | 涟漪在圆角容器内不外溢：一张截图 + 一条能力为 false 时的降级测试 |

## 待决问题

1. 着色器产物是**构建期生成**（需要 shadercross 作为构建机依赖）还是**入库**（配 `--check` 关卡）？
   倾向后者，理由与 D4 P1 的"host tool 是构建机依赖"约束一致。
2. SDF 与 CPU 几何是否长期共存（例如小尺寸用 CPU、大圆角用 SDF）？**倾向不共存**：两条路径会立刻
   在 AA 与混合语义上分叉。
3. Windows 上是否同时接 D3D12（DXIL 产物已能生成，但**加载**仍未验证）。属 D4 的未定项，不在 D7 内解决。

---

# D8 键码与输入码空间

## 背景

框架当前的键码就是 **GLFW 码**：`widget/key_codes.hpp` 开头写着"这些值是 GLFW 的键码（raylib 直接透传）"。
SDL 后端因此必须把 `SDL_Keycode` 翻译回 GLFW 码空间（要做字母大小写重映射、命名键映射表，未覆盖的键
计数跳过）。这是"用某一家的编号当框架契约"，也是 D4 契约里最后一条未收口的项（"规范键值归属"）。

## 现状核实

- `scene::KeyEvent` 携带 `int keycode`；widget 侧通过 `widget::keys::*` 常量使用（16 个文件、56 处引用）；
- SDL 平台的映射表已实现但有已知缺口（`{|}~`、小键盘、非 US 布局、媒体键、PrintScreen 等被跳过并计数）；
- 文本输入独立于键码：`TextInputEvent` 携带已合成的 UTF-8，不经过键码空间。

## 初步决定（方向已接受，尚未实施）

1. **框架自己拥有语义键码**：`nandina::input::Key` 枚举，值与任何厂商无关；
2. **映射归平台层**：每个后端一个 mapper（`SDL_Keycode → Key`、GLFW code → Key、将来 Web
   `KeyboardEvent.code → Key`）。位置刚好是 D4 建立的 `IPlatformWindow` seam；
3. **`scene::KeyEvent` 携带 `Key`**，另留 `native_code`（诊断、录制、极端快捷键用），
   不让厂商编码出现在组件签名里；
4. **`widget/key_codes.hpp` 并入 `input/key.hpp`**；56 处引用机械替换；
5. **枚举语义以物理位置为主**：`Key::z` 指键盘上 Z 所处的位置（SDL scancode / 浏览器 `event.code`），
   与布局无关，快捷键因而稳定；文本继续走 `TextInputEvent`；编辑键（Home/End/方向键）两种语义等价。

## 实施与验收

| 步 | 内容 | 验收 |
| --- | --- | --- |
| 1 | 定义 `input::Key` + 两个平台的 mapper + 未映射计数 | 表驱动的映射测试：已知键双向一致；未映射键**不产生事件**且计数增加 |
| 2 | `KeyEvent` 改携带 `Key`（保留 `native_code`） | 编辑/导航控件的现有测试改用 `Key` 后全绿；故障注入：把一个映射项改错必须让对应测试变红 |
| 3 | 迁移 56 处 `keys::` 引用 | 编译通过 + 上述测试；`widget/key_codes.hpp` 删除 |
| 4 | 跨后端输入对照 | 同一按键序列在 raylib 与 SDL 下产生**相同的 `Key` 序列**（可复用逐图元 trace 的思路做输入 trace） |

## 待决问题

1. **物理 vs 逻辑**需要最终拍板（本文按物理位置拟定）。这是用户可见行为：AZERTY 上 `Ctrl+Z` 的物理位置
   与标注字母不同；若选逻辑语义，则快捷键随布局变化。
2. 是否需要 `Key::unknown` 之外的"位置未知但字符已知"的表达？倾向不要：那种情况属于文本输入，不属于键码。
3. 输入法/组合期间的键码抑制规则。属 D1 第二阶段的文本服务范围，本文只保证不与它冲突。

---

# 各项之间的关系与排序建议

- **D1 已定案**（见 [focus_and_input.md](focus_and_input.md)），本文不再保留它的选项表。
  它留给 D2 的交集只剩一条：D1 的"用户始终显示焦点提示"偏好需要一个运行时更新通道，
  形状与 D2 相同（但偏好不是外观，两者不要合并成一条信号）。
- **D3** 的语义方向已接受、尚未实施：`nav_group` 空值是不分组，非空值兼作标题；改名不保留 `type` 兼容字段。
  改动仅涉及路由元数据、section 分组/呈现与对应测试。
- **D4** 先追求 Linux / Windows / Web 的应用级交付，SDL3 是主要候选平台层，但渲染路径仍待 PoC；它不是 D1 第一阶段的前置，也不要求现在实现 shell/compositor 集成。
- D2/D3/D4 不必按一条功能依赖链串行。D3 的元数据 API 审阅可独立完成；D4 P0 是独立探测，启动时间取决于项目近期开发优先级。
- ⚠️ D1 第二阶段以相应平台能力验收为准，不写死依赖 D4/P1；D4 其他后端选型仍是待验证工作稿，
  本次仅修正交付依赖，不代表这些选型已获确认。

# 拍板之后

| 决定 | 归档到哪里 |
| --- | --- |
| D1 | ✅ 已归档为 [focus_and_input.md](focus_and_input.md)（并已同步 `component_contract.md` / `component_roadmap.md` / `project_status.md`） |
| D2 | `theme` 相关 reference（或 `design_tokens.md` 里的解析链路一节）+ stories/010 |
| D3 | `page_and_router.md` + stories/011 |
| D4 | ✅ 已归档为 [platform_backend_contract.md](platform_backend_contract.md)（方向已定：完成计划内容后替换为 SDL3；证据见 [跨平台后端探测](cross_platform_backend_probe.md)，过程见 [story 015](../../stories/015-the-probe-that-was-not-in-version-control.md)） |
| D5 | ✅ 已冻结为 [resource_build_and_deployment.md](resource_build_and_deployment.md)，过程见 [story 013](../../stories/013-resource-scope-before-identity.md)；实施细节按批次定形 |
| D7 | 抗锯齿/混合/圆角裁剪进 [平台与后端契约](platform_backend_contract.md) 的差异表与验收；SDF 路径的取舍补一条 story |
| D8 | 键码语义进 [统一输入、焦点与文本编辑](focus_and_input.md)（或新 reference），迁移过程补一条 story |
| 本文 | D2/D3/D7/D8 拍板后删除或降级为历史记录；若中途放弃某项，把"为什么不做"补进对应 story |
