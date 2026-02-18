# NandinaUI

![](./Image/placeholder.jpg)

**南天竹： 一套使用 Catppuccin 配色方案的扁平设计 QML 组件库**  
**NandinaUI: A flat design QML component library based on Catppuccin scheme**

- 🎨 基于 Catppuccin 配色方案的现代化设计
- 📱 响应式设计，支持多种屏幕尺寸
- 🎯 容易上手的 QML 组件
- 🌙 内置四色主题切换并支持自定义主题
- 🔧 参考Shadcn的模块化设计
- 🚀 流畅的动画效果

[English](docs/README_EN.md) 

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

### 2) 视觉语言：
- [Catppuccin](https://github.com/catppuccin/catppuccin) - 配色方案
- [shadcn/ui](https://ui.shadcn.com/) - 组件设计理念
- [Skeleton UI](https://www.skeleton.dev/) - 布局与交互设计

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

## 计划中

> 版本策略：先补“设计系统底座”，再做基础控件，最后扩展复杂场景，减少后续 API 返工。

详细规范见：[`docs/v0.2-component-spec.md`](docs/v0.2-component-spec.md)

### 开发路线摘要（公开）
- 架构分层：`Nandina.Color` / `Nandina.Theme` / `Nandina.Tokens` / `Nandina.Controls` / `Nandina.Window`
- 主题策略：组件主题解析优先级为「显式 themeManager > 父级/ThemeScope 继承 > 组件 fallback」
- Token 策略：逐步将字体、间距、圆角、动效抽离为 Design Tokens，避免组件内硬编码
- API 策略：采用语义化版本，新增能力走 `MINOR`，破坏性改动仅在 `MAJOR`
- 组件演进：先完成基础表单控件一致性（状态机/键盘/主题），再扩展复杂导航与数据组件

### 文档说明
- `docs/` 目录保留对外公开规范（当前为 v0.2 组件规范）
- 内部分析与设计草案迁移至私有目录，不作为对外稳定文档承诺

**阶段 0: 设计系统底座 (v0.1.x)**
- [ ] Design Tokens（spacing / radius / typography / motion）
- [ ] 交互状态语义（normal / hover / pressed / focus / disabled / error）
- [ ] 控件基础行为约定（键盘操作、焦点可见性、禁用态）
- [ ] 主题映射规范（Token -> PaletteCollection）
- [ ] 文档：组件 API 命名与属性分层规范

**阶段 1: 基础表单组件 (v0.2.0)**
- [ ] 按钮 (Button)
- [ ] 输入框 (Input)
- [ ] 标签 (Label)
- [ ] 开关 (Switch)
- [ ] 复选框 (Checkbox)
- [ ] 单选组 (Radio Group)
- [ ] 文本区域 (Textarea)
- [ ] 滑块 (Slider)

**阶段 2: 基础布局组件 (v0.3.0)**
- [ ] 卡片 (Card)
- [ ] 分隔符 (Separator)
- [ ] 滚动区域 (Scroll Area)
- [ ] 选项卡 (Tabs)

**阶段 2.5: 通用反馈闭环 (v0.3.5)**
- [ ] 对话框 (Dialog/Alert Dialog)
- [ ] 提示框 (Toast/Sonner)
- [ ] 工具提示 (Tooltip)
- [ ] 加载指示器 (Spinner/Progress)

**阶段 3: 复杂布局与导航 (v0.4.0)**
- [ ] 侧边栏 (Sidebar)
- [ ] 抽屉 (Drawer)
- [ ] 可调整大小组件 (Resizable)
- [ ] 导航菜单 (Navigation Menu)

**阶段 4: 数据展示 (v0.5.0)**
- [ ] 列表 (List)
- [ ] 分页 (Pagination)
- [ ] 表格 (Table/Data Table)
- [ ] 树形视图 (Tree View)
- [ ] 骨架屏 (Skeleton)
- [ ] 空状态 (Empty)

**阶段 5: 选择与工具组件 (v0.6.0)**
- [ ] 下拉菜单 (Dropdown Menu)
- [ ] 组合框 (Combobox)
- [ ] 日期选择器 (Date Picker)
- [ ] 颜色选择器 (Color Picker)
- [ ] 头像 (Avatar)
- [ ] 徽章 (Badge)
- [ ] 面包屑 (Breadcrumb)
- [ ] 步骤条 (Steps)
- [ ] 时间轴 (Timeline)

### v0.2.0 最小可交付标准（建议）
- 组件覆盖：Button / Input / Label / Switch / Checkbox（优先）
- 每个组件至少提供：`normal/hover/pressed/focus/disabled` 五态
- 每个输入型组件至少提供：`error/success/helperText`
- 主题切换时无明显闪烁，过渡时长与 `ThemeManager` 保持一致
- 键盘可用：Tab 可聚焦，Enter/Space 触发核心行为

### API 设计约束（建议）
- 属性优先语义命名，避免把视觉细节暴露为主 API
- 先保证基础组件行为稳定，再扩展复杂样式变体
- 所有组件保持一致的状态字段命名（例如 `disabled`, `invalid`, `size`, `variant`）
- 尽量支持组合式用法，避免过度“大而全”单体组件


## 贡献

欢迎提 Issue / PR，特别是：
- 组件 API 一致性建议
- 主题 Token 设计建议
- 可复现的视觉或交互问题

## License

See [LICENSE](LICENSE).


