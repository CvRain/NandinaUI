# Nandina Icon System 使用指南

Nandina UI 的图标系统经过重构，现在基于 SVG 路径数据进行渲染，支持通过 JSON 配置文件管理图标，并与主题系统深度集成。

## 核心组件

*   **NanIconItem**: QML 组件，用于显示图标。
*   **IconManager**: 单例类，负责加载和管理 `icons.json` 中的图标数据。
*   **icons.json**: 存储 SVG 路径数据的配置文件。

## 快速开始

### 1. 通过名称使用图标 (推荐)

使用 `iconName` 属性指定 `icons.json` 中定义的键名即可显示图标。

```qml
import Nandina.Icon 1.0

NanIconItem {
    width: 24
    height: 24
    iconName: "close" // 对应 icons.json 中的 key
    color: "red"
}
```

### 2. 使用主题颜色

使用 `colorRole` 属性可以将图标颜色绑定到当前主题。当主题切换时，图标颜色会自动更新。

```qml
NanIconItem {
    iconName: "home"
    colorRole: "primary" // 使用主题中的 primary 颜色
    // colorRole: "text" // 或者使用 text 颜色
}
```

### 3. 使用预定义枚举 (旧版兼容)

仍然支持使用 `IconManager` 枚举，但建议迁移到字符串名称方式。

```qml
NanIconItem {
    icon: IconManager.ICON_CLOSE
}
```

### 4. 使用自定义 SVG 路径

直接通过 `pathData` 属性传入 SVG 路径字符串。

```qml
NanIconItem {
    pathData: "M12 2L2 22h20L12 2z" 
    color: "blue"
}
```

## 属性详解

| 属性名 | 类型 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `iconName` | `string` | `""` | 图标名称，对应 `icons.json` 中的键。 |
| `colorRole` | `string` | `""` | 主题颜色角色（如 "primary", "blue", "text"）。设置后会自动更新 `color`。 |
| `icon` | `enum` | `IconManager.ICON_NONE` | (旧版) 预定义图标枚举。 |
| `pathData` | `string` | `""` | SVG 路径数据。 |
| `color` | `color` | `Qt.black` | 线条颜色。如果设置了 `colorRole`，此属性会被覆盖。 |
| `lineWidth` | `real` | `1.0` | 线条粗细。 |
| `fillColor` | `color` | `Qt.transparent` | 填充颜色。 |

## 如何添加新图标

现在添加新图标无需修改 C++ 代码，只需编辑配置文件。

### 步骤 1: 编辑配置文件

打开 `NandinaUI/Nandina/Icon/icons.json` 文件。

### 步骤 2: 添加路径数据

在 JSON 对象中添加新的键值对。键为图标名称，值为 SVG 的 `d` 属性内容。

```json
{
    "existing_icon": "...",
    "my_new_icon": "M10 10 H 90 V 90 H 10 Z"
}
```

### 步骤 3: 重新编译资源

由于 JSON 文件包含在 Qt 资源系统 (`.qrc`) 中，修改后需要重新编译项目以更新资源文件。

```bash
cmake --build build
```

### 步骤 4: 在 QML 中使用

```qml
NanIconItem {
    iconName: "my_new_icon"
}
```

## 主题集成

`NanIconItem` 通过 `colorRole` 属性与 `NandinaTheme` 模块集成。支持的颜色角色包括 Catppuccin 主题的所有标准颜色，例如：
- `rosewater`, `flamingo`, `pink`, `mauve`, `red`, `maroon`, `peach`, `yellow`, `green`, `teal`, `sky`, `sapphire`, `blue`, `lavender`
- `text`, `subtext1`, `subtext0`
- `overlay2`, `overlay1`, `overlay0`
- `surface2`, `surface1`, `surface0`
- `base`, `mantle`, `crust`

## 最佳实践

1.  **图标尺寸**: 建议 SVG 路径基于 24x24 的视口（viewBox）设计。
2.  **优先使用 iconName**: 相比于枚举，字符串名称更灵活，且无需修改 C++ 头文件。
3.  **使用 colorRole**: 尽量使用 `colorRole` 而不是硬编码颜色值，以确保应用支持深色/浅色模式切换。
