# Nandina 开发工作流指南

> 面向贡献者与组件开发者的实践手册

---

## 1. 环境搭建

### 系统要求

| 依赖 | 最低版本 | 推荐方式 |
|------|---------|---------|
| **Qt** | 6.5+ (推荐 6.10) | Qt 在线安装器 |
| **CMake** | 3.16+ | 系统包管理器 / Qt 捆绑 |
| **C++ 编译器** | C++23 支持 | GCC 13+ / Clang 17+ / MSVC 17.8+ |
| **Python** | 3.8+ | 仅代码生成时需要 |
| **Ninja** | 任意 | 推荐（CMake 默认生成器） |

### 首次构建

```bash
# 克隆仓库
git clone <repo-url> NandinaUI && cd NandinaUI

# 配置 + 构建
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug -j$(nproc)

# 运行示例
./build/Debug/example/NandinaExampleApp
```

### IDE 配置

**CLion**: 打开项目根目录即可，CMake Profiles 中设置 `CMAKE_BUILD_TYPE=Debug`。

**VS Code**: 安装 CMake Tools 扩展，配置 `settings.json`：
```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build/Debug",
  "cmake.generator": "Ninja"
}
```

**Qt Creator**: 打开 `CMakeLists.txt`，选择合适的 Qt Kit。

---

## 2. 项目结构导览

```
NandinaUI/
├── CMakeLists.txt              # 顶层构建文件
├── README.md                   # 项目概述与路线图
├── Nandina/                    # 核心库源码
│   ├── CMakeLists.txt          # 库构建配置 + QML 模块注册
│   ├── Core/                   # 基础层：类型定义
│   │   ├── nandina_types.hpp   #   枚举：ThemePreset, ColorVariant, ColorAccent
│   │   ├── Color/              #   颜色系统
│   │   │   ├── color_palette.hpp   # 单色族 24 色值
│   │   │   └── color_schema.hpp    # 7 色族组合
│   │   ├── Primitives/         #   设计图元
│   │   │   ├── typography_schema.hpp
│   │   │   └── primitive_schema.hpp
│   │   └── Tokens/             #   预设令牌（自动生成）
│   │       ├── theme_presets.hpp
│   │       └── theme_presets.cpp
│   ├── Theme/                  # 主题管理器
│   │   ├── theme_manager.hpp
│   │   └── theme_manager.cpp
│   ├── Controls/               # 组件层（待实现）
│   └── Window/                 # 窗口组件（待实现）
├── example/                    # 示例应用
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── Main.qml                # 主界面（调色板 + 主题切换演示）
│   └── CustomTheme.qml         # 自定义主题示例
├── scripts/
│   └── generate_presets.py     # CSS → C++ 代码生成器
├── temp/
│   └── global/                 # Skeleton CSS 主题源文件
├── docs/                       # 文档
├── leyacy-archive/             # 旧版代码归档（仅供参考）
└── Image/                      # 项目图片资源
```

---

## 3. 开发路线图

项目遵循 **先底座、后控件、再复杂场景** 的渐进策略：

| 阶段 | 版本 | 目标 | 状态 |
|------|------|------|------|
| **0 — 设计系统底座** | v0.1.x | 枚举类型、颜色/图元 schema、主题管理器、代码生成 | ✅ 已完成 |
| **1 — 基础表单组件** | v0.2.0 | Button, Input, Label, Switch, Checkbox, Radio, Textarea, Slider | 🚧 下一步 |
| **2 — 基础布局组件** | v0.3.0 | Card, Separator, ScrollArea, Tabs | ⏳ 计划中 |
| **2.5 — 反馈组件** | v0.3.5 | Dialog, Toast, Tooltip, Spinner/Progress | ⏳ 计划中 |
| **3 — 复杂导航** | v0.4.0 | Sidebar, Drawer, Resizable, NavMenu | ⏳ 计划中 |
| **4 — 数据展示** | v0.5.0 | List, Pagination, Table, TreeView, Skeleton | ⏳ 计划中 |
| **5 — 选择与工具** | v0.6.0 | DropdownMenu, Combobox, DatePicker, Avatar, Badge 等 | ⏳ 计划中 |

---

## 4. 常见开发任务

### 4.1 添加新 Skeleton 主题

**步骤：**

1. **获取 CSS 文件**
   将新的 Skeleton 主题 CSS 放入 `temp/global/` 目录（如 `temp/global/theme-newname.css`）。

