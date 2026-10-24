# 开发路线（2026-10-20 重估）

> 本文是**下一阶段 P1–P4 的排序与退出条件**。已完成批次、设计议题与历史证据仍以
> [项目进度与下一步](project_status.md) 为准；组件之间的依赖顺序见
> [组件开发路线图](component_roadmap.md)；性能和视觉的门禁细节见
> [性能预算](performance_budget.md) 与 [视觉回归](visual_regression.md)。
> 本文不承诺日期，也不把"计划"写成"已实现"。

## 1. 当前基线（截至 2026-10-20，已验收）

| 方向 | 结论 | 证据入口 |
| --- | --- | --- |
| 平台 | **Linux 正式支持**（Wayland/X11，主套件 98/98）；**Windows 实验性**（release 构建通过、`--suite unit` 80/80、showcase 可运行、DXIL 设备级验收通过）；**macOS 不支持**（无硬件，不承诺） | README“在 Windows 上构建”、路径契约 §14 |
| 渲染 | SDL3 为默认后端；生产 SDF 已接入**填充/描边/圆/线段/圆环扇区**，Vulkan(SPIR-V) 与 Windows D3D12(DXIL) 均通过设备级像素验收，直线 AA 数值逐位一致；其余图元仍走 CPU 几何 | 路径契约 §10、§14 |
| 视觉回归 | 6 条页面 golden 已批准（Linux，平台隔离）；比较器与审批工具就绪；Windows 基线未建立 | [视觉回归](visual_regression.md) |
| 性能 | release 基线 8 配置 + 确定性门禁（布局 0、稳态 miss 0、binds 上限）+ 候选计时阈值（`--enforce` 启用） | [性能预算](performance_budget.md) |
| 依赖与分发 | 10 个 wrap + 字体 submodule + SQLite/stb 入库源码；raylib 已删除；vendored SDL3 默认静态、可选共享；头文件门禁 182 个（10 internal） | [依赖策略](dependency_policy.md)、[SDK 表面](sdk_surface.md) |
| 工程门禁 | format / suite / header / shader artifact / golden manifest / backend budget；CI 无 GPU | README、路径契约 §13 |

## 2. 推进原则

1. **先定行为与边界，再写实现**：需要用户判断的未定项在实施前收敛，不把实验性约定写进公开承诺。
2. **测试必须证明会红**：每批交付要说明"改坏哪里、什么会红"；没有回归证据的修复不算完成。
3. **平台结论分开记录**：Linux 的验收不能替代 Windows/DXIL；Windows 的验收不能替代 MSL；每个平台各自有证据。
4. **不静默降级**：能力缺失要写进 `sdf_unavailable_reason()`、capability 查询或文档，而不是悄悄换路径。
5. **CI 与发布门禁分工**：无 GPU 的检查（format、header、artifact、golden 清单、预算确定性门禁）进 CI；
   需要 GPU/窗口的检查（页面 golden、性能采样、设备级像素）是发布检查或本地门禁。

## 3. 阶段划分

