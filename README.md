# NandinaUI

![](./Image/placeholder.jpg)

**南天竹： 一套使用 Catppuccin 配色方案的扁平设计 QML 组件库**  
**NandinaUI: A flat design QML component library based on Catppuccin scheme**

[English](docs/README_EN.md)

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/releases/latest)
[![GitHub](https://img.shields.io/github/license/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/blob/main/LICENSE)
[![GitHub all releases](https://img.shields.io/github/downloads/Nandina/NandinaUI/total?style=flat-square)](https://github.com/Nandina/NandinaUI/releases)
[![GitHub issues](https://img.shields.io/github/issues/Nandina/NandinaUI?style=flat-square)](https://github.com/Nandina/NandinaUI/issues)

## ✨ 特性

- 🎨 基于 Catppuccin 配色方案的现代化设计
- 📱 响应式设计，支持多种屏幕尺寸
- 🎯 容易上手的 QML 组件
- 🌙 内置四色主题切换 (Latte、Frappe、Macchiato、Mocha)
- 🔧 高度可定制化
- 🚀 流畅的动画效果
- 🎭 SVG 图标系统

## 🚧 开发状态

NandinaUI 目前处于 **早期开发阶段**。正在积极构建核心组件。

### 开发路线图

#### 🟢 已完成

**基础窗口组件 (NandinaWindow)**
- [x] 自定义标题栏
- [x] 窗口拖拽与调整大小
- [x] 窗口控制按钮 (最小化、最大化、关闭)

**主题配色管理器 (ThemeManager)**
- [x] 4 种 Catppuccin 主题
- [x] 26 种语义化颜色
- [x] 动态主题切换
- [x] 响应式颜色更新

**图标系统 (NanIconItem)**
- [x] SVG 路径渲染
- [x] 枚举和名称访问
- [x] 主题颜色集成
- [x] 自定义颜色支持

**按钮组件 (NanButton)** ⭐ _最新完成_
- [x] 15 种预设样式（8 种填充 + 7 种描边）
- [x] 图标支持（左侧、右侧、仅图标）
- [x] 自适应文字大小
- [x] 悬浮提示（文字截断时自动显示）
- [x] 流畅的交互动画（悬浮、按下、点击回弹）
- [x] 高度可定制的样式系统

#### 📋 计划中 (按优先级排序)

**阶段 1: 基础表单组件 (v0.2.0)**
- [ ] 输入框 (Input)
- [ ] 复选框 (Checkbox)
- [ ] 单选框 (Radio Group)
- [ ] 开关 (Switch)
- [ ] 标签 (Label)
- [ ] 文本区域 (Textarea)
- [ ] 滑块 (Slider)

**阶段 2: 布局组件 (v0.3.0)**
- [ ] 卡片 (Card)
- [ ] 分隔符 (Separator)
- [ ] 滚动区域 (Scroll Area)
- [ ] 选项卡 (Tabs)
- [ ] 侧边栏 (Sidebar)
- [ ] 抽屉 (Drawer)
- [ ] 可调整大小组件 (Resizable)

**阶段 3: 数据展示 (v0.4.0)**
- [ ] 表格 (Table/Data Table)
- [ ] 列表 (List)
- [ ] 分页 (Pagination)
- [ ] 树形视图 (Tree View)

**阶段 4: 反馈组件 (v0.5.0)**
- [ ] 加载指示器 (Spinner/Progress)
- [ ] 对话框 (Dialog/Alert Dialog)
- [ ] 提示框 (Toast/Sonner)
- [ ] 工具提示 (Tooltip)
- [ ] 骨架屏 (Skeleton)
- [ ] 空状态 (Empty)

**阶段 5: 选择组件 (v0.6.0)**
- [ ] 下拉菜单 (Dropdown Menu)
- [ ] 组合框 (Combobox)
- [ ] 导航菜单 (Navigation Menu)
- [ ] 日期选择器 (Date Picker)
- [ ] 颜色选择器 (Color Picker)

**阶段 6: 工具组件 (v0.7.0)**
- [ ] 头像 (Avatar)
- [ ] 徽章 (Badge)
- [ ] 面包屑 (Breadcrumb)
- [ ] 步骤条 (Steps)
- [ ] 时间轴 (Timeline)

## 📦 组件使用示例

### NanButton 按钮

```qml
import Nandina.Components
import Nandina.Icon

// 基础按钮
NanButton {
    text: "Primary Button"
    type: "filledPrimary"
}

// 带图标的按钮
NanButton {
    text: "With Icon"
    vectorIcon: IconManager.ICON_ROCKET
    iconPosition: NanButton.IconPosition.Left
    type: "filledSuccess"
}

// 仅图标按钮
NanButton {
    vectorIcon: IconManager.ICON_SETTINGS
    iconPosition: NanButton.IconPosition.IconOnly
    type: "outlinedPrimary"
    width: 40
    height: 40
}

// 长文本自动显示悬浮提示
NanButton {
    text: "这是一个很长的文本会被截断"
    showTooltip: true
    width: 120
}
```

**可用样式类型：**
- **填充样式**：`filledPrimary`, `filledSecondary`, `filledTertiary`, `filledSuccess`, `filledWarning`, `filledError`, `filledSurface`, `default`
- **描边样式**：`outlinedPrimary`, `outlinedSecondary`, `outlinedTertiary`, `outlinedSuccess`, `outlinedWarning`, `outlinedError`, `outlinedSurface`

### NanIconItem 图标

```qml
import Nandina.Icon

// 使用枚举
NanIconItem {
    icon: IconManager.ICON_HOME
    width: 24
    height: 24
}

// 使用名称
NanIconItem {
    iconName: "rocket"
    width: 24
    height: 24
}

// 自定义颜色角色
NanIconItem {
    icon: IconManager.ICON_BIRD
    colorRole: "rosewater"  // 使用主题中的 rosewater 颜色
}

// 手动设置颜色
NanIconItem {
    icon: IconManager.ICON_SETTINGS
    colorRole: ""  // 禁用主题颜色
    color: "#ff0000"  // 使用自定义颜色
}
```

**可用图标：**
- 窗口控制：`ICON_CLOSE`, `ICON_MAXIMIZE`, `ICON_MINIMIZE`, `ICON_EXPAND`
- 装饰图标：`ICON_BIRD`, `ICON_BIRDHOUSE`, `ICON_BONE`
- 功能图标：`ICON_HOME`, `ICON_ROCKET`, `ICON_SETTINGS`

### ThemeManager 主题管理

```qml
import Nandina.Theme

// 切换主题
ThemeManager.setCurrentPaletteType(0)  // Latte (亮色)
ThemeManager.setCurrentPaletteType(1)  // Frappe (柔和暗色)
ThemeManager.setCurrentPaletteType(2)  // Macchiato (暗色)
ThemeManager.setCurrentPaletteType(3)  // Mocha (深暗色)

// 访问主题颜色
Rectangle {
    color: ThemeManager.color.base
    border.color: ThemeManager.color.blue
}

// 监听主题变化
Connections {
    target: ThemeManager
    function onPaletteChanged() {
        // 主题已更改，可以在这里更新 UI
    }
}
```

**Catppuccin 颜色系统：**
- **基础色**：`rosewater`, `flamingo`, `pink`, `mauve`, `red`, `maroon`, `peach`, `yellow`, `green`, `teal`, `sky`, `sapphire`, `blue`, `lavender`
- **文本色**：`text`, `subtext1`, `subtext0`
- **覆盖层**：`overlay2`, `overlay1`, `overlay0`
- **表面色**：`surface2`, `surface1`, `surface0`
- **背景色**：`base`, `mantle`, `crust`

## 快速开始

### 环境要求

- Qt 6.5 或更高版本
- CMake 3.16 或更高版本
- C++23 编译器

### 方式一：作为子项目引入

#### A. 非 git 项目

假设新建的项目名为 `TryNandina`

1. 在您的项目下将本项目 clone 到本地
```bash
cd TryNandina
git clone https://github.com/CvRain/NandinaUI.git
```

2. 跳转到步骤 C

#### B. git 项目

假设项目名为 `TryNandina`

1. 初始化 git 仓库（如果还没有）
```bash
cd TryNandina
git init
```

2. 将项目作为子模块添加
```bash
git submodule add https://github.com/CvRain/NandinaUI.git
```

3. 跳转到步骤 C

#### C. 配置 CMakeLists.txt

1. 添加子项目
```cmake
add_subdirectory(NandinaUI)
```

2. 链接依赖
```cmake
target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Qt6::Core Nandinaplugin
)
```

3. **完整 CMakeLists.txt 示例**

```cmake
cmake_minimum_required(VERSION 3.16)

project(TryNandina VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Core)

qt_standard_project_setup(REQUIRES 6.5)

# 添加 NandinaUI 子项目
add_subdirectory(NandinaUI)

qt_add_executable(appTryNandina
    main.cpp
)

qt_add_qml_module(appTryNandina
    URI TryNandina
    VERSION 1.0
    QML_FILES
        Main.qml
)

set_target_properties(appTryNandina PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTryNandina
    PRIVATE Qt6::Quick Qt6::Core Nandinaplugin
)

include(GNUInstallDirs)
install(TARGETS appTryNandina
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

4. **创建 Main.qml**

```qml
import Nandina.Components
import Nandina.Icon
import Nandina.Theme
import Nandina.Window
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

NandinaWindow {
    width: 800
    height: 600
    visible: true
    title: "NandinaUI Demo"

    Column {
        anchors.centerIn: parent
        spacing: 20

        NanButton {
            text: "Hello NandinaUI!"
            type: "filledPrimary"
            vectorIcon: IconManager.ICON_ROCKET
            iconPosition: NanButton.IconPosition.Left
        }

        NanButton {
            text: "Switch Theme"
            type: "outlinedSecondary"
            onClicked: {
                var nextTheme = (ThemeManager.currentPaletteType + 1) % 4
                ThemeManager.setCurrentPaletteType(nextTheme)
            }
        }
        
        Row {
            spacing: 10
            
            NanIconItem {
                icon: IconManager.ICON_HOME
                width: 32
                height: 32
            }
            
            NanIconItem {
                icon: IconManager.ICON_ROCKET
                colorRole: "red"
                width: 32
                height: 32
            }
            
            NanIconItem {
                icon: IconManager.ICON_BIRD
                colorRole: "blue"
                width: 32
                height: 32
            }
        }
    }
}
```

5. **编译运行**

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/your_compiler
cmake --build build
./build/appTryNandina
```

## 🤝 贡献

欢迎贡献代码、报告问题或提出建议！

1. Fork 本项目
2. 创建您的特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启一个 Pull Request

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 🎨 设计灵感

- [Catppuccin](https://github.com/catppuccin/catppuccin) - 配色方案
- [shadcn/ui](https://ui.shadcn.com/) - 组件设计理念
- [Skeleton UI](https://www.skeleton.dev/) - 布局与交互设计

## 📮 联系方式

如有问题或建议，欢迎：
- 提交 [Issue](https://github.com/CvRain/NandinaUI/issues)
- 发起 [Pull Request](https://github.com/CvRain/NandinaUI/pulls)
- 在 [Discussions](https://github.com/CvRain/NandinaUI/discussions) 中讨论

---

Made with ❤️ by CvRain