2. **注册枚举值**
   编辑 `Nandina/Core/nandina_types.hpp`：
   ```cpp
   enum class ThemePreset {
       Catppuccin, Cerberus, Concord, Crimson, Fennec, Legacy,
       NewTheme  // <-- 新增
   };
   ```

   同步更新辅助函数：`themePresetName()`、`themePresetFromName()`、`allThemePresetNames()` 和 `AllThemePresets` 数组。

3. **更新代码生成器映射**
   编辑 `scripts/generate_presets.py`，在 `PRESET_NAMES` 字典中添加：
   ```python
   PRESET_NAMES = {
       # ... 已有映射
       "newname": "NewTheme",
   }
   ```

4. **运行代码生成**
   ```bash
   python3 scripts/generate_presets.py
   ```
   这将重新生成 `Nandina/Core/Tokens/theme_presets.hpp` 和 `theme_presets.cpp`。

5. **构建验证**
   ```bash
   cmake --build build/Debug -j$(nproc)
   ```

6. **在示例中测试**
   运行 `NandinaExampleApp`，新主题应自动出现在主题切换按钮列表中。

### 4.2 开发新 QML 组件（阶段 1 指南）

以实现 `NanButton` 为例：

#### 第一步：理解组件层次

```
Surface.qml (Primitives)
  └── Pressable.qml (Primitives)
       └── NanButton.qml (Controls)
```

组件应尽量复用 `Nandina.Primitives` 中的基础行为件：

| 基础件 | 用途 |
|--------|------|
| `Surface.qml` | 提供背景色、圆角、边框的基础视觉容器 |
| `Pressable.qml` | 封装 pressed/hovered/focused 状态机 |
| `BaseControl.qml` | 通用控件基类，组合 Surface + 状态 |
| `FormFieldBehavior.qml` | 表单字段行为：error/success/helperText |
| `Indicator.qml` | 开关/复选框的指示器动画行为基础 |

> 注意：Primitives 层的 QML 组件目前在 `leyacy-archive/Primitives/` 中，需迁移到 `Nandina/Controls/` 并适配新的主题系统。

#### 第二步：创建组件文件

在 `Nandina/Controls/` 下创建 `NanButton.qml`：

```qml
import QtQuick
import QtQuick.Controls.Basic as Basic
import Nandina.Core
import Nandina.Theme

Basic.Button {
    id: control

    // === 公共 API ===
    property string variant: "default"   // "default" | "primary" | "outline" | "ghost"
    property string size: "md"           // "sm" | "md" | "lg"

    // === 主题绑定 ===
    readonly property var colors: ThemeManager.colors
    readonly property var primitives: ThemeManager.primitives
    readonly property bool isDark: ThemeManager.darkMode

    // === 五态（Normal/Hover/Pressed/Focus/Disabled）===
    background: Rectangle {
        radius: primitives.radiusBase
        color: {
            if (!control.enabled) return resolveDisabledColor()
            if (control.pressed)  return resolvePressedColor()
            if (control.hovered)  return resolveHoverColor()
            return resolveNormalColor()
        }
        border.width: variant === "outline" ? primitives.borderWidth : 0
        border.color: colors.primary.shade500

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    contentItem: Text {
        text: control.text
        font.family: primitives.baseFont.fontFamily
        font.weight: primitives.baseFont.fontWeight
        color: resolveTextColor()
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    // === 内部辅助函数 ===
    function resolveNormalColor() {
        switch (variant) {
        case "primary": return colors.primary.shade500
        case "outline": return "transparent"
        case "ghost":   return "transparent"
        default:        return colors.surface.shade200
        }
    }

    function resolveTextColor() {
        if (variant === "primary")
            return colors.primary.contrast500
        return isDark
            ? primitives.baseFont.fontColorDark
            : primitives.baseFont.fontColor
    }

    // ... 其余状态解析函数
}
```

#### 第三步：注册到 CMake

编辑 `Nandina/CMakeLists.txt`，在 `NandinaControls` 模块的 `QML_FILES` 中添加：

```cmake
qt_add_qml_module(NandinaControls
    URI Nandina.Controls
    QML_FILES
        Controls/NanButton.qml
        # ... 其他控件
)
```

#### 第四步：编写演示页面

在 `example/` 下创建 `ButtonDemoPage.qml`：

```qml
import Nandina.Controls

Column {
    spacing: 16

    NanButton { text: "Default"; variant: "default" }
    NanButton { text: "Primary"; variant: "primary" }
    NanButton { text: "Outline"; variant: "outline" }
    NanButton { text: "Ghost";   variant: "ghost" }
    NanButton { text: "Disabled"; enabled: false }
}
```