### P1 收口与平台补齐（当前阶段）

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P1.1 | **发布检查脚本**：把 format、header、artifact、golden 清单、budget 确定性门禁串成一条 `tools/release_check.py`（本地/发布用，CI 可复用无 GPU 部分） | 各门禁已就绪 | **已完成（2026-10-20）**：8 项逐条 PASS/SKIP/FAIL、失败非零退出；`--ci` 跳过 GPU 项，`--report` 追加性能预算；已写进[开发流程](development_loop.md) §2 |
| P1.2 | **Windows 页面 golden 候选**：`capture_backend_golden.py --platform windows` 采集 → 人工审阅 → 批准（不替换 Linux 基线） | Windows 构建可跑（已满足） | **CPU 路径已交付（2026-10-21）**：3 条 approved（home/label/anchors cpu，`--renderer sdl2d`，SSH 会话无 swapchain）；候选图带平台标签；策略文档与路线图已同步。**已完成（2026-10-22）**：Windows 6 条 approved（CPU 3 + SDF 3），SDF 路径两次采集逐字节一致；DXIL 设备级验收通过（含新增 shadow/clip 断言）。P1.3 性能基线仍为可选项 |
| P1.3 | **Windows 性能基线（可选）**：同协议在 Windows release 构建采样，作为对照而非跨平台比较 | P1.2 同前置 | **已完成（2026-10-22，参考基线）**：CPU 路径（SSH + `--renderer sdl2d`）与 SDF 路径（桌面会话 + DXIL）各 4 组配置；8 条参考基线在 `probe/cross_platform/results/backend-baseline-windows-release-2026-10-22.json`，不跨平台比较、不参与计时门禁；SDF 能力行确认软阴影/圆角裁剪生效 |
| P1.4 | **CLI 端到端**（**已移出本仓库**：CLI 由独立仓库 NandinaCLI 承担，主仓库只保留库、模板契约与框架开发工具） | — | 本仓库不再构建/测试 CLI；`new/build/run` 的端到端与 SDK 获取在 NandinaCLI 验收。2026-10-20 移除 `tools/nandina`、`nandina-cli` 集成测试与 `NANDINA_SOURCE_ROOT` 注入 |
| P1.5a | **SDF 线段 + 环形扇区**：复用 32 字节参数块（模式 2/5），修正 CPU 弧角度约定与 sweep>π 的角向公式 | 现有 SDF 参数策略与产物链 | **已完成（2026-10-20）**：`sdf-device` 覆盖内部/圆头/整环 seam/270° 缺口/立方 AA/缓存命中（misses==2→hits==2）；golden 逐字节不变；release 预算 0 失败；产物三格式再生并经固定工具链验证 |
| P1.5b | **阴影（mode 4）+ 涟漪圆角裁剪**：先做 D7 批次 6 的涟漪专用圆角交集，再按用例定形通用裁剪 | P1.5a 已落地 | **已完成（2026-10-21）**：48 字节参数块（bounds/style/extra），mode 3 = 圆 ∩ 圆角矩形、mode 4 = 圆角矩形 SDF + spread 衰减；`sdf-device` 覆盖软边解析覆盖率、矩形/圆/圆角三向裁剪、缓存命中和 CPU 降级；页面 golden 逐字节不变（当前页面没有启用阴影的卡片）；MSL 产物同步再生但未实机验收 |
| P1.6 | **图片路径**：共享 CPU 解码器、SDL RGBA 上传、删除 `IPlatformWindow::make_image_decoder()`（破坏性，按 SDK 表面记录） | 解码库来源定形 | **已完成（2026-10-20）**：vendored stb 解码（PNG/JPEG/BMP/GIF/TGA + crop/resize/tint）、`make_cpu_image_decoder()` + 应用注入、SDL `create_rgba_texture`、三项能力拆分、旧钩子删除；`image-decoder` 8 用例 + `sdl-visual` 上传像素断言；套件 98/98。**未做**：SDL 页面级 Image 端到端采样 |
| P1.7 | **raylib 完整退役**：窗口测试宿主换 SDL3；删除旧后端、工厂、枚举项、wrap 与依赖，不保留临时开关；vendored SDL3 默认静态、可选共享 | P1.6 已完成（图像路径不再依赖 raylib） | **实现与 Linux 验收已完成（2026-10-20）**：本轮非 SDK 99/99；两个离线 SDK fixture 前轮通过；静态 ASan/lld 84/84 与 GPU 设备验收通过，no-RTTI 前轮 84/84；无后端全套 94 通过、1 SKIP。原 raylib CI 改为无后端全套门禁，窗口跳过、系统 SDL DLL 搜索补充回归测试。Windows 静态模式仍须真机复验，详见[依赖策略](dependency_policy.md) |

### P2 应用闭环（资源与工具）

