# 第三方依赖策略：来源、开关与版本

> **状态：策略已接受，机制未实施；2026-10-17 核对修订。**
> 本文统一依赖来源、功能开关、版本与交付规则。当前仍使用 submodule、系统依赖和入库
> SQLite；本文中的新选项、报告与清单均为目标形态。
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

### 2.1 仓库现状

- 10 个库源码 submodule，另有 1 个字体数据 submodule；当前没有依赖 wrap 文件。
- raylib、box2d、spdlog、utf8proc 经 CMake 适配；tomlplusplus 当前也经 CMake 接入，
  即使其上游提供其他构建入口，也不能把当前接法记成 Meson。
- FreeType、HarfBuzz、FriBidi、Catch2、nlohmann_json 使用原生 Meson 子项目。
- SDL3 通过系统 `dependency('sdl3', version: '>=3.4.0')` 获取，OpenSSL 也来自系统。
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

目标输出为配置 summary 与构建目录内的 `deps.json`，每项包含：

- 名称、是否启用、选择的来源、实际版本与源码 commit；
- min/pin、实际路径、构建方法及影响行为的关键选项；
- 构建机工具或目标机依赖的角色，以及未满足/跳过的原因。

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
| SDK 导出 | 收集解析后的依赖源码、许可证和版本信息，保持源码包与 manifest 一致 |
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
| 1 SDL3 最小实验 | 固定源码 + CMake wrap + 实际 target/选项/依赖消费；记录 Meson 版本 | 干净目录构建并链接一个实际消费者；核对 fallback 行为与产物 |
| 2 SDL3 正式接入 | 三态来源、功能开关、解析报告、CI 与 SDK | 三态与 disabled 组合、实际 SDL 消费、离线 SDK、三套代码验证 |
| 3 其余依赖逐项迁移 | 每项单独提交，必要时较大依赖拆实验与接入两批 | 保持受测版本；删除该 submodule 跟踪项；报告、离线/SDK 与三套验证通过 |
| 4 完成迁移 | 字体保留 submodule，SQLite 保留 amalgamation，相关说明与清单同步 | 全新检出、联网预热、断网源码包消费完整走通 |
| 5 可选清单生成 | 有实际收益时引入 manifest 与生成器 | 单一声明、生成物一致性关卡；不成为前四批前置 |

迁移期间保留未迁移 submodule 是正常的；单项切换后不得留下两份互相竞争的版本声明。
删除 submodule 跟踪配置不等于授权丢弃本地改动；实施前检查每个目录状态。

本轮只核实机制文档和仓库状态，尚未运行 SDL3 wrap 最小实验，也未交付新开关、deps.json、
shader 产物包或 SDK 改造。