#### 第五步：验证清单

- [ ] 五态（normal / hover / pressed / focus / disabled）视觉正确
- [ ] 六套主题下颜色均合理
- [ ] 亮色/暗色模式切换正确
- [ ] Tab 可聚焦，Enter/Space 触发点击
- [ ] 无构建警告

### 4.3 修改颜色/图元 Schema

如果需要给 `PrimitiveSchema` 添加新的设计令牌：

1. **在 Schema 类中添加属性**
   编辑 `Nandina/Core/Primitives/primitive_schema.hpp`，添加 `Q_PROPERTY` 和对应成员。

2. **更新代码生成器**
   修改 `scripts/generate_presets.py`，从 CSS 中解析新的令牌值，并生成对应的 setter 调用。

3. **重新生成预设**
   ```bash
   python3 scripts/generate_presets.py
   ```

4. **构建 + 测试**

### 4.4 自定义主题（QML 侧）

用户可以在 QML 中动态修改主题值，无需修改库源码：

```qml
// CustomTheme.qml
import Nandina.Theme
import Nandina.Core

Component.onCompleted: {
    // 先应用一个基础主题
    ThemeManager.setThemeByName("cerberus")

    // 再覆盖特定颜色
    ThemeManager.colors.primary.shade500 = "#FF6B00"
    ThemeManager.colors.primary.shade600 = "#E05C00"

    // 修改图元
    ThemeManager.primitives.radiusBase = 16
    ThemeManager.primitives.spacing = 24
}
```

---

## 5. 构建系统详解

### CMake 模块结构

```
Nandina (SHARED/STATIC 库)
├── qt_add_qml_module(NandinaCore)          URI: Nandina.Core
├── qt_add_qml_module(NandinaColor)         URI: Nandina.Color
├── qt_add_qml_module(NandinaPrimitives)    URI: Nandina.Primitives
├── qt_add_qml_module(NandinaTokens)        URI: Nandina.Tokens
├── qt_add_qml_module(NandinaTheme)         URI: Nandina.Theme
├── qt_add_qml_module(NandinaControls)      URI: Nandina.Controls
└── qt_add_qml_module(NandinaWindow)        URI: Nandina.Window
```

每个子模块是独立的 QML 模块，通过 `target_link_libraries` 建立依赖。

### 关键 CMake 选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `BUILD_SHARED_LIBS` | `ON` | 构建共享库/静态库 |
| `CMAKE_BUILD_TYPE` | - | Debug / Release |

### AUTOMOC

项目启用了全局 `AUTOMOC`。所有包含 `Q_OBJECT` / `Q_NAMESPACE` / `Q_GADGET` 的头文件会自动被 moc 处理，无需手动添加 moc 步骤。

---

## 6. 代码规范

### 命名约定

| 类别 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `ColorPalette`, `ThemeManager` |
| 枚举 | PascalCase | `ThemePreset::Catppuccin` |
| Q_PROPERTY | camelCase | `currentTheme`, `darkMode` |
| QML 文件 | PascalCase | `NanButton.qml`, `Surface.qml` |
| C++ 头文件 | snake_case | `color_palette.hpp`, `theme_manager.hpp` |
| CSS 令牌 | kebab-case (源) → camelCase (目标) | `--border-width` → `borderWidth` |

### 组件 API 设计约束

1. **语义优先**：属性命名反映功能而非视觉（`variant` 而非 `backgroundColor`）
2. **零配置可用**：`NanButton {}` 应有合理默认外观
3. **一致性**：所有组件共享统一的状态字段名（`enabled`, `variant`, `size`）
4. **可组合**：优先提供小而稳定的基础件，复杂效果通过组合实现

### 文件组织

- 每个 QML 组件一个文件
- C++ 类使用 header-only 风格（schema 类），实现复杂的用 `.hpp` + `.cpp`
- 自动生成的代码用注释标记 `// AUTO-GENERATED`，不要手动修改

---

## 7. 测试策略

### 视觉验证（当前）

目前项目以示例应用（`NandinaExampleApp`）作为主要验证手段：

```bash
cmake --build build/Debug -j$(nproc) && ./build/Debug/example/NandinaExampleApp
```

验证要点：
- 所有 6 套主题切换后颜色正确
- 亮色/暗色模式过渡自然
- 组件在不同主题下可读性良好（对比色正确）

### 构建验证

确保编译零警告：

