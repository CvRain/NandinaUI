# 高 DPI 与多显示器：契约、实施记录与剩余边界

> **状态（2026-11-10）**：P4.5a 内容缩放基础已实施并在本机 175% 显示器验收（逻辑单位契约、
> 平台 seam 归一化、窗口/渲染/输入/IME 输入区换算、固定 1× 模式）。
> **P4.5b 混合 DPI 多显示器已在真实双屏上验收**：主屏 2880×1800 @175%、第二屏
> 2560×1440 @125%（Windows 报告为横向），覆盖双向跳转、真实标题栏拖动跨屏、缩放后
> resize 带回、两个缩放下物理坐标命中与可用区域钳制，见 §4。
> 真实 IME/候选窗位置、Linux Wayland 分数缩放与无障碍文本缩放仍不在本批；不得当作已支持。
> 本文既是契约也是实施记录，未实施项明确标注。

## 1. 修复前复现（2026-11-09）

维护者报告：高 DPI 显示器上应用观感不佳，并指出会牵出多显示器不同 DPI、组件缩放、文字渲染
与交互等新问题。

本机复现（Windows 11 26200，AMD Ryzen 5 6600H + Radeon 660M，面板 2880×1800，
`HKCU\Control Panel\Desktop\WindowMetrics\AppliedDPI = 168`，即 **175%** 系统缩放）：

| 观测 | 修复前 | 修复后（P4.5a） |
| --- | --- | --- |
| 请求逻辑尺寸 1200×800 | 客户区 **1200×800 物理像素** | 客户区 **2100×1400 物理像素**（外框 2124×1464） |
| `SDL_GetWindowSize` / `...InPixels` | 1200×800 / 1200×800 | 2100×1400 / 2100×1400 |
| 内容缩放（display scale / pixel density） | 未查询（渲染 1×） | 1.75（日志 `content_scale=1.75`） |
| 文字物理尺寸/清晰度 | 1×（物理偏小） | 按 1.75× 密度栅格，1:1 物理像素绘制 |

修复前整个窗口只占 175% 面板的一小部分，文字与命中目标偏小：见
[home（修复前，1224×864）](../images/high_dpi_2026-11-09/showcase-home-175pct.png)、
[label（修复前）](../images/high_dpi_2026-11-09/showcase-label-175pct.png)、
[输入与焦点（修复前）](../images/high_dpi_2026-11-09/showcase-input-focus-175pct.png)。

## 2. 契约（P4.5a 定形）

以下为已实现的作者/平台/渲染边界，后续批次不得静默改变：

1. **单位**：`WindowConfig::width/height`、布局、文本字号、事件坐标、语义 bounds 与
   `TextInputArea` 一律是**逻辑单位（DIP）**。物理像素只出现在平台 framebuffer 与渲染器
   出口。
2. **平台 seam（`IPlatformWindow`）**：
   - `screen_size()`：逻辑窗口尺寸（布局/作者空间）；
   - `drawable_size()`：framebuffer 物理像素；
   - `display_scale()`：物理像素 / 逻辑单位（Windows 175% = 1.75；Retina = 2；1× 模式 = 1）；
   - `poll_input()`：所有指针坐标已折回逻辑窗口坐标（与 `screen_size` 同一空间）。
3. **窗口创建**：作者尺寸是逻辑尺寸，平台把原生窗口设为
   `round(logical × content_scale)`，其中
   `content_scale = display_scale / pixel_density`：
   Windows 175% → 1.75（原生窗口坐标即物理像素）；Retina → 1.0（原生坐标是点，2× 只进
   framebuffer）。该换算由 `app/window_metrics.{hpp,cpp}` 的纯函数完成并有单测。
   内容缩放后的窗口可能超出按原始尺寸放置的位置：创建与缩放变化后把窗口收进当前显示器
   **可用区域**（最大化/全屏不改位置；大于工作区时贴左上保证标题栏可见）。
