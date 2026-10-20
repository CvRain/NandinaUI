# 第三方依赖策略：来源、开关与版本

> **状态：批次 2 已完成；2026-10-18 核对修订。**
> 11 个代码依赖使用固定 commit 的 wrap，字体保留 submodule，SQLite 保留入库源码。
> SDL3 三态来源、解析报告与 SDK 消费门禁已落地；其它功能/来源开关和可选清单生成器按正文另列计划。
> 执行顺序见[项目进度与下一步](project_status.md)：SDL3 先交付，其余依赖逐项迁移，
> 不要求完成全部迁移才开始 SDF。D5 的资源模块解耦与运行时身份仍按独立批次推进。

## 1. 决定摘要

| 项 | 决定 |
| --- | --- |
| 获取机制 | 依赖源码迁移到 Meson `wrap-git`；上游需要 CMake 时用 `method = cmake` |
| 默认来源 | **vendored**：使用项目固定的源码版本；`system` 与 `auto` 是显式选择 |
| 来源选项 | 每个适用依赖的 `*_source = vendored / system / auto`；`auto` 表示系统优先、缺失再回退 |
| 功能选项 | 与来源正交；例如 `sdl_backend` 决定是否构建 SDL 后端，关闭时不查找或下载 SDL |
| 版本 | `min` 为硬下限；`pin` 记录受测版本及完整源码 commit；报告实际解析结果 |
| WrapDB | 不作为主路径；确需社区构建适配时单独评估 |
| submodule | 完成迁移后仅字体数据包 `caskaydia-cove` 保留 |
| SQLite | 保留 `third_party/sqlite3` 官方 amalgamation；D5 的功能解耦及 system 来源验收另行推进 |
| vcpkg / Conan | 当前不引入；CMake 成为一等消费者时再评估 |
| 权威声明 | 每个依赖只有一份受控的来源/版本声明；获取副本可按来源选项选择 |
| 清单生成器 | `deps/manifest.toml` 是后续候选，不是 SDL3 首批或 SDF 的前置 |

本次明确撤销旧文中“默认 wrap 优先，但默认 auto 又先系统”的矛盾表述。

## 2. 当前事实与机制核对

### 2.1 仓库现状（2026-10-18 迁移后）

- **stb（vendored，非 wrap）**：`third_party/stb/stb_image.h` 与 `stb_image_resize2.h`
  （Public Domain）用于后端无关的 CPU 图片解码，见 `third_party/stb/README.md`。
- **raylib 计划退役**：图片解码已迁走（2026-10-20），剩余用途是后备渲染后端与窗口测试宿主；
  这两项迁走后 raylib 将从依赖闭包和 CI 中移除。移除前它仍是硬依赖（`nandina_dependencies`）。
- **11 个依赖全部是 `wrap-git`**（SDL3、raylib、spdlog、box2d、utf8proc、tomlplusplus、freetype、
  harfbuzz、fribidi、Catch2、nlohmann_json），每个 pin 完整 40 位 commit；只剩**字体数据包**
  `caskaydia-cove` 是 submodule，SQLite 仍是 amalgamation。
- 构建方式沿用迁移前的接法：CMake —— raylib、spdlog、box2d、utf8proc、tomlplusplus、SDL3；
  原生 Meson —— freetype、harfbuzz、fribidi、Catch2、nlohmann_json。
- 迁移只换来源，不换构建入口：`cmake.subproject(...)` / `subproject(...)` 调用与 target 名保持不变。
- OpenSSL 仍来自系统。
- `sdl_backend=auto` 未找到 SDL3 时仍可构建 raylib 路径；这不证明 SDL 交付成功。
- `tools/sdk_bundle.py` 通过 `git ls-files --recurse-submodules` 收集源码，
  manifest 也记录 submodule；依赖来源改变时必须同时迁移这条交付链。
- wrap 下载到 `subprojects/<dep>/` 的是**未跟踪工作树**（不是 submodule）：`.gitignore` 目前只覆盖
  `packagecache` 与少数几个目录，`git status` 会把它算成未跟踪文件，SDK 的 `ls-files` 也收不到它。
  两项都要在首批迁移时一起改：忽略规则 + 显式的依赖源码/许可证收集路径。

### 2.2 CMake wrap 支持 dependency 回退

