# NandinaUI

![](./Image/placeholder.jpg)

**NandinaUI V2（重构中）**

一个面向 Qt/QML 的组件系统：
- 组件行为与组合思路参考 shadcn（可组合、语义化、低耦合）。
- 视觉语言参考 Svelte Skeleton（简洁、层次清晰、轻动效）。
- 主题系统基于 Catppuccin，并持续向语义化 Token 演进。

[English](docs/README_EN.md) | [中文补充](docs/README_ZH.md)

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/releases/latest)
[![GitHub](https://img.shields.io/github/license/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/blob/main/LICENSE)
[![GitHub all releases](https://img.shields.io/github/downloads/Nandina/NandinaUI/total?style=flat-square)](https://github.com/Nandina/NandinaUI/releases)
[![GitHub issues](https://img.shields.io/github/issues/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/issues)


- 统一组件设计哲学（行为优先 + 视觉可替换）。
- 强化主题能力（语义色、状态色、渐变与过渡）。
- 保持 QML 端易用，同时给 C++ 侧保留扩展空间。

## 设计原则

### 1) 组件逻辑：shadcn 风格
- 以“可组合”代替“大而全单体组件”。
- 属性命名尽量语义化，避免“样式即 API”。
- 优先提供稳定基础件，再迭代复杂场景。

### 2) 视觉语言：Skeleton 风格
- 扁平化 + 轻层次，不做重阴影堆叠。
- 强调内容可读性与密度控制。
- 动效短促克制，优先反馈而不是炫技。

### 3) 主题策略：Token 化
- 通过 ThemeManager 暴露语义色板。
- 组件默认消费语义色，而不是硬编码颜色。
- 支持主题切换时平滑过渡（含窗口渐变动画）。

## 当前模块

### Nandina.Window
- NanWindow.qml
- DefaultTitleBar.qml
- TitleBarButton.qml
- CornerRectangle.qml
- WindowResizer.qml / ResizeHandler.qml

### Nandina.Theme
- ThemeManager（主题类型、当前色板、定制色板）

### Nandina.Color
- PaletteType、ColorCollection、PaletteCollection
- 内置 Catppuccin 主题：Latte / Frappe / Macchiato / Mocha

### Nandina.Core
- 当前作为基础支撑模块，持续演进中。

## 功能亮点（已落地）

- NanWindow 支持默认/无边框/自定义标题栏模式。
- 标题栏按钮支持 Canvas 自绘图标（最小化/最大化/还原/关闭）。
- 按钮支持强调色悬停模式（更贴近系统主题）。
- 窗口支持主题渐变背景与切换动画。
- 可根据主题明暗自动调整动画时长（也可手动覆盖）。

## 快速开始

### 环境要求
- Qt >= 6.5
- CMake >= 3.16
- C++23

### 方式一：作为子项目引入（推荐）

```bash
git clone https://github.com/CvRain/NandinaUI.git
```

在你的工程 CMakeLists.txt 中：

```cmake
add_subdirectory(NandinaUI)

target_link_libraries(YourApp PRIVATE
    Qt6::Quick
    Nandinaplugin
)
```

> 如果你的目录名不是 NandinaUI，请按实际路径调整 add_subdirectory(...)

### 方式二：直接运行仓库示例

```bash
cmake -S . -B build/Debug
cmake --build build/Debug -j4
./build/Debug/example/NandinaExampleApp
```

## QML 使用示例

```qml
import QtQuick
import Nandina.Color
import Nandina.Window

NanWindow {
    width: 960
    height: 640
    windowTitle: "NandinaUI V2"

    titleBarMode: NanWindow.CustomTitleBar
    windowRadius: 12

    enableThemeGradient: true
    autoAdjustThemeTransitionDuration: true

    Component.onCompleted: {
        themeManager.currentPaletteType = NandinaColor.Mocha
    }
}
```

## 贡献

欢迎提 Issue / PR，特别是：
- 组件 API 一致性建议
- 主题 Token 设计建议
- 可复现的视觉或交互问题

## License

See [LICENSE](LICENSE).


