# NandinaUI 配色系统使用指南

## 概述

NandinaUI 基于 **Catppuccin** 配色方案，提供了统一的主题管理系统。所有组件都可以通过 `colorRole` 属性与主题系统对接，实现自动主题切换和颜色一致性。

## 主题系统

### 支持的主题

NandinaUI 支持 Catppuccin 的 4 种主题：

- **Latte** (浅色主题)
- **Frappe** (深色主题)
- **Macchiato** (深色主题)
- **Mocha** (深色主题)

### 切换主题

```qml
import Nandina.Theme

Button {
    text: "切换主题"
    onClicked: {
        // 0: Latte, 1: Frappe, 2: Macchiato, 3: Mocha
        ThemeManager.setCurrentPaletteType(2)
    }
}
```

## 颜色角色 (Color Roles)

Catppuccin 提供了 26 种语义化颜色角色：

### 强调色
- `rosewater`, `flamingo`, `pink`, `mauve`
- `red`, `maroon`, `peach`, `yellow`
- `green`, `teal`, `sky`, `sapphire`
- `blue`, `lavender`

### 文本色
- `text` - 主要文本颜色
- `subtext1`, `subtext0` - 次要文本

### 覆盖层
- `overlay2`, `overlay1`, `overlay0`

### 表面色
- `surface2`, `surface1`, `surface0`

### 背景色
- `base` - 主要背景
- `mantle` - 次要背景
- `crust` - 边框/分隔线

## NanIconItem 配色使用

### 使用默认主题颜色

默认情况下，`NanIconItem` 使用主题的 `text` 颜色：

```qml
import Nandina.Icon

NanIconItem {
    icon: IconManager.ICON_CLOSE
    // colorRole 默认为 "text"，会自动使用主题文本颜色
}
```

### 使用指定的颜色角色

```qml
NanIconItem {
    icon: IconManager.ICON_BIRD
    colorRole: "lavender"  // 使用主题的 lavender 颜色
}

NanIconItem {
    iconName: "bird"
    colorRole: "red"  // 使用主题的 red 颜色
}
```

### 手动设置颜色（不使用主题）

如果需要手动控制颜色，将 `colorRole` 设置为空字符串：

```qml
NanIconItem {
    icon: IconManager.ICON_MAXIMIZE
    colorRole: ""  // 禁用主题颜色
    color: "#ff0000"  // 手动设置为红色
}

NanIconItem {
    iconName: "close"
    colorRole: ""
    color: "white"  // 手动设置为白色
}
```

### 主题切换响应

当主题切换时，所有使用 `colorRole` 的图标会自动更新颜色：

```qml
ColumnLayout {
    NanIconItem {
        icon: IconManager.ICON_BIRD
        colorRole: "lavender"  // 主题切换时自动更新
    }
    
    NanIconItem {
        icon: IconManager.ICON_CLOSE
        colorRole: "red"  // 主题切换时自动更新
    }
}
```

## NanButton 配色使用

NanButton 内部使用的图标会根据按钮样式自动调整颜色：

```qml
import Nandina.Components

NanButton {
    text: "关闭"
    type: "filledError"  // 使用错误样式（红色背景）
    vectorIcon: IconManager.ICON_CLOSE
    iconPosition: NanButton.IconPosition.Left
    // 图标颜色会自动匹配按钮的 foreground 颜色
}
```

按钮支持的样式类型包括：
- **Filled**: `filledPrimary`, `filledSecondary`, `filledTertiary`, `filledSuccess`, `filledWarning`, `filledError`, `filledSurface`
- **Outlined**: `outlinedPrimary`, `outlinedSecondary`, `outlinedTertiary`, `outlinedSuccess`, `outlinedWarning`, `outlinedError`, `outlinedSurface`

## 自定义组件配色

如果你在开发自定义组件，建议遵循以下最佳实践：

### 1. 为组件定义配色方案

在组件的样式 JSON 文件中定义颜色引用：

```json
{
  "target": "MyComponent",
  "styles": [
    {
      "type": "default",
      "background": "@base",
      "foreground": "@text",
      "border": "@overlay0"
    }
  ]
}
```

### 2. 监听主题变化

在组件中连接主题变化信号：

```qml
import Nandina.Theme

Item {
    property color currentColor: ThemeManager.color.text
    
    Connections {
        target: ThemeManager
        function onPaletteChanged() {
            currentColor = ThemeManager.color.text
        }
    }
}
```

### 3. 使用颜色动画

为颜色变化添加平滑过渡：

```qml
Rectangle {
    color: currentColor
    
    Behavior on color {
        ColorAnimation {
            duration: 200
            easing.type: Easing.OutCubic
        }
    }
}
```

## 最佳实践

### ✅ 推荐做法

1. **优先使用 colorRole**：让组件自动适配主题
2. **语义化颜色选择**：选择符合用途的颜色角色（如错误用 `red`，成功用 `green`）
3. **添加颜色动画**：主题切换时提供平滑过渡
4. **保持一致性**：同类组件使用相同的配色方案

### ❌ 避免做法

1. **硬编码颜色**：避免直接使用 `"#FF0000"` 等固定颜色
2. **混用配色系统**：不要在同一组件中混用主题颜色和手动颜色
3. **忘记空 colorRole**：手动设置颜色时记得设置 `colorRole: ""`

## 示例：完整的主题响应式图标展示

```qml
import Nandina.Icon
import Nandina.Theme
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 600
    height: 400
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        
        // 主题切换按钮
        Button {
            text: "切换主题"
            onClicked: {
                let current = ThemeManager.getCurrentPaletteType()
                ThemeManager.setCurrentPaletteType((current + 1) % 4)
            }
        }
        
        // 图标展示
        GridLayout {
            columns: 4
            rowSpacing: 20
            columnSpacing: 20
            
            // 默认文本色
            NanIconItem {
                icon: IconManager.ICON_CLOSE
                implicitWidth: 32
                implicitHeight: 32
            }
            
            // 强调色
            NanIconItem {
                icon: IconManager.ICON_BIRD
                colorRole: "lavender"
                implicitWidth: 32
                implicitHeight: 32
            }
            
            // 成功色
            NanIconItem {
                icon: IconManager.ICON_BIRDHOUSE
                colorRole: "green"
                implicitWidth: 32
                implicitHeight: 32
            }
            
            // 错误色
            NanIconItem {
                icon: IconManager.ICON_BONE
                colorRole: "red"
                implicitWidth: 32
                implicitHeight: 32
            }
        }
    }
}
```

## 问题排查

### 图标颜色不随主题切换

**原因**：可能设置了 `colorRole: ""`  
**解决**：删除 colorRole 设置，使用默认值，或设置为合适的颜色角色

### 手动设置的颜色不生效

**原因**：colorRole 正在覆盖手动设置的颜色  
**解决**：设置 `colorRole: ""` 以禁用主题颜色

### 颜色切换不平滑

**原因**：缺少颜色动画  
**解决**：添加 `Behavior on color { ColorAnimation { ... } }`

## 相关文档

- [Icon System Guide](./IconSystem.md)
- [Custom Components Guide](./custom_components_guide.md)
- [Catppuccin 官方文档](https://github.com/catppuccin/catppuccin)