核对依据为项目 CI 固定的
[Meson 1.7.2 官方 wrap 手册](https://github.com/mesonbuild/meson/blob/1.7.2/docs/markdown/Wrap-dependency-system-manual.md#cmake-wraps)。

旧文“CMake 子项目不能用于 dependency fallback”不准确：`method = cmake` 可参与 wrap
依赖解析，但 CMake wrap 的 `[provide]` 不能照搬 `dependency_names = foo`。
应写 `公开依赖名 = 转换后的target名_dep`；非字母数字字符转换为下划线。

以下仅演示官方映射规则，不是 SDL3 的已验证 target：

```ini
[wrap-git]
directory = example
url = https://example.invalid/example.git
revision = <full-commit>
method = cmake

[provide]
foo-bar-1.0 = foo_bar_dep
```

该机制在上述版本文档中标为 experimental。SDL3 首批必须实测其真实 target、静态/动态
选项传递、版本可见性及 `dependency()` 消费路径；需要额外控制时可以显式使用
`cmake.subproject().dependency()`，但不能以“机制不支持”为由预先造通用解析器。

已有源码目录可能优先于下载，**不代表它一定等于 pin**。迁移验证应分别使用已有目录与
全新检出；目录存在时仍核对 commit 和脏状态，不能从 wrap 的声明反推实际版本。

### 2.3 WrapDB 不是 wrap 机制本身

2026-10-16 的既有调查记录：WrapDB 中未找到 raylib，box2d 条目为 2.4.1，
freetype 使用 freetype2 名称，SDL3 条目是 3.4.14 的社区 Meson 端口，而本机受测 SDL3
为 3.4.18。这些是当时的选型证据，不是对仓库索引最新状态的持续保证，本轮未重新查询索引。

选用上游源码和上游构建系统的理由是减少额外适配维护面；不把“版本更新”当作唯一判断标准。

### 2.4 SDL3 最小实验实测结论（2026-10-18）

环境：本机 Meson **1.12.1**（策略上文引用的 1.7.2 是 CI 固定版本；所用 wrap/`cmake.subproject`
能力远早于 1.7，但**正式接入前仍需在 CI 的 1.7.2 上复跑一次**，不能默认一致）。实验在 `/tmp` 的
独立工程里做，未改仓库。

| 项 | 结论 |
| --- | --- |
| `[wrap-git]` + `method = cmake` + `cmake.subproject('SDL3')` | **可行**：SDL3 作为子项目配置并构建成功，消费者链接、运行，`SDL_GetVersion` 输出 3.4.18 |
| CMake 选项传递 | `options:` 必须是 `cmake.subproject_options()` 对象（`add_cmake_defines({...})`），**不接受裸 dict**（裸 dict 会报类型错误） |
| 实测可用选项 | `SDL_TESTS=OFF, SDL_EXAMPLES=OFF, SDL_INSTALL=OFF, SDL_SHARED=OFF, SDL_STATIC=ON, SDL_TEST_LIBRARY=OFF` |
| 真实 target 名 | `SDL3-static`（静态）/ `SDL3-shared`（动态）；`target_list()` 还列出 `SDL3_Headers`、`SDL3-collector` 等。**不要猜名字**：`cmake.subproject().dependency()` 不接受 `required:`，猜错只会得到硬错误 |
| wrap 存在会**遮蔽**系统同名依赖 | 只有 `subprojects/SDL3.wrap`、没有 `[provide]` 时，`dependency('sdl3', required: false)` 返回 **not found**（不带 wrap 的对照工程是 `pkgconfig 3.4.18`） |
| `[provide] sdl3 = SDL3_static_dep` | 系统存在时 `dependency('sdl3')` 得到 `pkgconfig 3.4.18`（遮蔽解除）；但**系统隐藏时仍 not found**——子项目被配置（fallback 已提升）却解析不到依赖变量。**结论：CMake-only 上游不要依赖 `[provide]` 变量名，显式分支** |
| 显式解析三态（推荐形态） | `system`：`dependency('sdl3', version:'>=3.4.0', method:'pkg-config', required:false)`（`method:` 过滤避开 wrap 遮蔽）；`vendored`：`cmake.subproject('SDL3', options:…).dependency('SDL3-static')`；`auto`：先 system，未找到再 vendored。三者均已实测：分别得到 `system 3.4.18` / 子项目构建 / 回落 vendored |
| **静默失效陷阱** | 把没解析到的结果留成 `disabler()` 时，`system` 状态在无系统库的机器上**配置仍然 exit 0**，执行文件被静默禁用。正式接入必须 `error()` 明确失败 |
| 离线 | `-Dsdl3_source=vendored --wrap-mode=nodownload`（源码已在 `subprojects/`）构建并运行成功 |
| 全新检出 | 无 `subprojects/SDL3`、系统隐藏、`auto` → 按 wrap 下载 `release-3.4.18`、构建、运行成功 |
| CMake 适配器告警 | SDL3 的 CMake 里有 Meson 不认识的 generator expression（如 `$<COMPILE_LANGUAGE:…>`、`$<TARGET_NAME:SDL3::Headers>`、`$<CONFIG:…>`）与 `target_precompile_headers`；均为告警，构建与链接正常 |

**据此确定的接入形态**：不写 `[provide]` 变量名，改为**显式三态块**（`if/else` 即可，不需要用户函数）；
解析结果写入解析报告；解析失败必须 `error()`。

### 2.5 SDL3 正式接入的落地记录（2026-10-18）

| 项 | 落地形态 |
| --- | --- |
| wrap | `subprojects/SDL3.wrap`：`[wrap-git]` 指向上游、`revision = release-3.4.18`、`method = cmake`、**不写 `[provide]`** |
| 选项 | `-Dsdl3_source=auto\|system\|vendored`（默认 **vendored**）；`sdl_backend` 仍是"要不要这个后端"的独立开关 |
| 解析位置 | 根 `meson.build` 顶部的显式三态块（只有一个依赖；超过 2–3 个再抽 `meson/deps.build` + `subdir()`，届时也要先定义共享变量，Meson 没有用户函数） |
| 报告 | `configure` 打印 `[deps] sdl3 -> <origin> <version>`；`build/deps.json` 由 `meson/deps.json.in` 模板生成（`configure_file(content:)` 在 Meson 1.12 不存在，`output_format:'json'` 只接受扁平 dict） |
| 失败策略 | `sdl_backend=enabled` 且未解析 → `error()`；`auto` 未解析 → 警告并回落 raylib；`disabled` → 不查找、不下载 |
| 忽略规则 | `.gitignore` 增加 `/subprojects/SDL3/`（wrap 检出是未跟踪工作树） |
| pin 形式 | wrap 的 `revision` 是**完整 40 位 commit**（`829a65d7…`，对应 `release-3.4.18`），tag 只写在注释里：tag 可移动，不满足不可变来源 |
| 检出校验 | `tools/sdk_bundle.py` 校验：revision 必须是 40 位 commit、`HEAD == pin`（否则失败）、逐依赖 `git status --porcelain --untracked-files=all` 为空（`--allow-dirty` 时记录 `dirty: true`）。Meson 自己写入的 `.meson-subproject-wrap-hash.txt` 不算本地修改 |
| 报告字段 | `deps.json` 区分来源：vendored 记录 `pin/pin_version/revision/path/resolved_by`；system 的 `pin/revision/pin_version` 为 `null`（不能让人以为系统库是项目固定源码） |

**接入时发现并解决的链接冲突（重要）**：静态 SDL3 自带 Wayland 协议生成代码
（`wayland-protocols` → `fractional-scale-v1-protocol.c`），而 raylib 内置的 GLFW 也自带同一份，
`wp_fractional_scale_manager_v1_interface` 在两个静态库里各定义一次 → 所有同时链接两个后端的可执行
文件都链接失败（完整套件里的 `router_tests` / `window_lifecycle_tests` 最先暴露）。

取舍：

| 方案 | 结果 |
| --- | --- |
| **vendored SDL3 编成共享库**（`SDL_SHARED=ON` / `SDL_STATIC=OFF`，target `SDL3-shared`） | **采用**。符号各自留在 DSO 里，raylib 保持原生 Wayland；Meson 会给构建树里的可执行文件加相对 rpath（`$ORIGIN/../subprojects/SDL3`），测试与探针直接可运行 |
| 让 raylib 关掉 Wayland（X11/XWayland） | 备选，但会让对照后端的平台能力与产品后端不一致 |
| `--allow-multiple-definition` | 否决：会掩盖真正的重复定义 |
| 把两个后端拆成独立库、按需链接 | 架构上更干净，属后续批次（当前 `nandina` 单库同时编入两个设备） |

**SDK 交付**：`tools/sdk_bundle.py` 现在解析 `subprojects/*.wrap`、要求 wrap 检出存在（缺失直接失败）、
把依赖源码（除 `.git`/`.github`）收进源码包，并在 `nandina-sdk.json` 里记录
`name/directory/url/pinned/revision/method`。SDL3 源码树 53 MB / 2195 文件；脏工作区下单次打包约 98 s
（三种格式）。fixture 消费者显式 `sdl_backend=disabled` 以控制集成测试时长，并新增断言：
manifest 的 `dependencies` 含 SDL3 且 revision 为 40 位、tar 内含
`subprojects/SDL3/CMakeLists.txt`。fixture 超时 600 → 900 s，实测 250 s。

**SDK 交付闭环（新增 fixture）**：`tests/sdk_vendored_consumer_test.py`（`integration` 套件，
`is_parallel: false`，实测 193 s）走完整链路：打包 SDK → 解压到全新目录 → 断网
（`--wrap-mode=nodownload`）→ 消费者以 `sdl_backend=enabled` + `sdl3_source=vendored` 配置、编译、
链接并运行 → 断言默认后端为 `sdl3`、子项目 `deps.json` 为 vendored/shared/cmake.subproject 且
revision 与 SDK manifest 一致、`ldd` 解析到的 `libSDL3` 位于**消费者构建树**。
资源 fixture 继续用 `sdl_backend=disabled` 保持时长可控；两者职责分开。

**CI（`.github/workflows/ci.yml`）**：`build` 与 `sanitize` 增加 wrap 缓存
（key = `subprojects/SDL3.wrap` 哈希）+ `meson subprojects download` 预热，随后用
`--wrap-mode=nodownload` 配置；新增三条显式路径任务：`sdl-system`（把固定源码装进 prefix 后走
`system`，断言 `pin=null`）、`sdl-disabled`（不预热，断言 `subprojects/SDL3` 不存在且报告 `disabled`）、
`sanitize-vendored-lld`（实验性、非阻断：vendored + ASan + lld，用于收集 lld 是否能绕过 DWARF bug 的证据）。
CI 固定的 Meson 是 **1.7.2**；本地已用 1.7.2 在 vendored + `--wrap-mode=nodownload` 下完成配置、
构建并跑通 `sdl_visual_tests`（155 断言）✓；`sdl-system` 任务的关键步骤也在本地验证过：
把固定源码用 CMake 装进 prefix（99 个文件）后以 `-Dsdl3_source=system` 配置，报告为
`source=system / resolved_by=pkg-config / version=3.4.18 / pin=null` ✓。

**验证矩阵（本次实跑）**：

| 场景 | 结果 |
| --- | --- |
| `sdl3_source=system` | `system 3.4.18`，`deps.json` 记录 `pkg-config` / `linkage: unknown` |
| `system` + 版本被伪装成 3.0.0 + `sdl_backend=enabled` | **配置失败**，报"未解析到 SDL3 >= 3.4.0"（不是静默回退） |
| `disabled` + `--wrap-mode=nodownload` 且源码移走 | 配置成功，日志中 **0 次** SDL3 提及，`enabled=false` |
| `auto` + 系统版本不足 | 回落 `vendored` |
| `vendored` + `--wrap-mode=nodownload` | 构建并运行 `sdl_visual_tests`（155 断言）通过 |
| 全新检出（无 `subprojects/SDL3`）+ 默认 vendored | 首次 configure 按 wrap 下载并构建 |
| 主套件 | 89/89（含 SDK fixture）；ASan/no-RTTI 见各构建目录日志 |

**ASan 配置的已知例外**：vendored 共享 SDL3 在 ASan 构建里链接失败
（`ld.bfd: DWARF error: mangled line number section (bad file number)`，本机没有 lld 可换）。
ASan 的职责是检查**本项目**代码，因此该构建目录显式 `-Dsdl3_source=system`（系统 3.4.18），
`buildDir-asan` 81/81 通过；默认构建与 no-RTTI 仍走 vendored 共享 SDL3。CI 若具备 lld，
应改为 vendored + lld 以同时覆盖第三方构建。

### 2.6 共享库的运行时分发（2026-10-18 实测）

vendored SDL3 是共享库，而 **CMake 子项目没有 Meson install 规则**：
`meson introspect buildDir --installed` 共 120 条，其中 SDL3 条目 **0 条**；本仓库当前也
不安装任何应用二进制（`bin/` 条目 0）。因此"只装可执行文件"的形态在运行期必然失败。

| 场景 | 结果 |
| --- | --- |
| 只拷贝 `nandina_showcase` 到新目录运行 | **失败**：`error while loading shared libraries: libSDL3_shared.so.0: cannot open shared object file` |
| `app/bin` + `app/lib/libSDL3_shared.so*` + RUNPATH `$ORIGIN/../lib` | **成功**：`ldd` 解析到打包目录内的 DSO，应用以 `renderer=gpu` 启动 |
| `meson install --destdir` | 成功，但安装内容不含 SDL3（也没有应用二进制） |

**策略**：

1. 应用以"便携目录"分发时，必须同时带上 `libSDL3_shared.so.0`（soname）与真实文件，并给可执行
   文件设置 `$ORIGIN/../lib`（或等价）RUNPATH；`LD_LIBRARY_PATH` 不是解决方案。
2. 以系统包形式分发时，把 SDL3 声明为**运行期依赖**（`-Dsdl3_source=system` 与发行版包一致），
   不要把项目固定的源码版本与系统包混着用。
3. 框架自身继续只分发**源码 SDK**；一旦开始分发应用二进制或 OS 包，必须同时把 DSO 纳入打包，
   并为此新增验收（当前 `portable-bundle` 只守卫 rpath 与打包目录解析）。
4. **守卫**：`tests/portable_bundle_test.py`（`portable-bundle`，integration，无显示器）同时断言
   反例（孤立二进制解析不到 soname）与正例（便携目录解析到 `lib/` 内）。

### 2.7 其余依赖迁移（2026-10-18）

- 10 个依赖生成 `subprojects/<name>.wrap`（`[wrap-git]`、完整 commit、`method = cmake|meson` 与
  迁移前的构建入口一致），`.gitmodules` 只保留字体包，`.gitignore` 忽略各 wrap 检出。
- **行为不变**：pin 就是迁移前 submodule 的 commit；`cmake.subproject`/`subproject` 调用与 target 名未改。
  迁移过程中顺手关掉了两个一直存在的浪费：raylib 的 `BUILD_EXAMPLES`（此前一直在编示例）与
  harfbuzz 的 `benchmark`（不构建基准，也才能不随 SDK 携带 `perf/`）。
- **SDK 体积**：依赖源码未裁剪时合计 333 MB（harfbuzz `test/` 90 MB、raylib `examples/` 70 MB 占大头）。
  现在按依赖排除"构建系统在关闭选项下不会进入"的目录（harfbuzz `test/perf/docs`、raylib
  `examples/projects`、SDL3 `test/examples/IDE 工程`、fribidi `test`、nlohmann `docs/tests`、
  Catch2 `tests`），预估约 100 MB；**裁剪由 `sdk-vendored-consumer` fixture 兜底**：它从打包后的
  SDK 离线配置/编译/运行，缺文件就会失败（实测 133 s 通过，比裁剪前的 190 s 更快）。
- **迁移后仍未做**：OS 包/安装器层面的 DSO 打包（当前没有应用安装路径可验）、
"internal 类型进公开签名"的评审项（脚本只查 include 方向）、以及"把两个后端拆成独立库"的重构
（拆库后静态 SDL3 与 raylib 的符号冲突会自然消失，那时可重新评估链接方式）。
  其余依赖**已全部迁移**；`header-surface` 门禁**已落地**（[SDK 表面](sdk_surface.md) §4）。

## 3. 来源、版本与构建适配

每个迁移项至少记录：源码 URL、完整 commit、可读 tag/版本、构建方法、目录名、
系统探测名、导出 target/依赖名、构建选项及许可证信息。

- vendored 解析必须核对实际 commit 等于 pin；本地修改不得默认为受测原版。
- tag 用于可读性，完整 commit 用于精确定位；不写浮动分支。
- 低于 min 直接失败；system 位于 min 与 pin 之间时警告，更新版本也只代表满足范围，
  不等于通过项目验证。声明范围不能掩盖 ABI 或构建选项差异。
- 不从 `git describe` 的最近标签猜真实版本；报告已知版本与 commit，未知字段明确标注。
- 运行库、构建机工具分别声明。shadercross 等执行于构建机，不能链接为应用的必需运行依赖。

不要求本轮选定所有第三方库的新版本。迁移优先保持现有受测源码，升级另批验收。

**`[wrap-file]` 与 `[wrap-git]` 是两种 pin，不要只看"能下载"：**

| 形态 | 适合 | 代价 |
| --- | --- | --- |
| `[wrap-file]`（上游归档 + `source_hash`） | pin 一个**正式发布版本**（如 SDL3 `release-3.4.18`） | 校验强度最高（内容哈希）、检出最快；只能取发布产物 |
| `[wrap-git]`（仓库 + 完整 commit） | pin 一个**非发布提交**、或需要子模块/超集 | 校验强度依赖 commit；首次检出慢，且离线预热要保留 git 工作树 |

首批 SDL3 如果钉的是发布 tag，应优先实测 `[wrap-file]`；只有在需要特定提交时才退回 `[wrap-git]`。

## 4. 功能开关与来源开关

| 来源 | 行为 | 网络 |
| --- | --- | --- |
| `vendored`（默认） | 只使用项目固定源码；不存在才按 wrap 获取 | 允许下载；nodownload 时缺源码即失败 |
| `system` | 只使用系统副本，缺失/版本不合要求即失败 | 不下载，不隐式 fallback |
| `auto` | 优先系统，缺失或不合版本要求时回退固定源码 | 仅回退时可能下载 |

`*_source` 与 `sdl_backend`、`physics2d`、测试/工具开关分别表达“从哪来”和“是否需要”。
命名要在首批定死并写进 meson_options：来源选项统一用 `<依赖>_source`（例如 `sdl3_source`），
功能开关沿用现有名字（`sdl_backend`、`physics2d`）；`sdl` 与 `sdl3` 前缀差异是有意的
（功能开关是历史名字），不要为了整齐而重命名已有选项——那会打断现存的 CI 命令与文档。
禁用功能时，不应先获取其专属依赖再跳过编译。D5 的工具/SQLite 开关尚未实现，不能把目标
选项写成当前可用命令。

使用 Meson 原生 `dependency`、wrap 和 subproject 能力实现；只封装实测出现的重复逻辑。
Meson 脚本不支持任意用户函数定义，旧 `resolve_dep(...)` 伪代码不是可以直接新增的 Meson 函数。
实现文件使用正常 `meson.build` 子目录入口；自有来源选择和 `--wrap-mode` 的优先关系必须测试。

至少保证：system 不偷偷走子项目，vendored 不偷偷采用系统同名库，nodownload 绝不下载。
冲突配置须明确报错或说明解析结果，不得静默改变用户选择。
SDL 交付关卡显式启用后端，不能让 auto 回落 raylib 后仍宣称 SDL 检查通过。

## 5. 解析报告与单一声明

目标输出为配置 summary 与构建目录内的 `deps.json`。**当前实现的字段**（与
`meson/deps.json.in` 一致，避免文档与产物各说一套）：

| 字段 | 含义 |
| --- | --- |
| `name` / `enabled` | 依赖名 / 是否**实际采用**（auto 未解析时为 false） |
| `source` / `resolved_by` | system / vendored / disabled / unavailable；pkg-config / cmake.subproject / none |
| `version` / `min` | 实际版本 / 硬下限 |
| `pin` / `pin_version` | 固定的完整 commit（system 与 disabled 为 `null`）/ 可读的 pin 版本 |
| `revision` | 实际 commit；vendored 在 configure 期等于 pin（检出是否真的在 pin 上由 SDK 打包器校验） |
| `path` / `method` / `linkage` / `license` | 源码目录 / cmake 或 pkg-config / shared 或 unknown / 许可证 |

报告反映实际输入，不能只是抄一遍 wrap。CI 检查关键字段并验证其确实对应构建依赖。

首批可直接以 wrap 保存源码来源/commit，以 Meson 适配声明 min、系统名称和选项；
同一字段不重复维护。只有维护成本出现后，再引入 `deps/manifest.toml`、生成器及一致性关卡。
届时 wrap 和适配表从清单派生，不把“未来清单”误写成当前文件或首批必需设施。

## 6. 离线、SDK 与 CI 是同一批交付

wrap-git 检出目录与 wrap-file 的归档缓存不同；**只缓存 packagecache 不足以保证
wrap-git 离线**。预热时保留所需源码检出及其必要嵌套依赖，并检查固定 commit。

| 场景 | 必须证明的行为 |
| --- | --- |
| 全新检出 | 没有旧 submodule 目录和系统 SDL 兜底，仍能取得并构建声明版本 |
| 预热后离线 | 禁用网络并使用 nodownload，从保存的源码输入成功构建 |
| 已有目录 | 错 commit/本地修改被报告或拒绝，不冒充 pin |
| SDK 导出 | 收集解析后的依赖源码、许可证和版本信息，保持源码包与 manifest 一致；文件收集（构建闭包）与公共表面（支持范围）按 [SDK 表面](sdk_surface.md) 分开 |
| SDK 下游消费 | 解压到新目录、断网配置/编译并运行消费者；不依赖原仓库或 playground |
| CI | 至少有显式启用 SDL 的构建/测试任务；基础回退路径另测，不混为同一结果 |

现有 SDK 的可重复打包及 subproject fixture 是迁移的一部分；不能只改 `.gitmodules`
而让源码包丢依赖。每次迁移都检查真实输入收集、许可证、manifest 和下游构建。

**打包与 fixture 必须在"无网 + nodownload + 已预热源码"下运行**：`nandina-subproject-fixture`
会把工作区打包两次并断言逐字节一致；如果打包过程里触发一次 wrap 下载或刷新了检出目录的时间戳，
这个断言就会因为与代码无关的原因失败（这类"假红"已经在开发流程里记为硬规矩）。
CI 缓存键应包含来源声明、pin、构建选项及适用工具链，缓存命中不替代版本核对。

## 7. 与 D7、D5 和未来构建系统的关系

- **D7** 只需要 SDL3 来源和着色器交付方式先稳定，不等待其余库退出 submodule。
  推荐随源码包交付受控的 shader 产物，并记录源哈希、工具版本、格式及生成命令；
  专用维护关卡重新生成/校验，普通消费者不必安装 shadercross。具体嵌入/资源布局在 SDF 批定形。
- **D5** 处理资源工具解耦、作用域、身份和部署。依赖迁移不修改资源 ID 或包 schema，
  也不能用“支持源码选项”代替纯工具配置和关闭组合验收。
- **CMake package** 仍暂缓；将来复用权威来源声明，不复制一份版本表。
- **vcpkg/Conan** 当前不引入是维护范围选择，不宣称它们在技术上无法服务 Meson。
  重新评估时仍守住同一依赖只有一份权威声明。

## 8. 实施与验收顺序

| 批次 | 交付 | 退出关口 |
| --- | --- | --- |
| 1 SDL3 最小实验（**已完成** 2026-10-18） | 固定源码 + CMake wrap + 实际 target/选项/依赖消费；记录 Meson 版本 | 已达成并记录于 §2.4：`SDL3-static`、选项对象、三态解析、离线、全新检出均实测；**结论不进 `[provide]` 变量名，改显式分支；解析失败必须 `error()`**。CI 的 Meson 1.7.2 复跑待正式接入时补 |
| 2 SDL3 正式接入 | 三态来源、功能开关、解析报告、CI 与 SDK | 三态与 disabled 组合、实际 SDL 消费、离线 SDK、三套代码验证 |
| 3 其余依赖逐项迁移（**已完成** 2026-10-18） | 每项单独提交，必要时较大依赖拆实验与接入两批 | 已达成：10 个依赖改为 wrap-git、pin 保持迁移前 commit、`.gitmodules` 只剩字体包、SDK 依赖源码收集 + 消费 fixture 通过；详见 §2.7 |
| 4 完成迁移 | 字体保留 submodule，SQLite 保留 amalgamation，相关说明与清单同步 | 全新检出、联网预热、断网源码包消费完整走通 |
| 5 可选清单生成 | 有实际收益时引入 manifest 与生成器 | 单一声明、生成物一致性关卡；不成为前四批前置 |

迁移期间保留未迁移 submodule 是正常的；单项切换后不得留下两份互相竞争的版本声明。
删除 submodule 跟踪配置不等于授权丢弃本地改动；实施前检查每个目录状态。

本轮只核实机制文档和仓库状态，尚未运行 SDL3 wrap 最小实验，也未交付新开关、deps.json、
shader 产物包或 SDK 改造。