```bash
cmake --build build/Debug -j$(nproc) 2>&1 | grep -E "warning|error"
```

### 未来测试计划

- **QML 单元测试**：使用 `Qt Quick Test` 框架
- **视觉回归测试**：截图对比（考虑集成 CI）
- **主题一致性测试**：自动验证所有色阶的对比度达到 WCAG AA 标准

---

## 8. Git 工作流（建议）

### 分支策略

```
main                    稳定发布分支
├── develop             开发主分支
│   ├── feature/xxx     功能分支
│   ├── fix/xxx         修复分支
│   └── refactor/xxx    重构分支
└── release/v0.x.x      发布准备分支
```

### Commit 约定

采用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

```
feat(controls): add NanButton component
fix(theme): fix dark mode body background
refactor(color): replace string keys with enum indexing
docs: add architecture design document
chore(scripts): update generate_presets.py for new theme
```

### PR 检查清单

- [ ] 代码编译无警告
- [ ] 示例应用正常运行
- [ ] 新增组件有对应的演示页面
- [ ] 更新了相关文档

---

## 9. 阶段 1 实施建议

当前设计系统底座已完成，下一步是 **阶段 1：基础表单组件**。

### 推荐实施顺序

```
1. NanButton    ← 最基础，验证主题绑定流程
2. NanLabel     ← 纯文本展示，测试排版令牌
3. NanSwitch    ← 验证开关状态机 + Indicator
4. NanCheckbox  ← 复用 Switch 的状态机
5. NanInput     ← 表单行为（error/success/helperText）
```

### 每个组件的交付标准

| 维度 | 要求 |
|------|------|
| **状态** | normal / hover / pressed / focus / disabled 五态 |
| **表单** | 输入型组件需支持 error / success / helperText |
| **主题** | 6 套主题 × 亮暗模式 = 12 种组合均可用 |
| **无障碍** | Tab 聚焦，Enter/Space 触发核心行为 |
| **过渡** | 主题切换无闪烁，动画时长一致 |
| **演示** | 对应的 DemoPage 展示所有变体 |

### 从 legacy-archive 迁移

`leyacy-archive/` 目录保存了旧版组件代码，可作为参考但需要重写：

| 旧版文件 | 迁移目标 | 变化要点 |
|----------|---------|---------|
| `Controls/Button/NanButton.qml` | `Nandina/Controls/NanButton.qml` | 绑定新 ThemeManager，枚举色族索引 |
| `Controls/NanSwitch.qml` | `Nandina/Controls/NanSwitch.qml` | 使用 PrimitiveSchema 令牌替代硬编码 |
| `Controls/NanCheckbox.qml` | `Nandina/Controls/NanCheckbox.qml` | 同上 |
| `Controls/NanLabel.qml` | `Nandina/Controls/NanLabel.qml` | 使用 TypographySchema |
| `Controls/Input/NanInput.qml` | `Nandina/Controls/NanInput.qml` | FormFieldBehavior 融合 |
| `Primitives/*.qml` | `Nandina/Controls/internal/` | 基础行为件，迁移为内部组件 |

迁移要点：
1. 替换旧的 `NanStyle.themeManager.currentPalette` 为 `ThemeManager.colors`
2. 替换硬编码尺寸为 `ThemeManager.primitives.spacing` / `radiusBase` 等令牌
3. 替换旧的 `color0..color17` 索引为语义化的 `primary.shade500` 形式
4. 确保所有颜色引用支持暗色模式切换

---

## 10. 常见问题

### Q: 为什么修改了颜色但 QML 没更新？

确保修改的是 ThemeManager 暴露的 `colors` / `primitives` 对象的属性，而非创建了新对象。设计系统使用 CONSTANT 指针 + 信号冒泡机制，QML 绑定依赖子属性的 `changed()` 信号。

### Q: 代码生成器报错找不到 CSS 文件？

检查 `temp/global/` 目录下是否有格式为 `theme-{name}.css` 的文件。生成器通过文件名匹配主题。

### Q: 如何只修改一个主题的某个颜色？

两种方式：
1. **源头修改**：编辑对应的 CSS 文件 → 重新运行生成器
2. **运行时覆盖**：在 QML 中 `ThemeManager.colors.primary.shade500 = "#新色值"`

### Q: 编译时出现 `namespace alias` 相关错误？

C++ 不允许在类体内使用 `namespace Types = Nandina::Types`。请在类体内使用完全限定名 `Nandina::Types::ThemePreset`，命名空间别名只能在文件作用域或函数作用域中使用。
