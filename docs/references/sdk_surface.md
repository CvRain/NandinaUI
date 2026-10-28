# 源码 SDK 的公共表面：头文件分类与门禁

> **状态：政策与三类门禁已落地（2026-10-18）；公开签名中的内部类型仍需评审。** 本文回答两件事：哪些头文件算**对外契约**、
> 用什么机制保证这件事不被静默破坏。它不改变 [构建系统与 Modules 范围](build_system_scope.md)
> 里"CMake package 暂缓"的结论，也不改变 [D7](reference_draft.md#d7-渲染质量与后端视觉一致性)
> 的图片解码归属。

## 1. 两个必须分开的概念

| 概念 | 说明 | 谁使用 |
| --- | --- | --- |
| **构建闭包**（collection） | 编译框架与工具所需的全部源文件；当前由 `tools/sdk_bundle.py` 用 `git ls-files --recurse-submodules` 收集 | 构建、SDK 打包 |
| **公共表面**（surface） | 允许下游直接包含、并按等级承诺稳定性的头文件清单 | 应用开发者、文档、未来的安装式 SDK |

**"SDK 里带着这个文件"不等于"它是公共 API"。** 源码 SDK 要从源码构建框架和工具，内部头也可能是
构建所必需的；不能用一张"公共头白名单"去过滤掉其余文件，那会把构建闭包一起删掉。两者关联，
但不是一个过滤器。

## 2. 三档分类

2026-10-22 P2.1a 构建表面变更：资源运行时通过 `resource_runtime_dep` 链接，协议核心可单独使用
`resource_core_dep`；扫描/manifest/packager 属于可选 `resource_build_dep`，不再由 `nandina_dep`
传递提供 scanner 实现。直接调用构建 API 的消费者须开启 `resource_tools` 并显式链接
`resource_build_dep`。`resource_package_format.hpp` 为 experimental 共享格式协议；旧
`SQLiteBackend::application_id/schema_version` 保留为同一常量的转发，包格式不变。
`NanApplicationConfig::resource_package` 默认值与外部 nanres 接入将在 P2.1b 单独迁移。

2026-10-23 P2.2a 后端协议变更（experimental）：`IResourceBackend::find()` / `open_stream()`
改返回带 `PublishGeneration` 与 `ResourceConsistency` 的 `ResourceSnapshot` /
`ResourceStreamSnapshot`，负结果也属于某个代次；新增 `generation_exhausted` 错误码。
`ResourceManager` 八个公开入口在 P2.2a 保持裸句柄返回，只在内部解包；作用域结果
（`ResolvedResource`/`ResolvedStream`、域令牌、歧义/过期错误）按 P2.2b 迁移。自定义后端
须返回一致的结果与代次；`non_snapshot` 结果不得仅凭作用域键进入共享内容缓存。

2026-10-23 P2.2b 作用域结果（experimental，破坏性）：`ResourceManager` 八入口改为返回
`ResolvedResource` / `ResolvedStream`，携带 `ResourceDomainToken`、`ResourceMountId`、
发布代次、一致性类别、记录 ID 与句柄；旧 `ResourceLookup`/`ResourceStreamLookup` 裸句柄别名
删除，消费者改用 `.resource` / `.stream`。新增 `resolve(const ResolvedResource&)` /
`resolve(const ResolvedStream&)`，只按原作用域重解析，过期返回 `expired_scope`；裸 ID 多重
挂载命中返回 `ambiguous_resource_id`，消息列出冲突挂载。缓存身份迁移（字体/图片/流）在
P2.2c，本批只做编译级消费者迁移。

2026-10-23 P2.2c 缓存身份（experimental，破坏性）：新增 `ResourceScopeIdentity`、
`scope_identity(...)` 与 `ResourceManager::domain()`。`text::FontFaceKey` 改为
`(ResourceScopeIdentity, face_index)`；`FontLoader::invalidate(ResourceId)` 改名为
`invalidate_wide(ResourceId)`，新增 `invalidate(const ResolvedResource&)`；直接
`ResourceHandle` 加载绕过共享 face 缓存，`non_snapshot` 来源不进入共享缓存。
`render::TextureCache` 新增 `load_resolved(const ResolvedResource&, options)`；资源异步加载在
后台解析后才形成作用域缓存键，`non_snapshot` 内容可上传但不索引、不保留。文本 key 的
`load_memory` 仍只用于调用方显式提供的非资源内存。

2026-10-24 P2.2d 缓存回收（experimental，增量）：`text::FontLoaderLimits{max_entries}` 为
FontLoader 提供有界 LRU，`cached_entries()` 返回当前关联数；访问时清扫 expired 域与已释放的
weak face，淘汰不销毁调用方持有的 face。`render::TextureCache` 新增 `sweep_expired()` 与
`invalidate(const ResolvedResource&)`；`begin_frame()`/`trim()` 执行 expired 域清扫，既有
`max_retained_entries`/`max_retained_bytes` 继续负责容量淘汰。资源异步加载改为解析/解码
两阶段，缓存命中不再解码或上传；pending 仍按域 + 逻辑 key + options 合并。

2026-10-24 P2.3a 资源身份（experimental，构建侧）：`resources.toml` 的 `[[resources]]` 新增
相对 `path`，与 `key` 一起构成显式 key→路径映射；显式映射优先于路径派生 key。资源构建核心
新增 `derive_resource_key(prefix, relative)` 与 `derive_resource_id(package_id, key)`；
后者使用版本前缀 + NUL + big-endian u32 长度 + package_id + key，SHA-256 截断 128 bit。
同 key 的旧 lock 保留原 ID，新 key 使用派生 ID；同一 `(package_id, key)` 的派生 ID 跨构建、
跨平台稳定（固定向量锁定），不同 key 即使内容相同也各得独立 ID。扫描新增
`ResourceScanOptions::mapped_paths`：显式映射拥有的文件不再派生 key、不报扫描诊断；
`resource_path_identity(...)` 统一路径身份比较（Windows 上按 ASCII 折叠大小写）。
`--rewrite-lock`、构建目录迁移与碰撞注入在 P2.3b。

2026-10-24 P2.3b lock 迁移（构建侧）：`nanres scan/validate/pack` 新增 `--lock <path>`；
构建 helper 将 lock 写到 `<output>.lock.toml`，普通构建不再写源码树。旧随机 ID 的 lock 会被
检测并失败，只有 `nanres scan --rewrite-lock` 显式迁移；两个标志仅在 manifest 模式生效，
原始扫描覆盖下返回用法错误。加载 lock 遇到同 ID 不同 key 或同 key 不同 ID 会列出冲突；lock
的包 ID 与 policy 不一致时先报包不匹配。相同策略/资产的独立重建得到逐字节相同的 lock，
无变化不重写 lock 或 `resource-location.json`。

2026-10-25 P2.3c 作者常量头（experimental，构建侧）：新增 `nanres keys --header <path>`，
从 `resources.toml` 与扫描结果生成只含逻辑 key 的 `inline constexpr std::string_view`
常量头；规范 key 与 `[aliases]` 都会生成，不含内部 ID、摘要、源路径或安装位置。命名空间为
`nandina::resource_keys::pkg_<package-id>`（`.`/`-`/`_` 分别转义为大写
`DOT`/`DASH`/`UNDERSCORE`），符号为 `RESOURCE_` 加大写 key。两个 key 折叠成同一符号时命令失败并列出双方，不静默改名；
内容未变化时不重写文件。`keys` 只接受 `--header`，`--root` 等扫描覆盖和其它命令的
`--header` 均返回用法错误。该头是可选构建输入，不影响只用运行时 key 的应用。

2026-10-25 P2.4 示例应用（构建侧，非 SDK API）：`examples/lab` 随普通 UI 构建默认参与，
一个可执行文件包含登录/注册、笔记、Focus 计时器、图片预览与确认弹窗。它要求 `build_ui=true`、
SDL3 和资源工具链；资源包与生成常量头由 Meson 在链接/编译前生成。示例不是 SDK 安装内容，
也不承诺密码掩码、持久化或真实认证。

2026-10-28 示例应用演进（同样是构建侧变化，非 SDK API）：`examples/lab` 已退役，由
`examples/notebook` 接替（个人笔记软件，数据层先行）。notebook 要求 `build_ui=true`、资源工具链
和 `resource_sqlite=enabled`（应用数据库与迁移包都走 SQLite），因此在关闭 SQLite 或关闭工具的
配置里用 `-Dexamples=false` 跳过；`notebook-data` 数据层测试随 `examples` 一起构建，Windows 上
可正常运行。P2.4 的 `examples/lab` 验收记录保留为历史。

| 等级 | 含义 | 稳定性承诺 | 谁可以包含 |
| --- | --- | --- | --- |
| `public` | 推荐使用的接口 | alpha 内也尽量不破坏；破坏时给迁移说明 | 应用代码、示例、下游宿主 |
| `experimental` | 允许外部使用，但接口仍会调整 | 明确标注可能变更；变更需要迁移说明 | 愿意跟进的宿主/适配器作者 |
| `internal` | 为构建与内部实现携带，不是契约 | 无 | 仅框架自身（含 probe、开发工具） |

规则：

1. **internal 头不得出现在 public/experimental 头或公开签名里**；需要暴露类型时要么前移类型，
   要么把它提升为 experimental 并说明。
2. **public/experimental 头必须能在 SDK 消费环境里独立包含**（只带自己需要的依赖）。
   仓库已有先例：`foundation/motion/spec.hpp` 承担独立包含测试，文本头退出时也保留了
   canonical 独立包含测试 —— 这条机制复用即可，不需要新造。
3. **新增框架头文件必须有分类**，没有分类直接失败（见 §4）。

## 3. 当前决策（2026-10-17）

| 头文件 | 等级 | 理由 |
| --- | --- | --- |
| `app/platform_window.hpp` | **experimental public** | 自定义宿主、平台适配器和窗口测试替身有真实外部用途；它已经是应用注入平台实现的接口。D8 会调整输入模型，所以允许变更但明确标注 |
| `render/recording_device.hpp` | **internal** | 目前服务后端取证：字符串记录格式、成本与行为都不是对外契约。外部使用者仍可用公开的 `IRenderDevice` + `NanWindow::set_render_device_wrapper()` 自己实现装饰器。确有下游诊断需求时再提升为 experimental |

把 `platform_window.hpp` 藏成 internal 并不符合实际用途（它本来就是给宿主实现方用的），
而把 `recording_device.hpp` 提升为 public 会让一个取证格式变成长期负担。两者的正确等级不同，
不能"统一处理"。

## 4. 三类门禁（**已实施 2026-10-18**）

| 门禁 | 检查什么 | 失败表现 |
| --- | --- | --- |
| 分类完整 | `nandina/**/*.hpp` 每个都在清单里恰好一条 | 新头未分类 / 陈旧条目 / 非法等级 → 失败（不是警告） |
| 不越界 | public/experimental 头不 `#include` internal 头（相对与 `<nandina/...>` 两种写法都解析） | 依赖方向倒置 → 失败 |
| 独立可包含 | 每个 public/experimental 头用一个只包含它的 TU 做 `-fsyntax-only` | 缺依赖/隐式前置 → 失败 |

落点：清单 `tools/header_surface.toml`（182 条，含 `level`、可选 `reason`、可选 `requires`），
脚本 `tools/check_headers.py`，注册为 `header-surface`（`integration`，实测约 16 s，8 路并行）。
用法：

```bash
tools/check_headers.py --source . --write                 # 新头补默认等级，陈旧条目清理
tools/check_headers.py --source . --build-dir buildDir    # 三条门禁全跑（CI 走这条）
tools/check_headers.py --source . --skip-compile          # 无构建目录时只跑前两条
```

实现要点：

- **编译 flag 不手写第二份**：从 `compile_commands.json` 汇总 `-I`/`-isystem`/`-D`/`-std=`，
  所以 spdlog/SDL3 等 include 路径自动正确。
- **可选功能用 `requires` 表达**：例如 `physics2d/physics_world2d.hpp` 标 `requires = "physics2d"`，
  该选项关闭时跳过独立可包含检查并打印跳过原因，而不是硬编码依赖的 include 路径。
- **故障注入已验证**：新增未分类头 → 失败；`button.hpp` 包含 `internal/focus_scope.hpp` → 失败并指出
  两端；破坏 `geometry.hpp` → 独立可包含失败（并级联到包含它的头）。
- **"internal 头不出现在公开签名里"仍靠评审**：脚本只能检查 include 方向，看不到类型出现在签名中；
  这一条写在这里，避免被误认为已自动化。

### 4.1 首次运行发现的真实违规与处理

`popover.hpp` / `hover_card.hpp` / `dropdown_menu.hpp` 在公开签名里使用
`internal::OverlayPlacement` / `internal::OverlayAlignment`，而定义它们的是
`nandina/widget/internal/anchored_positioner.hpp`（原分类 internal）。处理：**把该头提升为
experimental**，理由与后续动作写进清单——两个枚举是公开 widget API 的一部分，但文件仍在
`internal/` 目录下，后续应移动到 `nandina/widget/overlay_placement.hpp` 再删除这条例外。

**清单描述支持范围，`sdk_bundle.py` 描述构建闭包。** 将来提供安装式 SDK 时，由这份清单生成
公共 include 树，而不是再维护第二份"安装头列表"。

## 5. 破坏性变更的记录方式

图片批同时有两处**破坏性 API 变更**，都要登记：

| 变更 | 位置 | 迁移说明 |
| --- | --- | --- |
| 删除 `make_image_decoder()` | `IPlatformWindow`（experimental public） | 改为应用服务注入 `IImageDecoder`（2026-10-20 已删除，含两后端 override） |
| 删除 `make_raylib_image_decoder()` | `render/image_decoder.hpp`（experimental public） | 默认实现改为 `render::make_cpu_image_decoder()`（stb，后端无关） |
| `supports_image_loading()` → `supports_direct_image_loading()` | `IRenderDevice`（experimental public） | 语义收窄为"设备自己解码直载"，默认 false |
| 新增 `supports_rgba_texture_upload()` | `IRenderDevice` | SDL/raylib 均为 true；`Image` 用三项能力选择服务路径 |
| 新增 `NanApplication::image_decoder()` / `set_image_decoder()` | `app`（experimental public） | 默认注入 CPU 解码器，应用可替换；无热切换（窗口打开时快照） |
| 删除 `make_raylib_device()` 与 `raylib_device.hpp` | `render/backends`（experimental） | 使用 SDL3，或实现并注入自己的 `IPlatformWindow` / `IRenderDevice` |
| 删除 `WindowBackend::raylib` | `app/window_config.hpp`（experimental） | 改为 `automatic` / `sdl3`；`sdl3` 保留原数值 2。环境变量中的 raylib/未知值明确报错，不能静默选择另一路径 |
| 默认 SDL3 静态链接 | Meson / SDK | `sdl3_linkage=shared` 可恢复共享模式；关闭 SDL3 时无内置窗口后端，`showcase=false`，应用在 `open()` 前注入平台 |
| `NanApplicationConfig::resource_package` 默认值 | `app`（experimental public） | 默认从 `"resources.db"` 改为**空**：启动不再自动发现/打开包；需要自动开包时显式设置。迁移：依赖旧默认值的应用显式写 `"resources.db"`，或改用注入 |
| 新增 `NanApplication::mount_resource_backend()` | `app`（experimental public） | 在窗口/字体/图片首次消费前注入自定义后端；后端由应用保活 |
| `resource_tools=false` / `external_nanres` | Meson 构建表面 | 关闭本地工具时 `nandina_resource_toolchain` 仅在提供外部工具时存在；外部工具须通过 `nanres schema`（值 1）与 `--version` 握手，否则配置期失败 |
| 新增 `nanres schema` / `nanres --version` | `nanres` CLI（构建期工具） | 供外部工具握手与诊断；不属于应用运行时 API |
| `supports_image_loading()` 收窄 + 新增 RGBA 上传查询 | `IRenderDevice`（设备实现方接口） | 原先判断"能不能显示图片"的调用方，改为分别查询解码服务、设备直载、设备 RGBA 上传三项，见 [D7](reference_draft.md#三项能力分开表达2026-10-17-定) |

项目处于 alpha、源码集成、不承诺旧入口长期兼容，因此：

- **不保留**一个仍可 override、但框架已不再调用的虚函数 —— 那是最难发现的静默失效；
- 迁移说明只表达一件事：原来由自定义平台 override 提供的解码器，改为应用服务注入；
- 库内实现、测试替身、下游样例一次改完，要求重新编译。

退役后清单为 182 个头文件，其中 10 个 internal。生成模板归 NandinaCLI；
框架维护 [`sdk_app` 消费 fixture](../../tests/fixtures/sdk_app/README.md)，不维护第二份模板。

## 6. 不要越界声明的事

- 门禁已实现，但它**不检查签名**：internal 类型出现在公开签名里仍要靠评审（见 §4）；
- 不要把"文件在 SDK 里"当成"支持范围已定义"；
- 不要用本清单代替构建闭包校验（SDK 仍必须包含构建所需的 internal 头）；
- 提升等级是**决策**，不是 `--write` 的副作用：`--write` 只补默认值（`detail/`、`internal/` 归
  internal，其余 experimental），理由与例外必须人来写。
