# 待决设计草案

> **这是设计工作稿。** 2026-10-20 同步：D7 的混合修复与生产 SDF（填充/描边/圆）已交付并在 Vulkan/DXIL 上验收，D8 方向已接受未实施；D2 仍待审阅，D3 方向已接受、未实施。
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
| **D8** | **键码与输入码空间的归属** | P3.1a 每事件坐标/修饰键快照与 P3.1b 物理/逻辑键身份已实施（`foundation/input.hpp`）；跨类型保序（P3.1c）与真实布局/IME 仍待后续 | 快捷键、编辑键、输入法边界；跨后端输入对照 |

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
| D2 | [主题方案、作者入口与状态订阅](theme_authoring_and_state.md)，待审阅 |
| D3 | 本稿定形分组语义，实施时同步 [Page / Router](page_and_router.md) |
| D4 | [平台与后端契约](platform_backend_contract.md)，默认切换与 seam 已落地，逐平台验收继续 |
| D5 | [资源构建、部署与运行时定位](resource_build_and_deployment.md)，目标契约已冻结 |
| D6 | [作者 DSL 与内容测量](authoring_dsl_and_content_measurement.md)，详细契约待审阅 |
| D7 | 本稿分批方向；实现前将参数/能力契约和证据写回平台与后端 reference |
| D8 | 本稿分批方向；实现前将事件/类型契约归入输入 reference 或独立专题 |
| 依赖工程 | [第三方依赖策略](dependency_policy.md)，来源策略已接受，最小实验与机制未落地 |

故事保留当时的认识，不改写成“早已知道”；当前状态由 reference 与源码承担。