4. **渲染**：SDL 渲染器 scale 设为 `display_scale`，scene 以逻辑坐标绘制；SDF 路径从
   `SDL_GetRenderScale` 读取同一值换算设备像素；CPU 几何/裁剪由 SDL 按 render scale 放大。
5. **文本**：布局按逻辑字号；栅格尺寸 =
   `逻辑字号 × logical_to_screen × screen_to_physical`，其中
   `screen_to_physical = drawable_size / screen_size = display_scale`；字形纹理按
   `raster_pixels / display_scale` 的逻辑尺寸绘制，因此最终是 1:1 物理像素，不二次放大。
   图集缓存键已含物理栅格尺寸。
6. **指针输入**：鼠标/滚轮的窗口坐标除以 `content_scale`；滚轮**增量**不缩放；键盘与文本
   committed 路径不变。物理命中与逻辑布局因此一致（本机 175% 已用真实点击验收）。
7. **IME/caret**：场景协议（`TextInputArea`）保持逻辑坐标；SDL 适配器在调用
   `SDL_SetTextInputArea` 前乘 `content_scale` 换成原生窗口坐标，候选窗坐标由平台处理。
8. **固定 1× 模式**：`WindowConfig::high_dpi = false` 表示不按 OS 缩放创建/映射/渲染
   （`content_scale = 1`，`display_scale()` 返回 1）。测试、探针、golden 与确定性诊断必须
   用它；默认仍为 `true`。页面探针自带 1× 主机（`page_probe.cpp` 不经过 `NanWindow`），
   现有 golden 基线不受影响；golden 仍按 platform 隔离，**不跨 DPI 比较**。
9. **缩放变化**：`SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED` 时重算内容缩放、渲染 scale 与
   IME 输入区比例，并按作者逻辑尺寸恢复窗口；`SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` 回写
   用户 resize 后的逻辑尺寸。单显示器系统缩放变化已覆盖；跨屏混合 DPI 见 P4.5b。
10. **`ViewportScalePolicy` 组合**：DPI scale 是独立乘数，不进入设计视口的 mapping。顺序固定为
    `logical → (viewport logical_to_screen) → screen → (display_scale) → physical`。
11. **平台范围**：Windows（本机 175%）已验收；Linux 1× 行为不变，Wayland 分数缩放需要维护者
    在 Linux 侧复验；macOS 仍不支持。

## 3. P4.5a 实施记录（2026-11-09）

- `nandina/app/window_metrics.{hpp,cpp}` 新增纯函数：`content_scale_from_display`、
  `window_size_from_logical_size`、`logical_size_from_window_size`（无效输入回退 1×，
  尺寸非法抛 `invalid_argument`），并在 `tests/window_metrics_tests.cpp` 覆盖
  1.0/1.75/2.0 与无效值、往返转换。
- `nandina/app/detail/sdl_platform_window.{hpp,cpp}`：创建时按内容缩放设置原生窗口；
  `screen_size()` 返回逻辑尺寸；渲染器 scale 与文本输入区比例随窗口刷新；
  指针/滚轮坐标折回逻辑空间；处理 `WINDOW_DISPLAY_SCALE_CHANGED` 与
  `WINDOW_PIXEL_SIZE_CHANGED`；日志记录
  `ready 1200x800 logical (2100x1400 native, 2100x1400 pixels), content_scale=1.75`。
- `nandina/app/detail/sdl_text_input_service.{hpp,cpp}`：新增 `set_window_scale()`，
  caret 区域从逻辑坐标换算到原生窗口坐标后交给 SDL。
- 窗口放置：内容缩放后整窗可能超出按原始尺寸放置的位置（本机 2124×1464 在 2880×1800 工作区
  上初始位置越界）；创建与缩放变化后把窗口夹进当前显示器的可用区域，本机复验整窗可见。
- `nandina/app/platform_window.hpp`、`nandina/app/window_config.hpp`：写清 seam 与
  `high_dpi` 语义。

