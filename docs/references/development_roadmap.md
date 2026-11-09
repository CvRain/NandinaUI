# 开发路线（2026-10-20 重估）

> 本文记录 **P1–P4 的排序与退出条件，当前推进 P3 输入与焦点**。已完成批次、设计议题与历史证据仍以
> [项目进度与下一步](project_status.md) 为准；组件之间的依赖顺序见
> [组件开发路线图](component_roadmap.md)；性能和视觉的门禁细节见
> [性能预算](performance_budget.md) 与 [视觉回归](visual_regression.md)。
> 本文不承诺日期，也不把"计划"写成"已实现"。

## 1. 当前基线（截至 2026-10-28；沿用已归档验收证据）

| 方向 | 结论 | 证据入口 |
| --- | --- | --- |
| 平台 | **Linux 正式支持**（Wayland/X11，主套件 98/98）；**Windows 实验性**（release 构建通过、`--suite unit` 80/80、showcase 可运行、DXIL 设备级验收通过）；**macOS 不支持**（无硬件，不承诺） | README“在 Windows 上构建”、路径契约 §14 |
| 渲染 | SDL3 为默认后端；生产 SDF 已接入**填充/描边/圆/线段/圆环扇区**，Vulkan(SPIR-V) 与 Windows D3D12(DXIL) 均通过设备级像素验收，直线 AA 数值逐位一致；其余图元仍走 CPU 几何 | 路径契约 §10、§14 |
| 视觉回归 | Linux 6 条页面 golden 已批准；Windows 6 条已批准（CPU 3 + SDF 3，2026-10-22），平台隔离；比较器与审批工具就绪 | [视觉回归](visual_regression.md) |
| 性能 | Linux release 基线 8 配置 + 确定性门禁（布局 0、稳态 miss 0、binds 上限）+ 候选计时阈值（`--enforce` 启用）；Windows CPU/SDF 各 4 组参考基线已归档，不参与跨平台比较或计时门禁 | [性能预算](performance_budget.md) |
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

