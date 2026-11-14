# Windows 人工验收清单（GUI / GPU 依赖项）

> 2026-10-21 建立。自动门禁（Linux 全量测试、Linux golden、设备级 `sdf-device`）覆盖不了
> **Windows 桌面会话里的 GPU swapchain 与真实窗口观感**。通过 SSH 运行的进程位于 session 0，
> `SDL_CreateGPURenderer` 拿不到 swapchain（`0x887A0022`），所以下列检查必须在
> **控制台登录 / RDP 的桌面会话里**打开终端执行（不是 SSH 会话）。
>
> 每项都写明「怎么跑、期望什么、怎么回报」。跑完把退出码、截图路径与现象发我，我通过 SSH
> 拉取产物并更新 manifest / 报告。

## 0. 准备

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cd D:\CLang\NandinaUI
git pull --ff-only
# SDK 打包夹具（nandina-subproject-fixture / sdk-vendored-consumer）会收录全部 wrap 源码；
# 默认构建关闭 physics2d 但 box2d 源码仍需先下载，否则夹具报 wrap dependency source missing。
meson subprojects download
ninja -C buildDir
```

> 每项之间清掉上一项留下的环境变量（尤其是 `NANDINA_SDL_SDF`）：
>
> ```powershell
> Remove-Item Env:NANDINA_SDL_SDF -ErrorAction SilentlyContinue
> Remove-Item Env:NANDINA_SHOWCASE_THEME -ErrorAction SilentlyContinue
> ```
>
> 忘了清 `NANDINA_SDL_SDF=off` 时，`sdf_device_test` 现在会直接报
> `NANDINA_SDL_SDF=off disables the SDF path` 并退出 1，而不是抛几十条像素断言失败。

`ninja` 结束后 `buildDir\tests\*.exe` 与 `buildDir\showcase\nandina_showcase.exe` 旁会自动
复制运行库 DLL；静态 SDL3 构建不会出现 `libSDL3_shared.dll`（若出现说明构建目录残留，
删除即可）。

## 1. 真实窗口回归（默认主题）

```powershell
.\buildDir\showcase\nandina_showcase.exe
```

- 依次切到 home / label / anchors 页面，滚动、hover、按下按钮、打开/关闭 dialog；
- **期望**：页面结构与 Linux 截图一致（字体栅格化/DPI 不同导致的字宽差异是预期的），
  没有空白页、错位、闪烁、崩溃；
- **回报**：正常/异常；异常时截图 + 控制台最后 20 行日志。

## 2. 软阴影（butter 主题）

```powershell
$env:NANDINA_SHOWCASE_THEME = 'butter'
.\buildDir\showcase\nandina_showcase.exe
```

- butter 的卡片配了 `shadow_spread = 10`；label / anchors 页的卡片应能看到**柔和渐变阴影**，
  边缘没有硬边；
- 对照：关掉 SDF（CPU 回退）后阴影是硬边圆角：

```powershell
$env:NANDINA_SDL_SDF = 'off'
.\buildDir\showcase\nandina_showcase.exe
```

- **期望**：SDF 打开时阴影边缘呈平滑衰减；`NANDINA_SDL_SDF=off` 时是同尺寸硬边阴影；
- **回报**：两张截图（SDF 开/关），或直接描述「一边柔一边硬」是否符合。

## 3. 涟漪圆角裁剪

同一 butter 主题窗口，点击按钮/开关：

- **期望**：涟漪圆形被**圆角容器轮廓**裁掉，圆角处不应出现溢出的圆角块；
- 对照 `NANDINA_SDL_SDF=off`：圆角处退化为方角裁剪（这是文档化的能力差异）；
- **回报**：动画中间帧截图（SDF 开/关各一张）或文字描述。

## 4. GPU 设备级验收（DXIL）

```powershell
Remove-Item Env:NANDINA_SDL_SDF -ErrorAction SilentlyContinue
.\buildDir\tests\sdf_device_test.exe --require-gpu --expect-format dxil
```

- **期望**：输出 `SDF device pixels, viewport scales, fallback, frame restore and eviction passed`，
  退出码 0；其中包含新增的 `shadow soft band`、`rounded corner cuts circle`、
  `circle edge cuts fill` 断言；
- **回报**：退出码 + 末尾几行。若报 `Cannot create GPU renderer` / swapchain 错误，说明仍在
  SSH 会话，请在桌面终端重跑。

## 5. Windows SDF 路径页面 golden（P1.2 收尾）

```powershell
meson configure buildDir -Dsdl_page_probe=true
meson setup --reconfigure buildDir
ninja -C buildDir
python3 tools/capture_backend_golden.py --probe buildDir\probe\cross_platform\desktop\nandina_sdl_page_probe.exe --platform windows --pages home,label,anchors --modes sdf,cpu
```

- **期望**：输出 6 行 `captured ...`，新增 6 张 `candidates\*-windows.png`，manifest 出现
  首批 6 条 `platform = "windows"` 的 candidate（2026-11-11 中文版重批后每平台 8 条）（CPU 那 3 条会覆盖已有 approved 的三张图，
  内容应逐字节一致）；
- **回报**：命令输出 + 告诉我「采完了」，我拉取 manifest 与 PNG 做确定性复核、目检并批准；
  如果 sdf 模式报 GPU/swapchain 错误，把错误发我，我把 SDF 路径继续标为待办。

## 6. Windows 性能基线（P1.3，可选）

> 2026-10-22：CPU 路径（SSH，sdl2d）与 SDF 路径（桌面会话，DXIL）均已采集并归档，
> 见[性能预算](performance_budget.md) §9。下列命令保留为复现步骤。

```powershell
meson setup buildDir-release --buildtype=release -Dsdl_page_probe=true --wrap-mode=nodownload
meson compile -C buildDir-release
python3 tools/backend_performance.py --probe buildDir-release\probe\cross_platform\desktop\nandina_sdl_page_probe.exe --pages home,label,anchors --modes sdf,cpu --interaction none --repeats 5 --output probe\cross_platform\results\backend-report-windows-release-none.json
python3 tools/backend_performance.py --probe buildDir-release\probe\cross_platform\desktop\nandina_sdl_page_probe.exe --pages label --modes sdf --interaction cycle --repeats 5 --output probe\cross_platform\results\backend-report-windows-release-sdf-cycle.json
```

- **期望**：两份 JSON 报告 + CSV 目录；报告里的数字只用于 Windows 自身对照，**不跨平台比较**；
- **回报**：告诉我「报告已生成」，我通过 SSH 拉取并归档到 `probe/baselines/`，在
  `performance_budget.md` 记录 Windows 基线（标明 CPU/SDF 路径与采集环境）。

## 7. 已验证记录（2026-10-21，桌面会话）

| 项 | 结果 | 证据 |
| --- | --- | --- |
| 1. 真实窗口回归 | 通过 | 维护者桌面截图：home / label / anchors（含暗色）布局、滚动、hover、dialog 正常 |
| 2. 软阴影 | 通过 | butter 主题下卡片阴影为柔和渐变；`NANDINA_SDL_SDF=off` 对照出现硬边与锯齿 |
| 3. 涟漪圆角裁剪 | 通过 | butter 主题按钮点击无溢出；`NANDINA_SDL_SDF=off` 退化为方角（符合能力差异） |
| 4. DXIL 设备测试 | 通过 | `--require-gpu --expect-format dxil` 退出 0；末尾 `SDF device pixels, viewport scales, fallback, frame restore and eviction passed`。直线 AA 的 MAE 0.048/0.078/0.081 与 Linux 记录一致；圆面积参考 max ≤17；新增 shadow/clip 断言通过 |
| 5. Windows SDF 页面 golden | **已批准** | home/label/anchors 三条 SDF approved；第二次采集逐字节一致（上传文件与仓库候选 sha 相同）；home 的 sha 与 CPU 基线相同（该页无 SDF 图元） |
| 6. 性能基线 | **已完成（2026-10-22，参考）** | CPU 路径经 SSH（`--renderer sdl2d`）、SDF 路径在桌面会话（DXIL，能力行确认软阴影/圆角裁剪生效）；4 份原始报告 + 8 条参考基线已入库，见[性能预算](performance_budget.md) §9。只作 Windows 自身参考，不跨平台比较、不参与计时门禁 |

> 注意：第一次 DXIL 运行曾因终端残留 `NANDINA_SDL_SDF=off` 在 CPU 回退路径上失败；清掉环境变量后通过。测试现已对这种情况快速失败并给出可操作提示。

## 8. 已知边界

- SSH 会话（session 0）只能跑 CPU 路径（`--renderer sdl2d`）与单元测试；GPU/SDF 相关一律走
  桌面会话；
- 跨平台 golden 像素差异（字体栅格化/DPI）是预期，不参与门禁；
- **高 DPI 内容缩放（P4.5a 已实现 2026-11-09；P4.5b 混合 DPI 已验收 2026-11-10）**：窗口/
  布局/事件/IME 输入区统一为逻辑单位，平台按
  `content_scale = display_scale / pixel_density` 放大原生窗口，SDL 渲染器与字形栅格按
  display scale 到物理像素；本机 175% 实测 1200×800 逻辑 → 2100×1400 物理、文字清晰、指针
  命中正确。混合 DPI 在真实双屏（2880×1800@175% + 2560×1440@125%）验收：双向跳转、真实
  标题栏拖动跨屏、125% 下 resize 后回 175% 均保持逻辑尺寸并更新 framebuffer/render scale，
  两个缩放下物理坐标命中与可用区域钳制正确。真实 IME 候选窗、Linux Wayland 分数缩放、
  显示器热插拔与系统缩放在线变化仍未验收。契约见
  [高 DPI 与多显示器](high_dpi_and_multi_display.md)。测试/探针/golden 用
  `WindowConfig::high_dpi=false` 保持固定 1×。
- **人工视觉验收（2026-11-10，showcase 全页 @175%）**：5 个页面（首页 / 锚点 / Label / Image /
  输入与焦点）× 亮/暗共 10 张，加 6 个交互状态（导航 hover、输入框 hover / focus、按钮 pressed、
  对话框打开与 Esc 关闭焦点恢复），由维护者逐页审阅，结论全部通过。发现并处理两点：① Label 页
  Font style 标本（Regular / Medium / Bold / Italic）当前无视觉差异——内置默认字体族只注册
  Caskaydia Cove Regular 一个 face，请求按“最近 face”解析且无合成字重/斜体，已记录为 P4.7；
  **后续：P4.7 合成回退已于 2026-11-10 实施**（分级 synthetic bold + 约 12° oblique，HarfBuzz
  advance 同步；高视口重采确认 Medium/Bold/Italic 可见），默认 1200×800 页面 golden 仍保持
  0 差异，详见 [合成字重/斜体回退](font_synthesis.md)；
  ② 「可编辑字段」缺少 hover 反馈——已补 TextField 默认 hover 配方（accent 底 + ring 边框，
  focused 时复位），证据图中 hover / focus 已更新。采集时窗口移离屏幕边缘，避免系统通知污染截图；
  每张页面图都经脚本校验侧栏选中项与页面一致。逐页清单、逐图与结论：
  [windows_visual_2026-11-10](../images/windows_visual_2026-11-10/review.md)，
  页面/状态总览：[contact-pages.png](../images/windows_visual_2026-11-10/contact-pages.png)、
  [contact-states.png](../images/windows_visual_2026-11-10/contact-states.png)。
- macOS/MSL 没有测试机，不在本清单内。

## 9. 全量套件与工具链 Windows 复验（2026-11-09）

> 本节是**自动化层面**的 Windows 复验记录；§1–§5 的真实窗口观感与软阴影/涟漪目检，以及
> P3.3b2b 的真实 IME/候选窗位置，仍由维护者在桌面会话人工验收，本节不替代那些项。

**环境**：Windows 11 专业版 10.0.26200（64 位）；AMD Ryzen 5 6600H + Radeon 660M（驱动
31.0.21024.2004）；MSYS2 UCRT64（GCC 16.2.0、Meson 1.12.1、Ninja 1.13.2；本机 Meson 与 CI
固定的 1.7.2 不同，只作本机复验证据）；`buildDir` debug 配置；集成测试由 Meson 调用的
MSYS2 Python 3.14 执行。

**结论**：修复 5 项 Windows 可移植性缺陷后，全量套件 **125 OK / 0 FAIL**（unit 104 +
integration 21；SDK 消费夹具按平台不注册，另新增 `header-flag-parsing` 回归）；`sdf-device`
在真实 D3D12 上通过，并包含 §16.1 的离屏 framebuffer 与**默认 window swapchain** 合成读回；
软件路径读回 1 用例 / 11 断言通过。

**复验前基线**：首轮 `--suite unit` 104/104 通过；首轮全量套件 117 OK / 8 FAIL。失败全部集中在
工具链与集成测试脚本，不涉及框架运行时代码。

| 失败项 | 根因 | 修复 | 红 → 绿证据 |
| --- | --- | --- | --- |
| `shader-artifacts` / `shader-artifacts-negative` | 产物头记录的是 shader 源文件 SHA-256；Windows `core.autocrlf` 检出为 CRLF，哈希与 LF 基线不符 | `tools/shader_artifacts.py` 哈希前把 CRLF/孤立 CR 归一化为 LF；`--emit` 时抑制 stdout 换行转写 | 修复前 negative 测试报 `shader source changed`；新增 `test_source_hash_ignores_checkout_line_endings` 先红后绿 |
| `header-surface` | `compile_commands.json` 的 Windows `command` 是带引号字符串且路径用反斜杠；原解析按空格拆分、按 `/nandina/` 过滤，收集到 0 个 `-I`，`nlohmann/json.hpp` 找不到 | `command_tokens` 优先 `arguments`、否则 `shlex.split`；匹配与包含路径归一化分隔符，相对 include 按编译目录解析为绝对路径；合成 TU 使用 POSIX 头路径 | 修复前 0 个编译 flag、`json.hpp` 失败；修复后 51 个 flag、当时 198 个分类（197 个独立编译 + 1 个 `physics2d` 特性跳过；现 202 个头文件，188 experimental + 14 internal）通过；新增 `header-flag-parsing` 回归（4 用例）先红后绿 |
| `format-gate` | 假仓库根使用 POSIX `/repo`（Windows 上无盘符），路径 `relative_to` 抛错；失败清单输出 `\` 分隔路径 | 测试根改为可解析的绝对临时路径；`check_format.py` 失败清单改 `as_posix()` | 修复前 3 个用例失败；修复后 8/8 通过 |
| `tool-dependencies-fixture` | 夹具直接查找 `*.o`；Meson 在 Windows 上生成 `.obj` | 按平台接受 `*.o` / `*.obj` | 修复前 `StopIteration`；修复后通过 |
| `resource-equivalence` | 夹具资源被 `core.autocrlf` 转成 CRLF，字节大小/哈希与锁和期望值不符（6→7、24→25） | 新增 `.gitattributes`：字节契约夹具 `tests/fixtures/resource_equivalence/assets/**` 设 `-text`，禁用 EOL 转换（提交对象为 LF） | 修复前 `nanres validate` 报 lock 过期；修复后通过 |
| `nandina-subproject-fixture` / `sdk-vendored-consumer` | `subprojects/box2d` 未下载（默认构建关闭 physics2d，但 SDK 打包会收录全部 wrap 源码） | 环境侧执行 `meson subprojects download box2d` | 修复前报 `wrap dependency source missing`；下载后 fixture 通过 |
| `sdk-vendored-consumer` | 下游消费断言使用 Linux 加载器语义（`ldd`、共享 DSO rpath），且深层临时构建路径把 DLL 链接命令行推到 Windows CreateProcess 的 32K 上限 | 按平台条件只在 Linux 注册（`tests/meson.build`）；Windows SDK 消费交付保留为未验收边界 | 修复后 Windows 全量套件不再包含该平台不适用项 |

新增的 `header-flag-parsing` 回归固定两种 `compile_commands` 形状：把 `command_tokens`
退回旧的按空格拆分时，该测试只剩 `-std=c++26`、断言失败；恢复 `shlex` 后 4 用例通过。
`format-gate` 与 `shader-artifacts-negative` 的修复同样先在 Windows 上复现原失败再转绿。

**D3D12 读回证据**（桌面会话，非 SSH；默认 GPU renderer）：

```powershell
.\buildDir\tests\sdf_device_test.exe --require-gpu --expect-format dxil
.\buildDir\tests\sdl_visual_tests.exe "[compositor]"
```

前者退出 0，输出
`composite readback matches the shared compositor (encoded sRGB, tolerance 4)`、
`default swapchain readback matches the shared compositor (encoded sRGB, tolerance 4)` 与
`SDF device pixels, viewport scales, fallback, frame restore and eviction passed`；后者 1 用例 /
11 断言通过。对 [主题契约](theme_authoring_and_state.md) §16.1 的意义：Windows/D3D12 离屏
目标与**默认 window swapchain（8 位 SDR 目标）**都与共享编码 sRGB 合成器一致、线性空间
负向对照被拒绝；读回不可用或目标不是 8 位 sRGB 时测试打印 SKIP 原因、不伪装覆盖。默认目标
检查的故障注入（删掉第二层绘制）报 `actual=(45,83,157) expected=(123,101,108)` 并退出 1，
恢复后重新通过。Linux 默认 swapchain 与其他驱动仍未验证。

**建立复验的步骤**（当前签出 `D:\CLang\NandinaUI`）：

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cd D:\CLang\NandinaUI
git pull --ff-only
# 已有克隆首次拉取 .gitattributes 后需一次性重检出字节契约夹具，否则资源仍是旧 CRLF：
git checkout -- tests/fixtures/resource_equivalence/assets
meson subprojects download          # SDK 打包夹具需要全部 wrap 源码（含可选 box2d）
meson setup --reconfigure buildDir
ninja -C buildDir
meson test -C buildDir --print-errorlogs
# 全量套件通过后跑发布门禁；clang-format 必须固定 23.1.1（MSYS2 自带的 23.1.3 会被门禁拒绝）：
python -m venv buildDir\format-venv
buildDir\format-venv\Scripts\pip install clang-format==23.1.1
$env:PATH = (Resolve-Path buildDir\format-venv\Scripts).Path + ';' + $env:PATH
python tools\release_check.py --ci --build-dir buildDir
```

> 全量套件运行期间不要编辑工作区：`nandina-subproject-fixture` 会把源码树打包两次并逐字节
> 比较，编辑会制造假失败。

**仍未覆盖**：§1–§5 的人工视觉项（真实窗口观感、软阴影与涟漪目检）、P3.3b2b 的真实 IME 与
候选窗位置（含跨屏候选窗跟随）、Linux 默认 swapchain 与其他驱动的读回、高 DPI 的显示器
热插拔/系统缩放在线变化与 Linux Wayland 分数缩放（Windows 单屏与混合 DPI 内容缩放已验收，
见 §8 与 [高 DPI 与多显示器](high_dpi_and_multi_display.md)）、Windows 性能基线复采、
真实物理键鼠布局、SDK 在 Windows 上的下游消费交付。Windows 仍为实验性平台，无应用交付承诺。

## 10. OS 无障碍文字缩放的端到端验收（2026-11-13）

- 在 Windows 上临时将 `HKCU\Software\Microsoft\Accessibility\TextScaleFactor` 设为 150
  （验收前该值缺失，验收后已删除恢复），运行
  `NANDINA_EXPECT_TEXT_SCALE=1.5 buildDir\tests\system_text_scale_tests.exe`：真实注册表 →
  `WindowsSystemTextScaleSource` → `NanApplication` → `ThemeManager` 的
  `[app][windows][text-scale]` 用例全绿（39 断言 / 4 用例，其中断言有效缩放为 1.5）。
  恢复原值后再次运行通过（36 断言）。
- 说明：本轮用「环境变量指定期望值」代替 GUI 截图，因为当前会话无法可靠捕获窗口。证据覆盖
  OS → app → theme 链路，**不代替**“用户在系统设置里改文字大小后窗口文字重排”的人工目检；
  后者仍是 `typography_scale.md` §8/§9 记录的剩余验收项。