> 实施契约（目标依赖图、选项真值表、启动策略、lock/revision/ID 语义、子批与门禁）见
> [D5 §8](resource_build_and_deployment.md#8-p2-实施契约2026-10-22-定稿p21ap21bp22ap22dp23a-已实施)。
> **状态：P2.1a/P2.1b/P2.2a–P2.2d/P2.3a 已实施（2026-10-22 至 10-24）；P2.3b/P2.3c、P2.4 未实施。**

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P2.1a | 目标拆分：`resource_core` / 运行时后端 / `resource_build` / `nanres` + 包格式协议头 + 选项真值表（`build_ui`、`resource_sqlite`、`resource_tools`、`examples`） | 无 | **已完成（2026-10-22）**：`nandina/resource/meson.build` 四层目标，`nanres` 只链 `resource_build_dep`；`resource_package_format.hpp` 解除打包器对运行时 SQLite 后端的依赖；5 组 `build_ui=false` 组合在固定 Meson 1.7.2 下干净配置/构建/测试；checker 覆盖直接/间接 UI 注入、归档篡改、缺 introspection/response file；旧 nanres 被拒绝、旧配置因选项不存在失败 |
| P2.1b | 启动策略：默认不自动开包、注入时机、SQLite 关闭诊断、外部 `nanres` 版本/schema 兼容检查 | P2.1a | **已完成（2026-10-22）**：`resource_package` 默认空 + `mount_resource_backend()`；`resource_sqlite=disabled` 时自动开包分支不编译并给出 `backend_unavailable`；`-Dexternal_nanres` 配置期 schema/version 握手、helper 构建期二次握手；`sdk_surface` 记录破坏性变更；主套件 104/104、无 SQLite 的 UI 构建单元套件 81/81、Windows 构建与 unit 通过（需按升级说明清理一次 stale thin archive） |
| P2.2a | 后端发布代次与快照一致性（结果与代次同一临界区，失败不发布半份状态） | P2.1 | **已完成（2026-10-23）**：后端协议返回 `ResourceSnapshot`/`ResourceStreamSnapshot`（代次 + 一致性类别 + 可选句柄）；MemoryBackend 在同一锁内读取结果与代次，成功发布才推进 64 位代次且拒绝回绕；失败路径不推进、不改映射；内存/内置、SQLite 内嵌/外置与目录来源分别标注快照类别；管理器公开八入口保持裸句柄，只在内部解包。并发配对用例在拆开读锁时稳定失败，恢复后主套件 104/104；ASan 配置 83/83 + window-lifecycle SKIP（真实 GPU 窗口触发驱动侧 LSan 泄漏，非项目代码） |
| P2.2b | 管理器八入口作用域：`ResolvedResource`/`ResolvedStream`、不透明域令牌、歧义/过期错误 | P2.1 | **已完成（2026-10-23）**：八入口返回域令牌 + MountId + 发布代次 + 一致性类别 + 记录 ID + 句柄；`ResourceDomainToken` 只弱引用域并按控制块所有权比较，manager 移动保持同一域；MountId 域内不复用、耗尽抛诊断；裸 ID 扫描全部挂载并报 `ambiguous_resource_id`（消息列出冲突挂载），同一后端挂两次也不去重，后端错误不被忽略；key/别名按挂载优先级解析；新增 `resolve(...)` 只按原作用域重解析，域/挂载/代次过期返回 `expired_scope`；消费者完成编译级迁移。红测：ID 扫描短路与令牌 `lock()` 比较分别使歧义/域身份用例失败。主套件 104/104、ASan 单元 83/83 + window-lifecycle SKIP（驱动侧 LSan）、Windows 构建与 unit 通过 |
| P2.2c | 消费者迁移：FontLoader 缓存键、图片/流/主题消费者 | P2.2b | **已完成（2026-10-23）**：`ResourceScopeIdentity` + `scope_identity(...)`；FontLoader 键为作用域 + face_index，非快照与直接句柄绕过缓存，失效分 scoped/wide；TextureCache 资源键为作用域 + options，新增 `load_resolved`，异步路径在后台解析后再定键，非快照上传但不索引/保留；Image 同步/异步均使用作用域身份。先写六条红测确认旧实现失败，修复后转绿；作用域失效只影响自身 scope。验证：主套件 104/104、无 SQLite UI 单元 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8、Windows 构建与 unit 82/82 |
| P2.2d | 缓存回收与并发：有限容量淘汰 + expired 域清扫 | P2.2c | **已完成（2026-10-24）**：FontLoader 新增 `FontLoaderLimits` + LRU，访问时清扫 expired 域/weak face，淘汰不销毁外部持有 face；TextureCache 在帧边界与 trim 清扫 expired 域，新增 `invalidate(resolved)` 定向清理，保留既有 retained 容量淘汰；卸载不广播，重叠中的异步完成可交付。异步资源加载改为“后台解析 → UI 查缓存 → 后台解码 → UI 上传”，命中不再解码/上传。红测注入：禁用容量循环/短路 sweep/跳过命中检查均使用例失败。验证：主套件 104/104、无 SQLite UI 单元 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8 |
| P2.3a | 策略 key 映射与生成算法、ID 编码（版本前缀/长度编码/固定测试向量） | P2.2 | **已完成（2026-10-24）**：`[[resources]] key + path` 显式映射优先于派生 key；`derive_resource_key` 冻结前缀+相对路径规则；`derive_resource_id` 使用版本前缀 + NUL + big-endian u32 长度 + SHA-256 前 128 bit，固定三组向量；lock 按 key 匹配，内容变化保 ID、派生重命名换 ID，不同 key 同内容独立 ID；旧随机 ID 暂按同 key 保留。先改 CLI 断言确认旧实现失败，修复后转绿；复核修复扫描器把归一化槽位登记误移到 key 验证之后的顺序变化（大写名先遍历时丢失碰撞诊断），复用 `resource_scan_options` 的 media-type 规则；新增 `ResourceScanOptions::mapped_paths` 让显式映射文件不派生 key/不报诊断，路径身份统一走 `resource_path_identity`（Windows ASCII 折叠大小写），拒绝 rooted `path`；补映射错误路径、扫描根外/根内不可派生映射、同 key 保 ID/revision、旧随机 ID 保留与长度前缀测试。主套件 104/104、无 SQLite UI 单元 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8；Windows 在 `8bb7fec` 上构建 0 错误、unit 82/82，定向脚本复验映射 ID 向量/根内映射/rooted 拒绝/大小写折叠；`nanres-cli` 的 `pack` 安装步骤在 Windows 为既有失败（见 D5 §8.17） |
| P2.3b | lock 迁移（生成数据移入 build 目录）、碰撞（同 ID 不同逻辑记录）、revision、无变化不重写 | P2.3a | 碰撞注入、重建确定性、迁移与不回写源码 |
| P2.3c | 最小常量头（只含逻辑 key）+ 编译依赖测试 | P2.3b | 删除声明后使用该符号编译失败；符号冲突处理 |
| P2.4a | 示例资源链路：`examples/notes` 的 resources.toml/资产；增量发现、成套发布、定向构建资源就绪、portable stage 重定位运行 | P2.1（完整验收在 P2.2/P2.3 后） | 干净检出 + 离线可构建；目录增删/无变化不重写；失败保留旧产物但返回失败 |
| P2.4b | 示例应用功能：浏览笔记、编辑表单、预览图片、确认弹窗（导航/滚动贯穿） | P2.4a | 实际运行 + 人工交互验收证据（截图/描述） |
| P2.4c | 测试脚本与文档：构建检查与运行检查分别记录，失败可复现证据入库 | P2.4b | smoke 脚本入库；showcase 保留组件验收职责 |

### P3 输入与焦点

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P3.1 | **D8 输入基础**：先修“宿主用帧级字段覆盖每事件坐标/修饰键”，再定形物理/逻辑键、事件快照、release/repeat/unknown、保序分发 | 无 | 同帧“在 A 按下、移到 B”派发到 A；事件时修饰键；US/非 US 布局；跨后端规范事件对照 |
| P3.2 | **D1 第一阶段**：统一焦点提示策略（paint-only 失效、程序化恢复、窗口失活/激活） | P3.1 事件语义 | 同节点模态变化、两种 roving 模型、文本能力矩阵 |
| P3.3 | **D1 文本会话**：注入式 `ITextInputService`、IME/软键盘、会话生命周期 | P3.1 | 真实 IME 分别记录平台状态；不另造第二套 host |

### P4 作者与组件

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P4.1 | **D6 首批**：Label 作者能力、绑定覆盖、有限宽度自然长高与缓存契约 | 详细 API 审阅 | Label 页与 ComponentInfoTable 验收；纯绘制变动不无谓整形 |
| P4.2 | **阶段 5 数据组件**：布局/选择/虚拟化依据 | P4.1 与布局稳定 | 复合数据组件有虚拟化与选择回归 |
| P4.3 | **D2 主题状态订阅**、**D3 nav_group** | D2 通知与生命周期定形；D3 无跨专题前置 | Shell 无重复事实来源；空组/合并/顺序/typed navigation 回归 |
| P4.4 | **视觉质量扩展**：页面 golden 覆盖面（更多页面/组件状态）、混合叠层验证 | P1.1 | 新页面有 approved 基线或明确不纳入的理由 |

## 4. 建议的近期切入顺序

1. **退役后的 Windows 复验**：静态构建、窗口生命周期与 DXIL。Linux 的静态/共享 SDK、窗口与无后端验收已完成。
2. **P1.6 页面补验**：SDL 完整 Image 页面端到端采样，不能用解码/上传单测替代。
3. **P1.5b 阴影与涟漪圆角裁剪**：已完成（2026-10-21）。
4. **P1.2 Windows golden**：已完成（2026-10-22，CPU+SDF 共 6 条 approved）；P1.3 性能基线可选。
5. P2/P3/P4 按应用需求插入；D8 与 D1 在组件扩张前推进。P1.1/P1.5a/P1.6 核心实现已完成，P1.4 归 NandinaCLI。

模板唯一归属 NandinaCLI，并按兼容 SDK 版本发布。框架仅维护
[`tests/fixtures/sdk_app`](../../tests/fixtures/sdk_app/README.md) 的消费契约 fixture；
CLI 仓库负责真实模板生成与 SDK 配对验收，主仓库 fixture 不替代跨仓库测试。

## 5. 非目标（明确不做，或只保留探测）

- **macOS**：没有硬件与测试环境，不提供构建/运行承诺；MSL 产物仅静态校验。
- **Web**：探测保留，不作为应用交付路径。
- **C++ Modules / CMake package**：暂缓，条件见[构建系统与 Modules 范围](build_system_scope.md)。
- **OS 包 / 安装器 / 后端拆库**：长期项，当前没有应用安装路径。
- **跨平台像素基线共用**：不比较不同平台像素，只比较结构/调用序列。

## 6. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| CI 无 GPU，渲染回归只能靠发布检查 | 回归发现晚 | P1.1 把确定性门禁自动化；发布检查清单化 |
| Windows 无持续构建环境 | 实验性路径可能再次腐化 | 每次改动涉及跨平台代码时本地跑一次 Windows 构建 + unit；DXIL 验收记录在案 |
| golden 平台隔离带来维护成本 | 新平台基线缺失 | 按需建立（P1.2），不追求所有平台同步 |
| 文档数字随代码漂移 | 结论过期 | 门禁输出为准；本目录更新时同步索引与状态行 |