### P1 收口与平台补齐（已交付；剩余平台边界单列）

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P1.1 | **发布检查脚本**：把 format、header、artifact、golden 清单、budget 确定性门禁串成一条 `tools/release_check.py`（本地/发布用，CI 可复用无 GPU 部分） | 各门禁已就绪 | **已完成（2026-10-20）**：8 项逐条 PASS/SKIP/FAIL、失败非零退出；`--ci` 跳过 GPU 项，`--report` 追加性能预算；已写进[开发流程](development_loop.md) §2 |
| P1.2 | **Windows 页面 golden**：`capture_backend_golden.py --platform windows` 采集 → 人工审阅 → 批准（不替换 Linux 基线） | Windows 构建可跑（已满足） | **已完成（2026-10-22）**：Windows 6 条 approved（home/label/anchors，CPU 3 + SDF 3）；CPU 路径在 SSH 会话以 `--renderer sdl2d` 采集，SDF 路径两次采集逐字节一致；DXIL 设备级验收通过（含新增 shadow/clip 断言） |
| P1.3 | **Windows 性能参考基线**：同协议在 Windows release 构建采样，作为对照而非跨平台比较 | P1.2 同前置 | **已完成（2026-10-22，参考基线）**：CPU 路径（SSH + `--renderer sdl2d`）与 SDF 路径（桌面会话 + DXIL）各 4 组配置；8 条参考基线在 `probe/cross_platform/results/backend-baseline-windows-release-2026-10-22.json`，不跨平台比较、不参与计时门禁；SDF 能力行确认软阴影/圆角裁剪生效 |
| P1.4 | **CLI 端到端**（**已移出本仓库**：CLI 由独立仓库 NandinaCLI 承担，主仓库只保留库、模板契约与框架开发工具） | — | 本仓库不再构建/测试 CLI；`new/build/run` 的端到端与 SDK 获取在 NandinaCLI 验收。2026-10-20 移除 `tools/nandina`、`nandina-cli` 集成测试与 `NANDINA_SOURCE_ROOT` 注入 |
| P1.5a | **SDF 线段 + 环形扇区**：复用 32 字节参数块（模式 2/5），修正 CPU 弧角度约定与 sweep>π 的角向公式 | 现有 SDF 参数策略与产物链 | **已完成（2026-10-20）**：`sdf-device` 覆盖内部/圆头/整环 seam/270° 缺口/立方 AA/缓存命中（misses==2→hits==2）；golden 逐字节不变；release 预算 0 失败；产物三格式再生并经固定工具链验证 |
| P1.5b | **阴影（mode 4）+ 涟漪圆角裁剪**：先做 D7 批次 6 的涟漪专用圆角交集，再按用例定形通用裁剪 | P1.5a 已落地 | **已完成（2026-10-21）**：48 字节参数块（bounds/style/extra），mode 3 = 圆 ∩ 圆角矩形、mode 4 = 圆角矩形 SDF + spread 衰减；`sdf-device` 覆盖软边解析覆盖率、矩形/圆/圆角三向裁剪、缓存命中和 CPU 降级；页面 golden 逐字节不变（当前页面没有启用阴影的卡片）；MSL 产物同步再生但未实机验收 |
| P1.6 | **图片路径**：共享 CPU 解码器、SDL RGBA 上传、删除 `IPlatformWindow::make_image_decoder()`（破坏性，按 SDK 表面记录） | 解码库来源定形 | **已完成（2026-10-20）**：vendored stb 解码（PNG/JPEG/BMP/GIF/TGA + crop/resize/tint）、`make_cpu_image_decoder()` + 应用注入、SDL `create_rgba_texture`、三项能力拆分、旧钩子删除；`image-decoder` 8 用例 + `sdl-visual` 上传像素断言；套件 98/98。**未做**：SDL 页面级 Image 端到端采样 |
| P1.7 | **raylib 完整退役**：窗口测试宿主换 SDL3；删除旧后端、工厂、枚举项、wrap 与依赖，不保留临时开关；vendored SDL3 默认静态、可选共享 | P1.6 已完成（图像路径不再依赖 raylib） | **实现与 Linux 验收已完成（2026-10-20）**：本轮非 SDK 99/99；两个离线 SDK fixture 前轮通过；静态 ASan/lld 84/84 与 GPU 设备验收通过，no-RTTI 前轮 84/84；无后端全套 94 通过、1 SKIP。原 raylib CI 改为无后端全套门禁，窗口跳过、系统 SDL DLL 搜索补充回归测试。Windows 静态模式仍须真机复验，详见[依赖策略](dependency_policy.md) |

### P2 应用闭环（资源与工具，已交付）

