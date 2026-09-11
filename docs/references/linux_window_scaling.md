# Linux 窗口后端与缩放

NandinaUI 在 Linux 上支持 Wayland 和 X11，但每个构建目录只编译其中一个 GLFW 窗口后端。Meson 默认根据配置时的桌面会话自动选择，也可以显式指定：

```bash
meson setup buildDir -Dlinux_window_system=wayland
meson setup buildDir -Dlinux_window_system=x11
```

已有构建目录可以使用 `meson configure buildDir -Dlinux_window_system=wayland` 切换，随后重新编译。

## 为什么不同时编译两个后端

当前随项目提供的 raylib/GLFW 可以在一个二进制中编译 Wayland 与 X11，但 raylib 的部分高 DPI 输入回调仍使用编译期平台宏。双后端构建运行在 X11 或 XWayland 时，窗口从一个缩放比例不同的显示器移动到另一个显示器后，绘制比例会更新，鼠标比例却可能继续使用原来的值，最终表现为控件绘制区域与命中区域错位。

为避免把上游后端细节泄漏到 NandinaUI 的输入 API，项目在配置阶段确定实际窗口系统。这样 raylib 的窗口尺寸、帧缓冲尺寸和鼠标坐标始终采用同一平台分支。

## 多显示器验证

涉及 DPI 或窗口后端的改动至少应验证：

- 相同缩放比例的显示器之间移动窗口；
- 不同缩放比例的显示器之间双向移动窗口；
- 移动后按钮边缘、文本输入光标和滚动区域的命中；
- 调整窗口尺寸后再次跨屏；
- Wayland 原生与 X11/Xorg 分别建立独立构建目录测试。

日志中的 `PLATFORM: DESKTOP (GLFW - Wayland)` 或 `PLATFORM: DESKTOP (GLFW - X11)` 可以确认实际后端。处于 Wayland 桌面时看到 Xorg/XWayland 标识，通常表示构建目录选择了 X11；重新以 `-Dlinux_window_system=wayland` 配置即可。
