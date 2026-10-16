# 第三方依赖策略：来源、开关与版本

> **状态：策略已定（2026-10-16），未实施。** 本文回答三件事：依赖**从哪来**、开发者**怎么选**、
> 版本**偏了怎么办**。它不重复 [构建系统与 Modules 范围](build_system_scope.md) 的结论（CMake
> 消费端仍暂缓），也不改变 [D5 资源工作线](resource_build_and_deployment.md) 的批次划分。
>
> 背景是 D4 后端替换：把生产后端换成 SDL3 之后，"SDL3 恰好是这台机器上装的 3.4.18"这种
> 事实变成了构建可用性的前提。本文把依赖获取从"看运气"改成"有声明、有开关、有报告"。

## 1. 决定摘要

| 项 | 决定 |
| --- | --- |
| 获取机制 | **统一用 meson wrap（`[wrap-git]`）**；CMake-only 的上游加 `method = cmake` |
| 默认来源 | **wrap 优先**（不再依赖"开发者机器上刚好装了"）；`system` 作为显式选择 |
| 开关粒度 | **依赖级三态**：`system` / `vendored` / `auto`（默认 `auto`）。不做逐个依赖的细粒度矩阵 |
| WrapDB | **不作为主路径**，仅在"上游没有可用构建系统且不想走 CMake"时作为备选 |
| submodule | 迁移完成后**只剩字体数据包**（`caskaydia-cove`）保留为 submodule |
| sqlite3 | **维持现状**：官方 amalgamation 入库 `third_party/sqlite3`，不进 wrap |
| vcpkg / Conan | **不引入**（理由见 §4） |
| 版本规则 | 每个依赖声明 `min`（硬下限）+ `pin`（受测版本）；详见 §6 |
| 未来形态 | `deps/manifest.toml` 作为人与工具的**真源**，生成 wrap 与 meson 表；见 §7 |

## 2. 为什么是 wrap，而不是 submodule 或 WrapDB

### 2.1 现状（2026-10-16 核实）

`subprojects/` 下 10 个 git submodule，没有任何 `.wrap` 文件：

| 依赖 | 上游构建系统 | 现在怎么接 |
| --- | --- | --- |
| box2d / raylib / spdlog / utf8proc | CMake | `cmake.subproject(...)`，submodule 提供源码 |
| freetype / harfbuzz / fribidi / tomlplusplus / Catch2 / nlohmann_json | 上游带 `meson.build` | `subproject(...)` |
| caskaydia-cove | —（字体数据） | submodule，仅数据 |

SDL3 目前来自系统 pkg-config（`dependency('sdl3', version: '>=3.4.0')`）。

### 2.2 机制事实（读本机 meson 源码核实，不是印象）

- `mesonbuild/wrap/wrap.py` 的 `methods_map` 支持 **`meson` / `cmake` / `cargo`**，wrap 文件里可以写
  `method = cmake`，此时 meson 要求源码根有 `CMakeLists.txt`。→ **CMake-only 的上游也能用 wrap 获取**，
  再交给 `cmake.subproject()` 构建，和现在 submodule 的做法等价。
- 同一个解析器里 `resolve_git_submodule()` 让"submodule 已检出"与"wrap 下载"成为同一个目录的
  两种来源（目录存在即优先）。→ 迁移可以渐进：先加 wrap，再删 submodule。
- `dependency(..., fallback: [...])` 只对**能提供 `declare_dependency()` 的 meson 子项目**有效；
  CMake 子项目没有这个入口（这正是现有代码写 `cmake.subproject(...).dependency('raylib')` 的原因）。
  → 三态分支必须显式写，见 §5。
- `--wrap-mode={default,nofallback,nodownload,forcefallback}` 是 meson 自带开关，可以叠加在自定义
  选项之上；`meson subprojects download` 可预热。→ 离线构建方案见 §8。

### 2.3 为什么不把 WrapDB 当主路径

WrapDB 确实覆盖了大部分依赖，但核实后有三条硬伤：

| 依赖 | WrapDB 最高版本 | 我们需要的 | 结论 |
| --- | --- | --- | --- |
| **raylib** | **不存在**（索引里 0 次命中） | 任意 pin | WrapDB 拿不到 |
| **box2d** | **2.4.1-4** | **3.1.1**（submodule 实测 `v3.1.1`） | 大版本不符 |
| freetype | 名为 **`freetype2`**（2.15.0-1） | 2.15.x | 名字与目录都不同 |
| **sdl3** | 3.4.14-1 | ≥ 3.4.0（`SDL_GPURenderState`） | 满足下限，但**旧于系统 3.4.18** |

更要紧的是 `sdl3_3.4.14-1.wrap` 的内容：

```ini
[wrap-file]
source_url = https://github.com/libsdl-org/SDL/releases/download/release-3.4.14/SDL3-3.4.14.tar.gz
patch_filename = sdl3_3.4.14-1_patch.zip
[provide]
dependency_names = sdl3
```

