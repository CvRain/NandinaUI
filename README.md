# NandinaUI

![NandinaUI](./Image/NandinaUI_icon.jpg)

NandinaUI 是一个正在持续演进中的 Qt 6 / QML 组件库，目标不是简单复刻 Qt Quick Controls，而是提供一套更偏设计系统思路的 UI 基础层：统一主题、统一语义枚举、统一交互原语，再在这之上构建可组合的业务组件。

当前仓库已经和早期 README 描述差异很大。现在的重点不再是概念性蓝图，而是一套已经可以运行、可以集成、并且有完整示例应用可直接查看的模块化 QML 组件体系。

## 当前状态

- 已提供可运行的示例应用，覆盖主题、色板、基础交互原语、卡片、按钮、侧边栏、徽章等页面。
- 已具备统一的主题管理器 `ThemeManager`，支持内置主题和亮暗色切换。
- 已具备共享枚举系统 `ThemeVariant`，用于颜色变体、色阶、预设风格、尺寸等跨组件语义。
- 已形成一批可复用基础组件，包含 `NanSurface`、`NanPressable`、`NanButton`、`NanBadge`、`NanCard`、`NanPanel`、`NanSideBar`、`NanWindow`。
- API 仍在迭代中，适合试用、参与设计和继续扩展；暂不建议把当前版本当作长期冻结的稳定 UI SDK。

## 项目特点

- 不是单纯堆组件，而是先建立主题、语义色、尺寸和交互原语，再向上组合。
- 组件 API 尽量使用语义字段，而不是把视觉细节直接暴露成主接口。
- 主题层和控件层是分离的，亮暗色切换、色族映射、基础 token 都通过共享 schema 驱动。
- 示例应用不是单页 Demo，而是完整的多页面导航结构，可直接用于开发期验证。
- 当前开发方向明显受 shadcn/ui、Skeleton 这类现代设计系统影响，但落地形式是原生 Qt/QML。

## 当前模块

### 对外 QML 模块

- `Nandina.Controls`
    当前包含：`NanPressable`、`NanSurface`、`NanButton`、`NanBadge`、`NanPanel`、`NanCard`、`NanSideBar`、`NanSideBarGroup`、`NanSideBarItem`、`NanSideBarTrigger`
- `Nandina.Theme`
    当前包含：`ThemeManager` 单例，负责主题名、亮暗色状态、颜色 schema、primitive schema
- `Nandina.Window`
    当前包含：`NanWindow`、`NanTitleBar`、`NanTitleBarButton`、`NanWindowResizer`、`NanResizeEdge`

### 内部基础模块

- `Nandina.Color`
    颜色 schema 与主题色板工厂
- `Nandina.Primitives`
    基础视觉 token 和 primitive schema
- `Nandina.Types`
    共享枚举与名称映射，例如 `ThemeTypes`、`ColorVariantTypes`、`PresetTypes`、`SizeTypes`
- `Nandina.Tokens`
    当前作为基础 token 层的一部分参与主题装配
- `Nandina.Fonts`
    内置字体加载支持

## 内置主题

当前仓库内置以下主题名称，均通过 `ThemeManager` 暴露给 QML：

- `Aurora`
- `Catppuccin`
- `Cerberus`
- `Concord`
- `Crimson`
- `Fennec`
- `Legacy`

主题切换支持按名称调用，也支持亮暗色模式切换。示例应用顶部工具条已经直接接入这一能力。

## 目录结构

```text
NandinaUI/
├── Nandina/
│   ├── Controls/      # 控件与容器
│   ├── Core/          # Color / Fonts / Primitives / Tokens / Types
│   ├── Theme/         # ThemeManager 与主题应用逻辑
│   └── Window/        # 自定义窗口与标题栏
├── example/           # 示例应用与页面
├── public/fonts/      # 随仓库提供的字体资源
└── CMakeLists.txt
```

## 环境要求

- Qt 6
- CMake >= 3.16
- C++26

项目当前 CMake 配置中使用了 `CMAKE_CXX_STANDARD 26`，因此需要编译器具备对应支持。

## 运行示例应用

```bash
cmake -S . -B build
cmake --build build
./build/example/NandinaExampleApp
```

如果你在 VS Code 中使用 CMake Tools，也可以直接配置并运行 `NandinaExampleApp` 目标。

## 作为子项目接入

在你的工程中添加：

```cmake
add_subdirectory(NandinaUI)

target_link_libraries(YourApp PRIVATE
        Qt6::Quick
        Nandina
)
```

接入后，你的 QML 中可以直接按模块导入：

```qml
import QtQuick
import Nandina.Theme
import Nandina.Controls
import Nandina.Window
```

## 一个符合当前实现的最小示例

```qml
import QtQuick
import Nandina.Theme
import Nandina.Controls
import Nandina.Window

NanWindow {
        width: 960
        height: 640
        windowTitle: "NandinaUI Demo"
        titleBarMode: "frameless"

        Column {
                anchors.centerIn: parent
                spacing: 12

                NanButton {
                        text: ThemeManager.darkMode ? "切换到亮色" : "切换到暗色"
                        onClicked: ThemeManager.darkMode = !ThemeManager.darkMode
                }

                NanCard {
                        width: 320
                        title: "Hello Nandina"
                        description: "NanCard / NanButton / ThemeManager 已经可以一起工作"

                        Text {
                                text: "Current theme: " + ThemeManager.currentThemeName
                                color: ThemeManager.colors.surface.shade600
                                wrapMode: Text.WordWrap
                                width: parent.width
                        }
                }
        }
}
```