**真实验收（本机 175%，桌面会话）**：

| 检查 | 结果 |
| --- | --- |
| 窗口物理尺寸 | 日志 `ready 1200x800 logical (2100x1400 native, 2100x1400 pixels), content_scale=1.75 renderer=gpu`；外框 2124×1464 |
| 窗口放置 | 内容缩放后的整窗被夹进显示器可用区域（标题栏/边框不再越界），本机整窗可见 |
| 布局/文字 | 整窗按 175% 物理密度绘制，文字清晰且物理尺寸正常 |
| 指针命中 | 用 175% 物理坐标点击侧栏 Label，页面正确切换（坐标折回逻辑后命中一致） |
| 1× 回归 | `window-metrics`、`sdl-platform-input`（dummy 1×）、`window-lifecycle`（真实窗口）通过；页面探针/golden 路径未改 |

修复后对照图：[home（修复后，2124×1464）](../images/high_dpi_2026-11-09/showcase-home-175pct-scaled.png)、
[label（修复后）](../images/high_dpi_2026-11-09/showcase-label-175pct-scaled.png)。

## 4. P4.5b：混合 DPI 多显示器验收记录（2026-11-10）

维护者为当前系统新增第二块显示器：**主屏 2880×1800 @175%（工作区 2880×1716）+
第二屏 2560×1440 @125%（工作区 2560×1380，Windows 报告为横向，位于主屏右侧
x=2880）**。同一 showcase 窗口按下列场景实机验收，应用全程无崩溃：

| 场景 | 操作 | 结果与证据 |
| --- | --- | --- |
| 初始化（175%） | 启动 | 日志 `ready 1200x800 logical (2100x1400 native, 2100x1400 pixels), content_scale=1.75`；外框 2124×1464、被钳入可用区域；[截图](../images/high_dpi_2026-11-09/showcase-primary-175pct.png) |
| 175% → 125% 跳转 | `SetWindowPos` 跨屏 | `GetDpiForWindow=120`；日志 `display scale 1.75 -> 1.25; logical=1200x800 native=1500x1000 pixels=1500x1000 render_scale=1.25`；逻辑尺寸不变、framebuffer/render scale 跟随；[截图](../images/high_dpi_2026-11-09/showcase-secondary-125pct.png) |
| 125% 物理坐标命中 | 点击侧栏 Label（客户区 160,282） | Label 页正确切换，坐标折回逻辑后命中一致；[截图](../images/high_dpi_2026-11-09/showcase-secondary-125pct-label.png) |
| 真实标题栏拖动跨屏 | 按住标题栏分 14 步拖入第二屏 | 拖动过程中完成缩放切换：`display scale 1.75 -> 1.25; logical=1200x800 native=1500x1000 ... render_scale=1.25`；窗口被钳入第二屏可用区（日志 `clamp window position 3090,412 -> 3090,371 (usable 2880,0 2560x1380)`），无崩溃、无错位；[截图](../images/high_dpi_2026-11-09/showcase-after-drag-125pct.png) |
| 125% 下 resize 后回 175% | 第二屏 resize 到客户区 1600×1100（逻辑 1280×880），再移回主屏 | 回主屏后 `native=2240x1540`（=1280×880×1.75）、`render_scale=1.75`，逻辑尺寸随用户 resize 更新并在跨屏后保持；[125% resize](../images/high_dpi_2026-11-09/showcase-resized-125pct.png)、[回 175%](../images/high_dpi_2026-11-09/showcase-resized-back-175pct.png) |
| 125% → 175% 回跳 | `SetWindowPos` 回主屏 | `display scale 1.25 -> 1.75; logical=1200x800 native=2100x1400 ... render_scale=1.75`；物理坐标命中恢复；[截图](../images/high_dpi_2026-11-09/showcase-primary-175pct-restored.png) |