那个 patch 有 **357 个文件**，核心是一份社区维护的 `meson.build` 端口。也就是说"用 WrapDB 的 SDL3"
= **把上游构建系统换成第三方端口**，而且它滞后于我们系统里的版本。对一个**生产后端**来说，
这比"wrap-git 指上游、用 SDL 自己的 CMake"更脆：端口的选项覆盖、Wayland/X11/GPU 开关与版本跟进
都不在我们控制内。

**结论**：要的是 meson 的 **wrap 机制**，不是 WrapDB 的**包集合**。这两件事经常被混为一谈。

## 3. 统一形态：`wrap-git` + `method`

```ini
# subprojects/SDL3.wrap
[wrap-git]
directory = SDL3                       # 注意：SDL 的仓库名是 SDL，目录名要显式给
url = https://github.com/libsdl-org/SDL.git
revision = release-3.4.18              # pin：我们受测的确切 tag/commit
method = cmake                         # CMake-only 上游

[provide]
dependency_names = sdl3                # 系统探测用（pkg-config 名）
```

要点：

- **`directory` 必须显式写**当仓库名与依赖名不同（SDL → `SDL3`）。
- **`revision` 只写 tag 或完整 commit**，不写分支：分支会让同一份 wrap 在不同时间解析出不同代码。
- **CMake-only 上游**（SDL3 / raylib / box2d / spdlog / utf8proc）加 `method = cmake`；
  其余（freetype2 / harfbuzz / fribidi / tomlplusplus / Catch2 / nlohmann_json）用默认 meson 方法。
- **目标名要单独确认**：`cmake.subproject(X).dependency('Y')` 里的 `Y` 是 CMake 导出的 target 名。
  现有代码用的是 `raylib` / `spdlog` / `box2d`；SDL3 需要一次最小实验确认（可能是 `SDL3` 或
  `SDL3-static`，取决于静态/动态选项）。这条不确认就会得到"子项目构建成功但拿不到依赖"。

## 4. 为什么不引入 vcpkg / Conan

- **vcpkg 没有 meson 消费端集成**。官方路径是 CMake（toolchain file / manifest）与 MSBuild；
  meson 侧只能手工把 `PKG_CONFIG_PATH` 指向 `installed/<triplet>`，且只有装了 `.pc` 的 port 可用，
  没有 manifest/lockstep 跨两套构建系统。"项目只挂一个 vcpkg 子项目"在 meson 上不成立。
- **它会让同一个依赖出现两份来源**。`meson.build` 里 sqlite3 那段已经把判据写清楚了：
  > wrap 只服务 Meson、vcpkg 只服务 CMake、submodule 要求消费者 clone 镜像仓，
  > 三条路都会让两套构建系统拿到来源不同的同一个依赖。

  sqlite3 因此选了 amalgamation 入库。这条判据现在是**成文规则**（§9）。
- **Conan 2** 确实是"一个管理器喂两套构建系统"的成熟选项（`CMakeToolchain`/`CMakeDeps` +
  `MesonToolchain`/`PkgConfigDeps`），代价是再引入一个工具、一套 recipe 维护面和一份二进制缓存
  策略。当前只有一套一等构建系统（Meson），收益不足以抵成本。**CMake 成为一等入口时重新评估，
  且届时 vcpkg 与 Conan 只能选一个，不能与 wrap 混用同一依赖。**

## 5. 三态开关

依赖级一个选项，取值三态：

| 取值 | 行为 | 是否下载 |
| --- | --- | --- |
| `system` | 只 `dependency()` 系统副本；缺失或低于 `min` 直接失败 | 否 |
| `vendored` | 只用 `subprojects/`（目录存在就用；缺失才按 wrap 下载） | 是（除非 `--wrap-mode=nodownload`） |
| `auto`（默认） | 先系统；缺失或低于 `min` 才回落 vendored，并打印解析结果 | 仅回落时 |

命名约定（避免和"要不要这个后端"混淆）：

- **`*_source`** = 依赖**从哪来**（`auto|system|vendored`），例如 `-Dsdl3_source=system`；
- 现有的 **`sdl_backend`** 继续表示**要不要这个后端**（`auto|enabled|disabled`）。两者正交，不要合并。

因为 CMake 子项目用不了 `dependency(fallback:)`，三态分支要显式写。建议抽到
`meson/deps.build`（与现有 `meson/nandina_resources.build` 同风格）：

```meson
# 形状示意，未实现
sdl3_dep = resolve_dep(
  'sdl3',                                    # 系统探测名
  source_option: get_option('sdl3_source'),
  subproject: 'SDL3',                        # subprojects/SDL3
  dependency: 'SDL3',                        # CMake target 名
  cmake_options: {'SDL_TESTS': 'OFF', 'SDL_EXAMPLES': 'OFF'},
)
```

## 6. 版本：两条线 + 一份报告

回答"虽然不能完全保证版本问题，但是可以提醒"：

| 线 | 语义 | 不满足时 |
| --- | --- | --- |
| `min` | **硬下限**，由我们实际用到的 API 决定（例如 SDL3 `>=3.4.0`，因为 `SDL_GPURenderState` 从 3.4 起） | 直接失败 |
| `pin` | **我们受测过的确切版本/提交**（写进 wrap 的 `revision`） | 系统副本 `< pin` 时**打警告**；vendored 副本必须**恰好等于 pin** |

