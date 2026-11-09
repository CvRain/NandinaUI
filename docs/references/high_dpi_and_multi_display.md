# 高 DPI 与多显示器：契约、实施记录与剩余边界

> **状态（2026-11-09）**：维护者报告高 DPI 显示效果不佳后，本批完成 **P4.5a 内容缩放基础**：
> 逻辑单位契约、平台 seam 归一化、窗口/渲染/输入/IME 输入区换算，以及测试与探针使用的
> 固定 1× 模式；并在本机 175% 显示器上完成真实验收。
> **P4.5b 混合 DPI 多显示器**（同机 100%/175% 双屏跨屏移动）仍待维护者新增第二块显示器后验收；
> 真实 IME/候选窗位置、Linux Wayland 分数缩放与无障碍文本缩放不在本批。
> 本文既是契约也是实施记录，未实施项明确标注，不得当作已支持。

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

## 4. P4.5b：混合 DPI 多显示器（待第二块显示器）

维护者将为本机新增第二块显示器用于不同缩放验收。实施/验收入口：

- 在 100% ↔ 175% 显示器之间**双向**移动同一窗口：逻辑尺寸保持不变，渲染 scale 与
  framebuffer 跟随新显示器；无闪烁、无错位、无崩溃。
- 移动后按钮/滚动条/文本光标/弹层定位的物理命中与视觉位置一致；`TextInputArea` 与
  IME 候选窗跟随新缩放。
- 窗口在跨屏拖动过程中（Windows `WM_GETDPISCALEDSIZE` 曲线）不出现布局抖动或光标偏移。
- 第二显示器接入/拔出与系统缩放设置变化时的 `DISPLAY_SCALE_CHANGED` 处理；必要时补
  `WINDOW_DISPLAY_CHANGED` 的回归。
- Linux Wayland 分数缩放（1.25/1.5）在维护者 Linux 机上按相同矩阵复验；如 SDL 语义不同，
  回到本文第 2 节重新定形 `content_scale`，不得只在单一平台修补。
- **已知实施风险（P4.5b 收敛）**：`WINDOW_PIXEL_SIZE_CHANGED` 与
  `WINDOW_DISPLAY_SCALE_CHANGED` 的到达顺序会影响“逻辑尺寸回写→按新缩放恢复窗口”的结果；
  当前按事件到达顺序更新，混合 DPI 首次移动时可能出现一次布局误差。P4.5b 用真实双屏确认
  是否需要去抖或合并到帧边界处理。

## 5. 非目标与明确延后

- 无障碍文本缩放/系统字体缩放、HDR、每显示器色彩管理不在本批承诺内。
- 最大化/全屏与多屏工作区选择策略、窗口置顶/无边框组合尚未定形；创建与缩放变化已做
  工作区可见性钳制（见 §2 第 3 条），其余由 OS 决定。
- macOS 仍无测试机；MSL 只保留静态产物。
- 不把 P4.5a 当作“高 DPI 已完成”：混合 DPI、真实 IME 与 Linux 分数缩放仍待验收。
