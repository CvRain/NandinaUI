# NandinaUI 项目索引

> 生成时间：2026-02-16
> 用途：快速定位模块、组件、主题与示例入口，便于后续迭代修改。

## 1. 构建与入口

- 根构建入口：`CMakeLists.txt`
  - 开启 `AUTOMOC/AUTOUIC/AUTORCC`
  - 选项：`NANDINA_BUILD_SHARED`、`NANDINA_DEV_IMPORT_PATH`
  - 子目录：`Nandina/`、`example/`
- 聚合库：`Nandina/CMakeLists.txt`
  - 子模块：`NandinaColor`、`NandinaControls`、`NandinaCore`、`NandinaTheme`、`NandinaWindow`
  - 顶层 QML 模块：`URI Nandina`
- 示例应用：`example/CMakeLists.txt`
  - 可执行目标：`NandinaExampleApp`
  - QML 模块：`URI NandinaExample`
- 运行入口：`example/main.cpp`
  - `engine.loadFromModule("NandinaExample", "Main")`

## 2. QML 模块总览

### Nandina.Window
- 模块定义：`Nandina/Window/CMakeLists.txt`
- 组件文件：
  - `Nandina/Window/NanWindow.qml`
  - `Nandina/Window/DefaultTitleBar.qml`
  - `Nandina/Window/TitleBarButton.qml`
  - `Nandina/Window/WindowResizer.qml`
  - `Nandina/Window/ResizeHandler.qml`
  - `Nandina/Window/CornerRectangle.qml`

### Nandina.Controls
- 模块定义：`Nandina/Controls/CMakeLists.txt`
- 组件文件：
  - `Nandina/Controls/Button/NanButton.qml`
  - `Nandina/Controls/Input/NanInput.qml`
  - `Nandina/Controls/NanLabel.qml`
  - `Nandina/Controls/NanSwitch.qml`
  - `Nandina/Controls/NanCheckbox.qml`
  - `Nandina/Controls/SideBar/NanSideBar.qml`
  - `Nandina/Controls/SideBar/NanSideBarTrigger.qml`
  - `Nandina/Controls/SideBar/NanSideBarItem.qml`
  - `Nandina/Controls/SideBar/NanSideBarGroup.qml`
- JS 工具：
  - `Nandina/Controls/theme_utils.js`
  - `Nandina/Controls/Button/button_style_utils.js`
  - `Nandina/Controls/Input/input_validation_utils.js`

### Nandina.Theme
- 模块定义：`Nandina/Theme/CMakeLists.txt`
- C++ 源：
  - `Nandina/Theme/theme_manager.hpp`
  - `Nandina/Theme/theme_manager.cpp`
- 关键类：`ThemeManager`

### Nandina.Color
- 模块定义：`Nandina/Color/CMakeLists.txt`
- C++ 源：
  - `Nandina/Color/color_schema.hpp`
  - `Nandina/Color/color_atla.hpp`
  - `Nandina/Color/color_atla.cpp`
- 关键类：`PaletteEnum`、`ColorCollection`、`PaletteCollection`、`NanColorAtla`

### Nandina.Core
- 模块定义：`Nandina/Core/CMakeLists.txt`
- 当前源：`Nandina/Core/nan_singleton.hpp`

## 3. 关键符号索引（后续改动高频）

### ThemeManager（主题切换主入口）
- 文件：`Nandina/Theme/theme_manager.hpp`
- 关键属性：
  - `currentPaletteType`
  - `currentColorCollection`
  - `currentPaletteCollection`
  - `customColorCollection`
  - `customPaletteCollection`
- 关键信号：`paletteTypeChanged`、`customThemeChanged`
- 关键实现：`Nandina/Theme/theme_manager.cpp`
  - `setCurrentPaletteType(...)`
  - `setCustomColorCollection(...)`
  - `setCustomPaletteCollection(...)`
  - `updateCurrentCollections()`

### 颜色与语义色映射
- 枚举与容器定义：`Nandina/Color/color_schema.hpp`
  - `PaletteType`: `Latte/Frappe/Macchiato/Mocha/Custom`
- 配色生成与映射：`Nandina/Color/color_atla.cpp`
  - `generate*Color(...)`
  - `generatePaletteCollection(...)`
  - `brightColor(...)`

### NanWindow（窗口系统核心）
- 文件：`Nandina/Window/NanWindow.qml`
- 关键能力：
  - 标题栏模式：`DefaultTitleBar/Frameless/CustomTitleBar`
  - 主题渐变背景与过渡
  - 自定义标题栏 + 系统控件注入
  - `WindowResizer` 调整尺寸

### NanSideBar（复杂导航组件）
- 文件：`Nandina/Controls/SideBar/NanSideBar.qml`
- 关键能力：
  - Side：`Left/Right`
  - Collapsible：`Offcanvas/Icon/None`
  - Header/Content/Footer 插槽
  - `toggle()/expand()/collapse()`

## 4. 示例与演示页索引

- 示例主页面：`example/Main.qml`
- 自定义主题样例：`example/CustomTheme.qml`
- 演示页：
  - `example/WindowDemoPage.qml`
  - `example/ButtonDemoPage.qml`
  - `example/InputDemoPage.qml`
  - `example/LabelDemoPage.qml`
  - `example/SwitchDemoPage.qml`
  - `example/CheckboxDemoPage.qml`

## 5. 规范文档

- 项目说明：`README.md`
- 组件规范（v0.2）：`docs/v0.2-component-spec.md`

## 6. 建议的后续改动入口（按需求类型）

- **主题系统调整**：从 `Nandina/Theme/theme_manager.*` + `Nandina/Color/color_atla.cpp` 入手
- **窗口行为/样式调整**：从 `Nandina/Window/NanWindow.qml` 入手
- **基础控件 API 或视觉调整**：从 `Nandina/Controls/*` 对应组件入手
- **示例交互验证**：从 `example/Main.qml` 与各 `*DemoPage.qml` 入手