**事件顺序观察**：真实拖动时“窗口位置钳制”可能先于 `DISPLAY_SCALE_CHANGED` 到达
（钳制日志在 scale 日志之前），但随后按新 `content_scale` 恢复逻辑尺寸，最终状态正确；
本轮未观察到此前担心的“逻辑尺寸被旧 scale 回写导致缩窗”问题。诊断日志固定在每次缩放
事件打一行（`SdlPlatformWindow: display scale …`），便于后续平台复验。

**仍未验收的混合 DPI 项**：

- 显示器热插拔/拔出（会改变桌面布局，未在维护者机器上强制模拟）；
- 系统缩放设置在线变化（需要改系统设置，未执行）；
- 真实 IME 候选窗跨屏跟随（需真实输入法交互）；
- 跨屏拖动期间最大化/全屏/贴边吸附组合；
- Linux Wayland 分数缩放（1.25/1.5）在维护者 Linux 机上按相同矩阵复验；如 SDL 语义不同，
  回到本文第 2 节重新定形 `content_scale`，不得只在单一平台修补。

## 5. 非目标与明确延后

- 无障碍文本缩放/系统字体缩放、HDR、每显示器色彩管理不在本批承诺内。
- 最大化/全屏与多屏工作区选择策略、窗口置顶/无边框组合尚未定形；创建与缩放变化已做
  工作区可见性钳制（见 §2 第 3 条），其余由 OS 决定。
- macOS 仍无测试机；MSL 只保留静态产物。
- 不把 P4.5a/P4.5b 当作“高 DPI 全平台完成”：真实 IME 候选窗位置、Linux Wayland 分数缩放、
  无障碍缩放、显示器热插拔与系统缩放在线变化仍未验收。

## 6. 高 DPI 回归：DropdownMenu 打开使整页缩小（2026-11-11，已修复）

维护者在 175% 会话里点击输入与焦点页的「打开菜单」后，整个应用（含侧栏）瞬间缩小到约
0.63 并锚在左上角，但命中坐标仍按原布局工作。复现与定位：

- `MenuItemNode` 的 `primitives::Text` 成员没有转发场景树的默认文本管线/字体上下文，
  菜单文字因此走 `Text::on_draw` 的调试字体回退；
- `SdlRenderDevice::draw_text()` 在该回退路径里先 `SDL_SetRenderScale(font_size/8)`，
  然后把 renderer scale 硬重置为 `1.0`，覆盖了平台在 175% 下设置的 `1.75`；
- 逻辑布局/命中仍按 1200×800，而渲染 scale 变成 1，所以整页视觉缩小、交互位置错位。
  1× 及既有 golden 不暴露该问题（scale 本来就是 1）。

修复：

- `MenuItemNode` 像 `Button`/`BreadcrumbLink` 一样实现
  `apply_default_text_pipeline()` / `apply_font_context()`，把管线转发给
  `label_text_` / `shortcut_text_`，菜单标签改走真实字形管线；
- `SdlRenderDevice::draw_text()` 保存并恢复调用方的 renderer scale
  （按当前 scale × 调试字号比例绘制），不再假设调用方是 1×。

回归测试：

- `[dropdown-menu][render][text]`：装载默认文本管线后，菜单打开时标签由
  `ITextLayoutRenderer` 绘制，回退文本只剩预先挂载的触发按钮 "Open"（红先：修复前
  renderer 0 次、fallback 9 次）；
- `[visual][text][sdl][device]`：在 1.75 renderer scale 下调用调试字体回退，
  `SDL_GetRenderScale` 仍为 1.75（红先：修复前 1.0）；
- 真实 showcase 175% 会话复验：菜单打开后窗口/布局尺寸不变，中文菜单项清晰，
  整页不再缩放。

这是 P4.5 高 DPI 验收的一个新边界：**任何临时的 renderer scale 修改都必须成对恢复**，
不能用“重置为 1”代替保存/恢复。
