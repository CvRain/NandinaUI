# 待决设计草案

> **这是设计工作稿。** 2026-10-17 同步 D7/D8 已接受的方向和分批验收；D2 仍待审阅，D3 方向已接受、未实施。
> 方向已接受不等于精确 API 已冻结；未定项在各批实现前定形。跨专题顺序以[项目进度与下一步](project_status.md)为准。
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
正式依据见 [焦点与输入 §5](focus_and_input.md#5-文本输入会话与软键盘)：分别记录协议/模拟测试、
指定平台组合输入、指定平台软键盘请求/反馈/遮挡三个状态，不以一个“阶段二完成”代替真实验收。

---

# D7 渲染质量与后端视觉一致性

> 方向已接受、未实施；2026-10-17 修订。采用独立批次修混合、交付生产 SDF、共享图片解码和裁剪，
> 不再用“一行修复”或“搬一下 shader”概括验收范围。

## 背景与当前实现

SDL 默认路径已经可运行，但按钮按下时白底白字、涟漪像实心圆盘、Dialog 遮罩近黑。
`render/backends/sdl_device.cpp` 只设置纹理混合，尚未设置 `SDL_SetRenderDrawBlendMode`；
这解释了半透明非纹理图元失真。修复还未落地。

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

## 图片解码器：与后端解耦，而不是给 SDL 单独补一个

已有 `IImageDecoder::decode_memory` 输入字节、输出 RGBA，`TextureCache` 负责后续上传。
目标由应用服务层提供默认 CPU 解码器，SDL/raylib 共享；平台可提供可选适配，但普通图片能力
不再由窗口后端决定。保持解码与纹理上传分工，不把 CPU 解码器塞入 GPU 设备实现。

首批覆盖 PNG/JPEG 和现有 crop/resize/tint 行为。stb_image 是候选，选库时同时确认预处理、
格式支持与许可证；不能因为能解码便假定能替换所有现有行为。其他格式按实际消费者扩展，
不自动承诺动画 GIF、WebP 或 SVG。

迁移期间保留无能力时的明确降级；重新核对 `supports_image_loading()` 是表示 CPU 服务、
纹理路径还是旧设备直载接口，避免把服务能力和设备能力混为一谈。图片批不依赖 SDF，
只需混合/纹理路径正确与解码库来源可取得，可根据应用需求提前。

## 实施顺序与验收

| 批次 | 交付 | 验收与故障注入 |
| --- | --- | --- |
| 混合 | 在设备合适的初始化边界设置混合并处理失败 | 已知背景（**测试自绘**）上的半透明矩形/圆/描边内部像素有独立预期；固定动画时间的按钮按下、涟漪中间态、Dialog 遮罩；读回在 present 之前；注册为 integration 测试；移除设置必须失败 |
| SDF 最小生产实验 | 同帧绘制多个不同尺寸/半径/颜色/描边图元，交错文字及上传纹理 | uniform 更新与布局正确，图元参数互不污染；退出自定义状态后文字/纹理正确；故意跳过恢复或参数更新会红 |
| 产物交付 | 源、固定工具信息、生成记录与 shader 产物进入可重复交付链 | 推荐交付受控产物，普通消费者无 shadercross 也能构建；维护关卡检查源与产物一致；每种格式分别记录生成、校验、加载 |
| 生产 SDF | 圆角、圆、线/描边、圆弧、阴影按现有设备契约逐项接通 | 静止与交互像素、图元 trace、分数缩放/小半径/细描边/半透明重叠；**每帧状态切换次数**与总帧时间、内存一起报告；驱动级逃生舱可用；基础回退独立验收 |
| 图片解码 | 共享服务、实际 Image 消费者、预处理行为 | PNG/JPEG、透明图片、错误输入与 crop/resize/tint；两后端一致，解码失败可定位 |
| 专用圆角交集 | 涟漪圆形与圆角矩形的交集 | 边角不外溢；缺能力时回退仍正确；不顺带宣称支持任意子树裁剪 |
| 通用圆角裁剪 | 有实际图片/子树用例后定形 clip 协议、嵌套和状态恢复 | 图像与嵌套内容、缩放、恢复、绘制/命中契约及能力缺失路径分别验证 |

SDF 只等待混合正确、SDL3 来源与 shader 交付定形，不等待整个 submodule 迁移完毕。
第一批混合回归可以复用当前固定环境，不能把依赖工程变成修复缺陷的前置。

## 实施前仍需定形

- shader 参数结构、资源绑定、alpha 约定、AA 外扩边界与坐标空间；
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

> 方向已接受、未实施；2026-10-17 修订。此前“统一物理键枚举 + 机械替换”的方案已调整为
> 分清物理位置、逻辑含义与提交文本，再迁移平台映射与消费者。精确类型/签名在首批定形。

## 当前实现

- `scene::KeyEvent` 使用 int；`widget/key_codes.hpp` 是 GLFW 码空间，SDL 临时翻译到它。
- 旧记录的 16 文件/56 处仅统计当时的 keys 引用，不包含完整事件与消费者改造；实施前重查。
- SDL 当前只转发非 repeat 的 key-down；key-up 未转发，修饰键从轮询时全局状态取得
  （SDL 的 `SDL_KeyboardEvent.mod` 已经带了**事件时**的修饰键，现在被忽略）。
- **已核实的两处事件字段丢失**（与保序批次独立，应先修）：平台把每个按钮/滚轮事件的
  `event.button.x/y`、`event.wheel.mouse_x/y` 都填进了事件，但 `NanWindow` 派发时统一用
  `frame_input_.pointer`（帧末位置）覆盖 —— 同一帧内"在 A 按下、移到 B"会把按下派发到 B。
- `FrameInput` 目前只有帧级 `pointer` 与帧级修饰键快照；按事件字段要一路带到消费者，
  不能只在平台侧填好再被宿主丢弃。
- `FrameInput` 按事件类型分多个数组，`NanWindow` 分组派发，不能保持跨类型事件原始先后顺序。
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

## 实施与验收

| 批次 | 交付 | 验收 |
| --- | --- | --- |
| 语义定形 | 物理/逻辑身份、修饰键、press/release/repeat、unknown 与原生诊断归属 | US 与至少一种非 US 布局的映射期望表；明确左右修饰键、小键盘及快捷键匹配策略 |
| 平台映射 | SDL 与 raylib/GLFW 分别适配现有可取得的信息 | 表驱动测试及错误映射故障注入；按下/释放身份一致；重复按键不丢失、不重复触发单次动作 |
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
| D2 | [主题方案、作者入口与状态订阅](theme_authoring_and_state.md)，待审阅 |
| D3 | 本稿定形分组语义，实施时同步 [Page / Router](page_and_router.md) |
| D4 | [平台与后端契约](platform_backend_contract.md)，默认切换与 seam 已落地，逐平台验收继续 |
| D5 | [资源构建、部署与运行时定位](resource_build_and_deployment.md)，目标契约已冻结 |
| D6 | [作者 DSL 与内容测量](authoring_dsl_and_content_measurement.md)，详细契约待审阅 |
| D7 | 本稿分批方向；实现前将参数/能力契约和证据写回平台与后端 reference |
| D8 | 本稿分批方向；实现前将事件/类型契约归入输入 reference 或独立专题 |
| 依赖工程 | [第三方依赖策略](dependency_policy.md)，来源策略已接受，最小实验与机制未落地 |

故事保留当时的认识，不改写成“早已知道”；当前状态由 reference 与源码承担。