构建期输出一行解析报告，并写一份机器可读结果：

```
[deps] sdl3      3.4.18   system   /usr/lib/x86_64-linux-gnu/pkgconfig/sdl3.pc
[deps] box2d     3.1.1    vendored subprojects/box2d @ v3.1.1
[deps] raylib    5.5      vendored subprojects/raylib @ <commit>
```

- 人看的：`meson setup` 结束时的 summary + stderr；
- 机器看的：`build/deps.json`（依赖 → 版本 → 来源 → 路径），CI 与 bug 报告共用同一份。

## 7. 未来形态：`deps/manifest.toml`

细粒度开关现在不需要；真需要时的正确形态是一份清单，而不是给每个依赖加参数。但必须先知道
meson 的两条硬约束：

1. **meson 不能读 TOML**（无 TOML 解析器）；
2. **meson 不能消费"生成出来的 meson 代码"**（文件在构建开始前就解析完了）。

所以可行形态是"清单为真源 + 生成物 + 关卡"，而不是"meson 直接读清单"：

```
deps/manifest.toml            ← 人与工具的唯一真源
        │  tools/gen_deps.py --write
        ▼
subprojects/*.wrap            ← wrap 文件（内容由清单决定）
meson/deps.build              ← meson 语法表（系统名 / 子项目名 / target / 选项默认值）
        │  tools/gen_deps.py --check   （CI 关卡：生成物与清单不一致就失败）
        ▼
build/deps.json               ← 构建期解析报告
```

字段草稿：

```toml
[deps.sdl3]
url      = "https://github.com/libsdl-org/SDL.git"
revision = "release-3.4.18"    # pin
min      = "3.4.0"             # 硬下限
method   = "cmake"
directory = "SDL3"             # 仓库名 ≠ 目录名时必须显式
provide  = "sdl3"              # 系统探测名
target   = "SDL3"              # cmake.subproject().dependency(...)
option   = "sdl3_source"       # 三态开关名
```

这与仓库现有文化一致：声明式表 + `tools/check_*.py` 关卡（`check_format.py`、
`check_test_suites.py`）。**不引入清单之前，版本表先落在 `meson/deps.build` 里。**

## 8. 离线与 CI

wrap 会改变"离线"的性质：submodule 是 `git clone --recursive` 就齐全，wrap 是**首次 configure 需要网络**。

| 场景 | 做法 |
| --- | --- |
| 开发者首次配置 | 直接 `meson setup`，wrap 按需下载；有系统库时（默认 `auto`）不下载 |
| 离线构建 | 先 `meson subprojects download` 预热，再 `--wrap-mode=nodownload` |
| CI | 缓存 `subprojects/packagecache` + `build/deps.json`；离线任务用 `--wrap-mode=nodownload` |
| 禁止回退（只允许系统库） | `-D<dep>_source=system` + `--wrap-mode=nofallback` |

## 9. 规则（写死，避免以后反复讨论）

1. **每个第三方依赖只有一份权威来源声明**，两套构建系统都从它派生；**禁止同一依赖同时存在第二种获取方式**。
2. **不写分支 revision**；只写 tag 或完整 commit。
3. **迁移一个依赖 = 一个提交**：加 wrap → 删 submodule → 更新 `.gitmodules` → 三套验证 + `deps.json` 报告。
4. **sqlite3 保持入库 amalgamation**；不要把 wrap "顺手"套回去。
5. **字体数据包是唯一保留的 submodule**；它不是构建依赖，迁移收益为零。
6. **新增依赖先问"上游用什么构建系统"**：CMake-only 加 `method = cmake`，否则默认 meson 方法。

## 10. 实施步骤与验收

| 步 | 内容 | 验收 |
| --- | --- | --- |
| 0 | 最小实验：`wrap-git` + `method = cmake` 取一个小型 CMake-only 库并成功 `dependency()`；顺带确认"wrap 与 submodule 共存时目录优先、`subprojects download` 不覆盖已有目录" | 实验结论写回本文 |
| 1 | `meson/deps.build` 里的 `resolve_dep()` + 三态开关 + 解析报告 + `build/deps.json` | 三种取值各跑一次：`system` 用系统库、`vendored` 用子项目、`auto` 回落并打印；CI 检查报告里没有空来源 |
| 2 | 逐个迁移（SDL3 优先，因为它是 D4 的生产后端） | 每迁移一个：三套验证 + `deps.json` 显示来源为 vendored + `--wrap-mode=nodownload` 下可离线构建 |
| 3 | 迁移完成后删除多余 submodule，保留 `caskaydia-cove` | `.gitmodules` 只剩字体包；干净检出（不带 `--recursive`）也能构建（wrap 下载） |
| 4 | （未来）`deps/manifest.toml` + 生成器 + `--check` 关卡 | 改一处清单 → 生成物变化；手工改生成物 → 关卡失败 |

**未实施前不要声明"依赖已可复现"**；当前状态是"策略已定，机制未落地"。
