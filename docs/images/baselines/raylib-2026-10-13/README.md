# Raylib 视觉基线（2026-10-13）

SDL 后端工作开始前，从当时的工作区采集的 raylib 参考画面。**这是人工视觉参考，不是像素级 golden**：
合成器缩放、窗口平铺与显示环境都会影响尺寸。

## 来源与构建

- HEAD：`3215822`（`feat(showcase): rebuild the Label page on a shared page chrome`）
- 工作区另含两处未提交的 `ComponentInfoTable::build() const` 改动。
- 后端：raylib 6.1-dev、GLFW Wayland、OpenGL 4.6 Core Profile。
- GPU：AMD Radeon RX 6750 GRE 12GB，Mesa 26.2.4。
- showcase 请求窗口 1200 × 800；合成器实际给出 1269 × 1383。

## 验证

`meson compile -C buildDir && meson test -C buildDir --print-errorlogs`

- 编译通过。
- 测试：87 通过 / 0 失败（79 unit + 8 integration）。完整日志见 `buildDir/meson-logs/testlog.txt`。

## 采集内容

- `00-home.png`：showcase 首页，亮色外观，初始路由与滚动位置。
- Anchors 与 Label 页面**没有**采集到：Wayland 输入注入没能完成导航，因此没有图像。

## 与后续改动的关系（2026-10-14 更新）

本基线采集于[布局收敛与文本测量](../../../references/layout_convergence_and_text_measurement.md)的修复之前。修复后：

- **首页（本基线所示页面）的几何不变**：首页没有使用 `cross_alignment(stretch)`，
  也没有 `max_width` 与滚动容器的组合，因此三条几何相关的修复对它都是 no-op。
- **Label 页面会变**：修复前 `percent` 宽度在 stretch 父级下会被拉满（静默失效），
  修复后按作者声明的百分比生效，"Wrapping 68%" 与 "Overflow 55%" 两节的实际宽度因此改变。
  该页面本来就没有基线，需要重新采集。
- 修复前后的性能对比与 `layout_root` 收敛证据见该 reference §1 与 §6。

## 使用方式

与 SDL 后端做视觉对比时，请先**重新采集**一份当前后端的基线，并记录实际窗口/像素尺寸；
不要直接把本文件与不同窗口尺寸下的截图做像素比较。采集方式应当复用
`playground/bench/label_bench.cpp` 那种「固定路由 + 固定帧数 + 记录实际尺寸」的做法。
