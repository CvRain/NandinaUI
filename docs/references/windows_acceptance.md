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
cd D:\playground\NandinaUI
git pull --ff-only
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
  6 条 `platform = "windows"` 的 candidate（CPU 那 3 条会覆盖已有 approved 的三张图，
  内容应逐字节一致）；
- **回报**：命令输出 + 告诉我「采完了」，我拉取 manifest 与 PNG 做确定性复核、目检并批准；
  如果 sdf 模式报 GPU/swapchain 错误，把错误发我，我把 SDF 路径继续标为待办。

## 6. Windows 性能基线（P1.3，可选）

```powershell
meson setup buildDir-release --buildtype=release -Dsdl_page_probe=true --wrap-mode=nodownload
meson compile -C buildDir-release
python3 tools/backend_performance.py --probe buildDir-release\probe\cross_platform\desktop\nandina_sdl_page_probe.exe --pages home,label,anchors --modes sdf,cpu --interaction none --repeats 5 --output probe\baselines\windows-release-none.json
python3 tools/backend_performance.py --probe buildDir-release\probe\cross_platform\desktop\nandina_sdl_page_probe.exe --pages label --modes sdf --interaction cycle --repeats 5 --output probe\baselines\windows-release-cycle.json
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
| 5. Windows SDF 页面 golden | 候选已采集 | home/label/anchors 三条 candidate；home 的 sha 与 CPU 基线相同（该页无 SDF 图元），label/anchors 目检通过；**待第二次采集确认确定性后批准** |
| 6. 性能基线 | 未跑 | 可选（P1.3） |

> 注意：第一次 DXIL 运行曾因终端残留 `NANDINA_SDL_SDF=off` 在 CPU 回退路径上失败；清掉环境变量后通过。测试现已对这种情况快速失败并给出可操作提示。

## 8. 已知边界

- SSH 会话（session 0）只能跑 CPU 路径（`--renderer sdl2d`）与单元测试；GPU/SDF 相关一律走
  桌面会话；
- 跨平台 golden 像素差异（字体栅格化/DPI）是预期，不参与门禁；
- macOS/MSL 没有测试机，不在本清单内。