> 实施契约（目标依赖图、选项真值表、启动策略、lock/revision/ID 语义、子批与门禁）见
> [D5 §8](resource_build_and_deployment.md#8-p2-实施契约2026-10-22-定稿p21ap24c-已实施)。
> **状态：P2.1a/P2.1b/P2.2a–P2.2d/P2.3a–P2.4c 已实施（2026-10-22 至 10-25）；P2 应用闭环完成，后续进入 P3。**
> **2026-10-28 方向调整**：示例应用由 P2.4 的 `examples/lab` 改造为 `examples/notebook`（综合笔记软件）。
> 数据层（SQLite + 资源驱动迁移 + 仓储）已落地；notebook 的服务与 UI、showcase 组件由维护者手工编写，
> 不属于本轮框架实现代写范围。UI 测试在本地未跟踪的 `playground/`，仓库内保留数据层等确定性测试；P2.4 的验收记录保留为历史证据。

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P2.1a | 目标拆分：`resource_core` / 运行时后端 / `resource_build` / `nanres` + 包格式协议头 + 选项真值表（`build_ui`、`resource_sqlite`、`resource_tools`、`examples`） | 无 | **已完成（2026-10-22）**：`nandina/resource/meson.build` 四层目标，`nanres` 只链 `resource_build_dep`；`resource_package_format.hpp` 解除打包器对运行时 SQLite 后端的依赖；5 组 `build_ui=false` 组合在固定 Meson 1.7.2 下干净配置/构建/测试；checker 覆盖直接/间接 UI 注入、归档篡改、缺 introspection/response file；旧 nanres 被拒绝、旧配置因选项不存在失败 |
| P2.1b | 启动策略：默认不自动开包、注入时机、SQLite 关闭诊断、外部 `nanres` 版本/schema 兼容检查 | P2.1a | **已完成（2026-10-22）**：`resource_package` 默认空 + `mount_resource_backend()`；`resource_sqlite=disabled` 时自动开包分支不编译并给出 `backend_unavailable`；`-Dexternal_nanres` 配置期 schema/version 握手、helper 构建期二次握手；`sdk_surface` 记录破坏性变更；主套件 104/104、无 SQLite 的 UI 构建单元套件 81/81、Windows 构建与 unit 通过（需按升级说明清理一次 stale thin archive） |
| P2.2a | 后端发布代次与快照一致性（结果与代次同一临界区，失败不发布半份状态） | P2.1 | **已完成（2026-10-23）**：后端协议返回 `ResourceSnapshot`/`ResourceStreamSnapshot`（代次 + 一致性类别 + 可选句柄）；MemoryBackend 在同一锁内读取结果与代次，成功发布才推进 64 位代次且拒绝回绕；失败路径不推进、不改映射；内存/内置、SQLite 内嵌/外置与目录来源分别标注快照类别；管理器公开八入口保持裸句柄，只在内部解包。并发配对用例在拆开读锁时稳定失败，恢复后主套件 104/104；ASan 配置 83/83 + window-lifecycle SKIP（真实 GPU 窗口触发驱动侧 LSan 泄漏，非项目代码） |
| P2.2b | 管理器八入口作用域：`ResolvedResource`/`ResolvedStream`、不透明域令牌、歧义/过期错误 | P2.1 | **已完成（2026-10-23）**：八入口返回域令牌 + MountId + 发布代次 + 一致性类别 + 记录 ID + 句柄；`ResourceDomainToken` 只弱引用域并按控制块所有权比较，manager 移动保持同一域；MountId 域内不复用、耗尽抛诊断；裸 ID 扫描全部挂载并报 `ambiguous_resource_id`（消息列出冲突挂载），同一后端挂两次也不去重，后端错误不被忽略；key/别名按挂载优先级解析；新增 `resolve(...)` 只按原作用域重解析，域/挂载/代次过期返回 `expired_scope`；消费者完成编译级迁移。红测：ID 扫描短路与令牌 `lock()` 比较分别使歧义/域身份用例失败。主套件 104/104、ASan 单元 83/83 + window-lifecycle SKIP（驱动侧 LSan）、Windows 构建与 unit 通过 |
| P2.2c | 消费者迁移：FontLoader 缓存键、图片/流/主题消费者 | P2.2b | **已完成（2026-10-23）**：`ResourceScopeIdentity` + `scope_identity(...)`；FontLoader 键为作用域 + face_index，非快照与直接句柄绕过缓存，失效分 scoped/wide；TextureCache 资源键为作用域 + options，新增 `load_resolved`，异步路径在后台解析后再定键，非快照上传但不索引/保留；Image 同步/异步均使用作用域身份。先写六条红测确认旧实现失败，修复后转绿；作用域失效只影响自身 scope。验证：主套件 104/104、无 SQLite UI 单元 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8、Windows 构建与 unit 82/82 |
| P2.2d | 缓存回收与并发：有限容量淘汰 + expired 域清扫 | P2.2c | **已完成（2026-10-24）**：FontLoader 新增 `FontLoaderLimits` + LRU，访问时清扫 expired 域/weak face，淘汰不销毁外部持有 face；TextureCache 在帧边界与 trim 清扫 expired 域，新增 `invalidate(resolved)` 定向清理，保留既有 retained 容量淘汰；卸载不广播，重叠中的异步完成可交付。异步资源加载改为“后台解析 → UI 查缓存 → 后台解码 → UI 上传”，命中不再解码/上传。红测注入：禁用容量循环/短路 sweep/跳过命中检查均使用例失败。验证：主套件 104/104、无 SQLite UI 单元 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8 |
| P2.3a | 策略 key 映射与生成算法、ID 编码（版本前缀/长度编码/固定测试向量） | P2.2 | **已完成（2026-10-24）**：`[[resources]] key + path` 显式映射优先于派生 key；`derive_resource_key` 冻结前缀+相对路径规则；`derive_resource_id` 使用版本前缀 + NUL + big-endian u32 长度 + SHA-256 前 128 bit，固定三组向量；lock 按 key 匹配，内容变化保 ID、派生重命名换 ID，不同 key 同内容独立 ID；旧随机 ID 暂按同 key 保留。先改 CLI 断言确认旧实现失败，修复后转绿；复核修复扫描器把归一化槽位登记误移到 key 验证之后的顺序变化（大写名先遍历时丢失碰撞诊断），复用 `resource_scan_options` 的 media-type 规则；新增 `ResourceScanOptions::mapped_paths` 让显式映射文件不派生 key/不报诊断，路径身份统一走 `resource_path_identity`（Windows ASCII 折叠大小写），拒绝 rooted `path`；补映射错误路径、扫描根外/根内不可派生映射、同 key 保 ID/revision、旧随机 ID 保留与长度前缀测试。主套件 104/104、无 SQLite UI 单元 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8；Windows 在 `8bb7fec` 上构建 0 错误、unit 82/82，定向脚本复验映射 ID 向量/根内映射/rooted 拒绝/大小写折叠；Windows 打包曾因数据库句柄/CRLF 指纹失败，已由 D5 §8.18 修复，`nanres-cli`/`nanres-build-workflow`/`portable-stage` 通过 |
| P2.3b | lock 迁移（生成数据移入 build 目录）、碰撞（同 ID 不同逻辑记录）、revision、无变化不重写 | P2.3a | **已完成（2026-10-24）**：`nanres --lock` 统一 lock 路径，helper 写到 `<output>.lock.toml`，源码树不再生成 lock；普通 scan/validate/pack 遇到旧随机 ID 失败并提示 `scan --rewrite-lock`，仅显式 flag（scan）重写；加载 lock 对同 ID 不同 key / 同 key 不同 ID 列出冲突；独立目录重建得到逐字节相同 lock；无变化跳过 lock 与 location 描述写入。先加迁移/碰撞/确定性断言确认旧实现失败，修复后转绿；fixture lock/expected ID 已迁移。复核补：`--lock`/`--rewrite-lock` 仅在 manifest 模式生效（原始扫描覆盖下报用法错误），lock 包不匹配先于迁移检查报错，新增同 key 不同 ID 碰撞与 legacy validate 迁移断言。验证：主套件 104/104、无 SQLite UI 81/81（+1 SKIP）、纯工具 7/7、ASan 83/83 + window SKIP、no-RTTI 84/84、release check 8/8；Windows（bundle 同步）构建 0 错误、packaging 3/3、unit 82/82 |
| P2.3c | 最小常量头（只含逻辑 key）+ 编译依赖测试 | P2.3b | **已完成（2026-10-25）**：`nanres keys --header` 从 policy + scan 生成 `inline constexpr std::string_view`；只含规范 key 与别名，不含 ID/摘要/源路径/安装位置。符号规则 `RESOURCE_*`，包级命名空间可逆转义；符号碰撞列出双方并失败，不静默改名。删除 `[[resources]]` 后重生成移除符号，消费者编译失败；无变化不重写头文件。新增真实 C++ 编译 integration 测试，并先注入碰撞检查/无变化写入失败确认红测；复核补包名 `.`/`-`/`_` 转义编译断言、空 policy 与源路径缺失断言。验证：主套件 105/105、无 SQLite UI 81/81（+1 SKIP）、no-RTTI 84/84、纯工具 8/8、ASan 83/83 + window SKIP、release check 8/8；Windows（bundle 同步）构建 0 错误、unit 82/82、`nanres-keys` 与 packaging 测试 4/4 通过 |
| P2.4a | 示例资源链路：`examples/lab` 的 resources.toml/资产；增量发现、成套发布、定向构建资源就绪、portable stage 重定位运行 | P2.1（完整验收在 P2.2/P2.3 后） | **已完成（2026-10-25）**：单可执行多路由示例；`nanres_build_helper build/keys` 产出资源包与常量头，`link_depends` + generated header source 使定向构建就绪；新增 `example-lab-incremental` 覆盖新增/删除资源、无变化不重写、失败保留旧包；`example-lab-run` 先用构建树包运行 Preview，再 staging → 移动后四路由运行并断言无错误日志；干净 `buildDir-examples`（离线、`-Dshowcase=false`；仅为验收证据，不是独立配置要求）构建通过 |
| P2.4b | 示例应用功能：登录/注册、番茄钟、笔记列表/编辑、预览图片、确认弹窗（导航/滚动贯穿） | P2.4a | **已完成（2026-10-25）**：Notes 起始页 + Account/Workspace 分组导航；登录/注册本地校验与 demo 预填；笔记 keyed list、TextArea 长文、保存/新增/删除与 AlertDialog；Focus 25/5 帧驱动计时（启动/暂停/重置/切换）；Preview 从 `res://` 加载生成常量 key；交互验收证据入库 `docs/images/lab/`（登录后会话行、长文编辑、删除弹窗、计时器、预览），合成指针（ydotool）+ 截图采集覆盖导航、笔记选择、新增/删除确认、倒计时/跳过、登录→Notes 状态传递；键盘文本输入未自动化，表单提交走 demo 预填 |
| P2.4c | 测试脚本与文档：构建检查与运行检查分别记录，失败可复现证据入库 | P2.4b | **已完成（2026-10-25）**：`tests/example_lab_test.py` 分 build/run/incremental；`example-lab-build`、`example-lab-run`（无 display 返回 SKIP）、`example-lab-incremental` 入库；`examples/lab/README.md` 记录构建、资源链、portable stage、测试钩子与边界；showcase 保留组件视觉验收职责。复核补：run 断言错误标记并覆盖构建树+四路由、SQL `ORDER BY`/`closing`、lab-store 随 `build_ui` 注册、外部 nanres `keys` 握手、helper 跨盘符回退、Notes 选中同步与 demo invalid 清理。验证：主套件 109/109、release check 8/8；干净离线配置与定向重建通过；单元矩阵 ASan 84/84（+SKIP）、no-RTTI 85/85、no-SQLite 82/82（+1 SKIP），no-RTTI 下完整示例 app 亦编译通过。Windows（bundle 同步）：构建 0 错误（lab exe/包/头齐备）、unit 83/83、lab build/incremental 通过、资源 4/4、run 无 display 为 SKIP |

### P3 输入与焦点（当前阶段）

当前批次按维护者安排只做 Linux 验证；Windows 调试设备可用后另行通知并恢复对应验收。
已有 Windows 历史证据保留，不作为本批变更的验证结果，也不阻塞当前 Linux 施工。

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P3.1a | **D8 每事件坐标与修饰键快照**（已实施，2026-10-29）：按钮/滚轮使用事件坐标，经 viewport 映射派发；SDL 键盘使用事件 mod，鼠标使用按队列推进的修饰键状态；帧级状态保留 | 无 | `sdl-platform-input` 4 用例/160 断言通过，覆盖 A 处点击/滚轮后移动到 B、2×/letterbox、同帧与跨帧修饰键及过滤事件；红测与验证矩阵见 [D8](reference_draft.md#p31a事件快照2026-10-28) |
| P3.1b | **D8 键身份与键生命周期**（已实施，2026-10-29）：`LogicalKey`/`PhysicalKey`、press/repeat/release、独立 unknown 计数，SDL scancode/keycode 映射，全部消费者迁移 | 精确键契约已定形 | `sdl-platform-input` 覆盖合成布局差异、左右修饰键、小键盘、unknown 与身份稳定；`key-lifecycle` 覆盖重复/单次激活；红测与验证矩阵见 [D8](reference_draft.md#p31b键身份与生命周期2026-10-29-已实施)。实机布局与 Windows 未验收；不并入 P3.1a |
| P3.1c | **D8 跨类型事件保序**（已实施，2026-10-31）：`FrameInput::events` 按平台队列顺序保存变体事件，宿主顺序派发，不再按类型分组 | 有序事件契约已定形 | 同帧“点击 A → 文本 → 点击 B”中文本到达 A；SDL 顺序断言与真实 `NanWindow::tick()` 回归通过，红测见 [D8](reference_draft.md#p31c跨类型事件保序2026-10-31-已实施)。Windows 已补验（2026-10-31：构建 0 错误、unit 85/85、输入测试通过） |
| P3.2a | **D1 焦点核心与提示策略（已实施，2026-10-31）**：`InputModality`/请求上下文、`should_show_focus_hint()`、窗口失活/激活、paint-only、组件绘制迁移 | P3.1 事件语义 | `focus-hint` 8 用例/45 断言 + 真实窗口激活与 button 绘制用例通过；红测见 [D1 §6](focus_and_input.md#第一阶段实施记录2026-10-31p32a-核心批次) |
| P3.2b | **D1 第一阶段收尾（已实施，2026-11-01）**：Showcase 输入与焦点页、Tabs 活动项提示、弹窗进入/恢复显式原因、只读字段能力与“始终显示”开关 | P3.2a | `input-focus-showcase` 构建/布局回归与页面 probe 渲染通过；`focus-hint`/`tabs`/`dialog` 回归覆盖同节点模态、恢复与只读能力；红测见 [D1](focus_and_input.md#第一阶段实施记录2026-10-31p32a-核心批次)。真实设备与 IME 仍待验收 |
| P3.3a | **D1 文本会话协议与 SDL 合成路径（已实施，2026-11-01）**：`ITextInputService`/`ITextInputTarget`、会话身份与迟到过滤、committed 唯一路径、SDL_TEXT_EDITING 入帧事件 | P3.2 事件语义 | `text-input-session` 10 用例/48 断言、`sdl-platform-input` 16 用例/417 断言与真实窗口 tick 用例通过；预编辑绘制由 P3.3b1、caret 区域由 P3.3b2a 补齐，真实平台验收留给 P3.3b2b |
| P3.3b1 | **D1 预编辑与密码用途显示（已实施，2026-11-02）**：`EditableText` 视觉文本映射、inline preedit/下划线、password purpose 掩码、组合期占位符抑制、指针命中回映射 | P3.3a | `widget` 新增组合/掩码用例通过；红测：关闭掩码后密码用例失败；真实 IME 归 P3.3b2b |
| P3.3b2a | **D1 caret 输入区域（已实施，2026-11-02）**：编辑目标局部 caret → 外层控件/viewport → 窗口逻辑坐标；SDL 声明 `input_area` 并转发 | P3.3b1 | mock 断言祖先/viewport/letterbox/滚动后的窗口坐标；SDL GetTextInputArea 回读与 resize 回归；无能力保持 unsupported；无 widget 平台旁路 |
| P3.3b2b | **D1 平台验收（进行中，2026-11-03）**：SDL 用途/多行属性转发与 composition UI 声明已实施；Linux Wayland 真实 IME 组合/提交/取消、多行与密码策略已测 | P3.3b2a | X11（XWayland）提交/取消已测，inline preedit 受当前 XIM 配置限制；独立 Xorg/Windows 与候选窗位置视觉验收待补；桌面软键盘/遮挡仍 unsupported，见 D1 §5.5 |

应用侧并行：维护者手工编写 `examples/notebook`（数据层已完成；服务与 UI 后续）及 `showcase/` 组件，
不改变 P3 的框架优先级；组件先在 showcase 进行视觉验收，再由 notebook 组合。

### P4 作者与组件

| 编号 | 交付 | 前置 | 退出条件 |
| --- | --- | --- | --- |
| P4.1 | **D6 首批**：Label 作者能力、绑定覆盖、有限宽度自然长高、**文本在指派盒子内的垂直对齐**与缓存契约 | 详细 API 审阅 | Label 页与 ComponentInfoTable 验收；纯绘制变动不无谓整形 |
| P4.1a | **D6 盒内对齐子批（2026-11-03，已实施）**：Text/Label top/center/bottom、两轴构建器入口、paint-only/缓存与 viewport 单次缩放 | 对齐契约见 D6 §16 | 多行/短盒/renderer/缓存回归；Label 页和 ComponentInfoTable 默认行为/宽度变化验收。P4.1 其余绑定覆盖、作者能力表与完整测量契约仍待定形 |
| P4.1 文本作者/绑定子批 | **已实施（2026-11-03）**：字体/溢出/行数 builder，当前布局结果查询，单源文本绑定、直接写入替代与作用域/退出树清理 | D6 §17 | Property/Label/BuildContext 共用绑定身份；迟到/重入回调不会复活旧源；字体请求失效回归。逐字段 reset 与完整 D6 仍保留 |
| P4.1b | **视觉表面作者能力（已提交 `0e7fca1`，代码复核完成，2026-11-03）**：可设圆角/内容的 `Surface` 原语、Card content inset、Image cover 的百分比裁剪锚点 | P4.1 的测量稳定；不新增第二套绘制路径 | 登录页可让图片真正贴边；Image 可用百分比锚点裁剪 cover；Showcase 正式视觉验收仍待完成 |
| P4.2a | **阶段 5 数据基础（已提交 `5d91e01`，维护者复核完成，2026-11-04）**：SelectionModel + VirtualListView，布局/选择/虚拟化依据 | P4.1 与布局稳定 | 复合数据组件基础有虚拟化与选择回归；契约见 [数据组件：选择模型与虚拟化依据](data_components_and_virtualization.md) |
| P4.2b | **静态 Table 布局（2026-11-05，代码复核收口）**：共享列宽、自然行高、表头/单元格语义与 Builder 槽位 | P4.2a 数据基础 | 有限宽度 wrap、列轨道一致、树语义与滚动组合回归；Showcase 视觉验收待维护者完成 |
| P4.2c | **Collapsible（2026-11-05，已随 `095336b` 提交）**：保留内容的折叠、统一激活、焦点恢复、Signal 与语义 expanded | P4.2b 代码收口；既有 Button/布局/绑定 | 布局收敛、内容生命周期、键盘/语义/回调重入与作用域回归；Showcase 视觉验收待维护者完成 |
| P4.3 | **D2 主题状态订阅**、**D3 nav_group**、**作者颜色入口（新增）**：显式 `NanColor` 覆盖与 primitive palette 访问，不为 red/blue 等追加语义 token | D2 通知与生命周期定形；D3 无跨专题前置 | Shell 无重复事实来源；自定义组件可设任意颜色；语义 token 仍是默认；空组/合并/顺序/typed navigation 回归 |
| P4.3 D2 首批 | **主题呈现窄投影（2026-11-05，已随 `8920f3a` 提交）**：§5.5 两字段只读投影、作用域绑定投影与 `ui.theme_state()`、showcase 开关跟随 | 已接受的 §5.5 契约；既有 ThemeManager 观察者 | 具名/家族发布修复、分阶段通知、重入拒绝、异常与通知中销毁、showcase 外部外观跟随 |
| P4.3 D2 Shell 框架准备 | **多方案设置页依赖收敛（2026-11-08，已随 `f821f40` 提交）**：scheme/family typed errors、选择/目录作用域订阅便利入口、Shell 动作链回归 | 第 4 步观察通道；§20 定形 | 固定/双槽/system、当前与非当前替换、动态目录、外部同步、失败原子性与页面解绑；不改页面正文 |
| P4.3 D2 第 4d 步 | **TOML 命名颜色字面量（2026-11-08，已随 `5061b39` 提交）**：颜色字面量字段接受命名颜色字符串，引用语义与校验不变 | §19 定形；4c foundation 双入口 | 名称与 C++ 同值、`$palette.*`/`$palettes.*` 语义保持、规范名优先、规则下标/校验错误完整路径、实际 parse/apply 失败不修改活动状态 |
| P4.3 D2 第 4c 步 | **foundation 命名颜色字面量（2026-11-08 复核完成，已随 `f383201` 提交）**：CSS Color 4 148 色快照、类型化/字符串双入口、私有表 | §18 定形；foundation 颜色基础 | 全表独立期望与实际枚举身份、RGBA 字节、大小写/有界字符串/全字节拒绝；SVG 147 项与 CSS Color 4 固定快照 148 项原文补验通过，TOML 简写另排期 |
| P4.3 D2 第 4b 步 | **作者颜色入口（2026-11-07 复核补修，已随 `3e2542a` 提交）**：primitive 色阶类型化访问、显式 `NanColor` 语义覆盖、`from_reference`/`from_semantic_colors`、与文件导入相等性 | §17 定形；第 4 步方案/内容模型 | 别名规范化、批量原子性、派生保留来源、全族/全档寻址、C++/TOML 共用校验；不新增原始色 token，Shell 设置页与视觉验收另排期 |
| P4.3 D2 第 4a 步 | **合成器、对比度诊断与后端读回（CPU 侧 `fef829a`；读回 2026-11-09 复核完成）**：foundation 唯一合成器、显式规则/档案、三态诊断、软件与 GPU 离屏及默认 swapchain 生产绘制路径 framebuffer 读回 | §6.5–§6.8 定形；第 4 步角色/目录模型 | 编码空间手算、解析/采样在背景终点停止、显式 tone 与配置记录、三态诊断、阈值边界；软件 ±2、Linux/Hyprland+RADV 离屏 GPU ±4、Windows 11/D3D12 离屏与默认 swapchain ±4 读回对照共享合成器及错误空间拒绝（默认目标非 8 位 sRGB/读回不可用时打印 SKIP）；注册门槛、Linux 默认 swapchain/其他驱动读回待后续 |
| P4.3 D2 第 4 步 | **观察通道与作者入口（2026-11-07，已随 `4746173` 提交推送）**：选择/目录投影、明确变化事件、派生/结构校验、staged-only 发布 | 第 3 步目录/选择模型；§14 定形 | 三通道触发边界、事件端点与去重、深层派生隔离、结构校验错误分类；4a 与广播收窄另行排期 |
| P4.3 D2 第 3 步 | **方案目录与选择（2026-11-06 复核补修，已随 `995e6ae` 提交）**：`ThemeSchemeId`/`ThemeSelection`/catalog、legacy 写入口移除与调用点迁移、styles.toml 编译为规范 ID | 已接受的 §12 API 形状；D2 首批窄投影 | 失败原子提交、槽位类型校验、in_use 移除保护、匿名来源、内容替换发布、家族目录与查询 |
| P4.3 D3 子批 | **导航分组（2026-11-05，复核完成并随 `095336b` 提交）**：`nav_group` / `NavSection::group`、空组无标题、精确值全局合并 | 已接受的 D3 契约；既有 Routes/Sidebar | 默认/显式空串、交错合并、精确组名、导航过滤与真实 Sidebar 分隔线/typed navigation 回归；D2 与颜色入口仍独立待实施 |
| P4.4 | **视觉质量扩展**：页面 golden 覆盖面（更多页面/组件状态）、混合叠层验证 | P1.1 | 新页面有 approved 基线或明确不纳入的理由 |
| P4.5 | **高 DPI 与多显示器（已知缺陷，延后实施，2026-11-09 维护者报告）**：Windows/macOS/Linux 内容缩放、混合 DPI 多显示器、组件缩放模型、文字栅格与命中/IME 坐标 | 先定形单位/坐标模型、`ViewportScalePolicy` 组合与跨屏事件；不阻塞当前 P3/P4 批次 | 1.0/1.25/1.5/1.75/2.0 映射单测；Windows 175% 与 100/175 双屏、Linux 分数缩放真实窗口；按钮/滚动/光标命中无偏移，golden 基线仍按 1× 固定；见 [高 DPI 与多显示器](high_dpi_and_multi_display.md) |

## 4. 建议的近期切入顺序

1. **P3.1a 事件快照**：先修每事件坐标/修饰键被帧级状态覆盖；本批不扩展键身份、跨类型顺序、焦点策略或 IME。
2. **D1 与 P4（已确认顺序）**：P3.3b2a caret 输入区域 → P3.3b2b 真实 IME/软键盘/遮挡验收 → P4.1 Label 垂直对齐（并入 D6 首批）→ P4.1b Surface/Card inset 与 Image cover 裁剪锚点 → P4.3 作者颜色入口。P3.3b1 已把预编辑与密码用途合并交付，P3.3b2a 已接通 caret 窗口坐标与 SDL input_area；P3.3b2b 已接通 SDL 属性并完成部分 Linux 实机检查，下一步补齐 X11 inline preedit/Windows 与候选窗位置验收。P4.1a 与 P4.1 文本作者/绑定子批已交付，P4.1b 可接续推进，P4.1 的样式 reset/完整作者体验仍单独保留。
3. **保留的 P1 边界**：退役后的 Windows 静态构建/窗口生命周期复验、P1.6 完整 Image 页面端到端采样仍分别记录；不把它们混同于已交付的 Windows golden 与参考性能基线。
4. **已交付记录**：P1.2 Windows CPU/SDF 共 6 条 approved、P1.3 Windows 参考性能基线、P1.5b 阴影/圆角裁剪及 P2 应用闭环均已归档；P1.4 归 NandinaCLI。

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
