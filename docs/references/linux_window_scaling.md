# Linux 窗口后端与缩放

> **历史勘误（2026-10-20）**：raylib/GLFW 已退役。下面关于旧实现“每个构建目录只编译一个
> GLFW 后端”的描述保留为历史记录，不适用于当前 SDL3 构建。当前行为见下文。

NandinaUI 在 Linux 上支持 Wayland 与 X11，窗口/输入/帧驱动由 SDL3 提供。

## 构建期选择与运行期驱动

vendored SDL3 可以**在同一个二进制里同时编译 X11 与 Wayland 驱动**，运行时由 SDL 根据
环境选择（`WAYLAND_DISPLAY` 可用时通常选 Wayland，否则 X11/XWayland）。

`-Dlinux_window_system=` 只控制**vendored SDL3 编译哪些驱动**：

- `auto`（默认）：交给 SDL 的 CMake 探测，按环境里可用的开发包构建；
- `x11` / `wayland`：只启用指定驱动，可用来说明“必须走某条路径”，或减少依赖面；
- system SDL3（`-Dsdl3_source=system`）的驱动由发行版构建决定，本选项不重编译它。

切换该选项需要**重新配置并重编 SDL3**：

```bash
meson configure buildDir -Dlinux_window_system=wayland
meson compile -C buildDir
```

运行期可以用 `SDL_VIDEODRIVER=wayland` / `SDL_VIDEODRIVER=x11` 临时强制驱动；应用内可用
`SDL_GetCurrentVideoDriver()` 查询实际驱动，或看窗口初始化日志。

## 多显示器验证

涉及 DPI 或窗口后端的改动至少应验证：

- 相同缩放比例的显示器之间移动窗口；
- 不同缩放比例的显示器之间双向移动窗口；
- 移动后按钮边缘、文本输入光标和滚动区域的命中；
- 调整窗口尺寸后再次跨屏；
- Wayland 原生与 X11/Xorg 分别建立独立构建目录测试（`-Dlinux_window_system=x11` / `wayland`）。

历史记录（仅作背景，不适用于 SDL3）：旧 raylib/GLFW 构建每个目录只编译一个窗口系统，
且其高 DPI 输入回调使用编译期平台宏；当时建议在配置阶段固定窗口系统以避免“绘制比例更新、
鼠标比例不更新”的错位。该缺陷与限制随 raylib 退役一起退出，SDL3 的 DPI/输入一致性仍需按
上面的矩阵重新实测。