## 示例应用包含的页面

当前示例应用入口位于 `example/Main.qml`，已经组织成一个带侧边栏的多页面演示程序，包含以下页面：

- Theme
- Color Palettes
- Primitives
- Cards
- Button
- SideBar
- Badge

这意味着仓库当前最值得参考的文档，其实不是旧 README，而是示例应用本身。

## 设计思路

### 1. 先统一原语，再做组件

`NanPressable` 负责交互状态，`NanSurface` 负责主题感知表面，按钮、卡片、面板等都建立在这些基础原语之上，而不是各写一套状态机。

### 2. 先统一语义，再决定视觉

项目已经统一了跨组件共享的语义枚举：

- `ColorVariantTypes`
- `ColorAccentTypes`
- `PresetTypes`
- `SizeTypes`

这套设计的意义是：按钮、徽章、卡片、容器不再各自发明一套字符串 token，而是尽量共享同一组主题语言。

### 3. 主题是运行时系统，不是静态配色表

`ThemeManager` 不只负责“当前主题名”，还负责把主题真正应用到颜色 schema 与 primitive schema 上。亮暗色切换、组件颜色绑定、示例页刷新，都是围绕这一点构建的。

## 当前已经落地的组件能力

### NanPressable

- 纯交互原语
- hover / pressed / click / long press
- 可作为其他视觉组件的输入层

### NanSurface

- 主题感知容器
- 支持语义色族和 shade 层级
- 是大量容器类组件的视觉底座

### NanButton

- 预设：`Filled` / `Tonal` / `Outlined` / `Ghost` / `Link`
- 尺寸：`Sm` / `Md` / `Lg`
- 支持左右图标、键盘触发、焦点环、按压回弹

### NanBadge

- 预设：`Filled` / `Tonal` / `Outlined` / `Ghost`
- 支持 dot 模式、图标、尺寸变体

### NanCard

- 支持图片、标题、描述、内容区、页脚区
- 支持交互态和预设变体

### NanSideBar

- 支持 `icon` / `offcanvas` / `none` 三种折叠策略
- 包含 group / item / trigger 组合能力
- 已用于示例应用主导航

### NanWindow

- 自定义窗口容器
- 自定义标题栏与窗口边缘调整

## 当前项目更接近什么阶段

如果要客观描述现状，这个项目现在更接近：

- 一个已经有明确结构和审美方向的 QML 设计系统雏形
- 一套正在形成统一 API 的基础组件库
- 一个适合继续扩展输入类、反馈类、数据展示类组件的地基

而不是一个“所有常见组件都已经完备”的成熟 UI 框架。

## 开发路线图

详细路线图见 [docs/component-roadmap.md](docs/component-roadmap.md)。

这份文档明确了三件事：

- 组件命名统一采用 `NanButton`、`NanInput`、`NanLabel` 这类风格，而不是直接使用无前缀名称
- 组件语义对齐 shadcn/ui，但实现坚持原生 Qt Quick 基础类型
- 组件开发顺序以“先原语、再输入、再浮层、最后复杂展示件”为主线

## 组件开发 Todo

### 已完成

- [x] `NanPressable`
- [x] `NanSurface`
- [x] `NanButton`
- [x] `NanBadge`
- [x] `NanPanel`
- [x] `NanCard`
- [x] `NanSideBar`
- [x] `NanSideBarGroup`
- [x] `NanSideBarItem`
- [x] `NanSideBarTrigger`
- [x] `NanWindow`

### 下一阶段

- [ ] `NanLabel`
- [ ] `NanInput`
- [ ] `NanTextarea`
- [ ] `NanCheckbox`
- [ ] `NanSwitch`
- [ ] `NanRadioGroup`
- [ ] `NanSeparator`

### 后续阶段

- [ ] `NanScrollArea`
- [ ] `NanSkeleton`
- [ ] `NanAvatar`
- [ ] `NanBreadcrumb`
- [ ] `NanCollapsible`
- [ ] `NanDialog`
- [ ] `NanAlertDialog`
- [ ] `NanPopover`
- [ ] `NanTooltip`
- [ ] `NanSheet`
- [ ] `NanDropdownMenu`
- [ ] `NanContextMenu`
- [ ] `NanSelect`
- [ ] `NanCombobox`
- [ ] `NanCalendar`
- [ ] `NanDatePicker`
- [ ] `NanForm`
- [ ] `NanAccordion`
- [ ] `NanTabs`
- [ ] `NanPagination`
- [ ] `NanTable`
- [ ] `NanEmpty`
- [ ] `NanCarousel`
- [ ] `NanChart`

## 贡献

欢迎提 Issue 或 PR，尤其是以下方向：

- 组件 API 一致性
- QML / C++ 边界设计
- 主题系统与 token 结构
- 交互细节、键盘可用性、亮暗色回归问题

## License

See [LICENSE](LICENSE).


