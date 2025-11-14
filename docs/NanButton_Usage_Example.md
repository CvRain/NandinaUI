# NanButton 使用说明

## 功能特性

### 1. 自动字体调整
- **autoFitText**: 控制是否自动调整字体大小以适应按钮尺寸
  - `true`: 自动调整字体大小(默认)
  - `false`: 使用手动设置的字体大小
- **manualFontSize**: 当 `autoFitText` 为 `false` 时使用的字体大小
- **minimumFontSize**: 自动调整时的最小字体大小(默认: 8)
- **maximumFontSize**: 自动调整时的最大字体大小(默认: 72)

### 2. 图标支持
- **iconSource**: 图标文件路径(支持 SVG, PNG 等格式)
- **iconPosition**: 图标位置
  - `NanButton.IconPosition.Left`: 图标在文本左侧
  - `NanButton.IconPosition.Right`: 图标在文本右侧
  - `NanButton.IconPosition.IconOnly`: 仅显示图标,不显示文本
- **iconSize**: 图标大小(默认: 24)
- **iconSpacing**: 图标与文本之间的间距(默认: 8)

### 3. 动画效果
- 悬停放大效果
- 点击果冻回弹效果
- 平滑颜色过渡

## 使用示例

### 1. 基础按钮(自动字体)
```qml
NanButton {
    text: "点击我"
    width: 120
    height: 60
    autoFitText: true  // 字体自动适应按钮大小
}
```

### 2. 手动字体大小
```qml
NanButton {
    text: "固定字体"
    width: 120
    height: 60
    autoFitText: false  // 关闭自动调整
    manualFontSize: 16  // 使用固定的 16pt 字体
}
```

### 3. 带左侧图标的按钮
```qml
NanButton {
    text: "保存"
    width: 140
    height: 50
    iconSource: "qrc:/icons/save.svg"
    iconPosition: NanButton.IconPosition.Left
    iconSize: 20
    iconSpacing: 8
}
```

### 4. 带右侧图标的按钮
```qml
NanButton {
    text: "下一步"
    width: 140
    height: 50
    iconSource: "qrc:/icons/arrow-right.svg"
    iconPosition: NanButton.IconPosition.Right
    iconSize: 20
}
```

### 5. 仅图标按钮
```qml
NanButton {
    width: 50
    height: 50
    iconSource: "qrc:/icons/settings.svg"
    iconPosition: NanButton.IconPosition.IconOnly
    iconSize: 28
}
```

### 6. 自定义样式
```qml
NanButton {
    text: "主要操作"
    type: "filledPrimary"  // 按钮样式类型
    width: 150
    height: 55
    
    // 自定义缩放参数
    hoverScale: 1.05
    pressScale: 0.95
    
    // 自定义颜色调整
    hoverBrightness: 1.2
    pressBrightness: 0.8
}
```

### 7. 组合使用示例
```qml
Row {
    spacing: 16
    
    // 带左图标的大按钮(自动字体)
    NanButton {
        text: "上传文件"
        width: 180
        height: 60
        iconSource: "qrc:/icons/upload.svg"
        iconPosition: NanButton.IconPosition.Left
        autoFitText: true
    }
    
    // 固定字体的按钮
    NanButton {
        text: "取消"
        width: 100
        height: 60
        autoFitText: false
        manualFontSize: 14
    }
    
    // 仅图标按钮
    NanButton {
        width: 60
        height: 60
        iconSource: "qrc:/icons/delete.svg"
        iconPosition: NanButton.IconPosition.IconOnly
        iconSize: 24
    }
}
```

## 属性参考表

| 属性名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `autoFitText` | bool | true | 是否自动调整字体大小 |
| `manualFontSize` | real | 18 | 手动字体大小 |
| `minimumFontSize` | real | 8 | 最小字体大小 |
| `maximumFontSize` | real | 72 | 最大字体大小 |
| `iconSource` | url | "" | 图标路径 |
| `iconPosition` | enum | Left | 图标位置 |
| `iconSize` | real | 24 | 图标大小 |
| `iconSpacing` | int | 8 | 图标与文本间距 |
| `hoverScale` | real | 1.04 | 悬停时缩放比例 |
| `pressScale` | real | 0.96 | 按下时缩放比例 |
| `hoverBrightness` | real | 1.15 | 悬停时亮度因子 |
| `pressBrightness` | real | 0.85 | 按下时亮度因子 |

## 代码优化说明

### 已实现的优化:
1. **属性分组**: 将相关属性按功能分组(字体、图标、动画等)
2. **代码复用**: 提取了 `getInteractiveColor()` 函数,避免重复代码
3. **清晰的枚举**: 使用 `IconPosition` 枚举提高可读性
4. **智能布局**: 使用 `Row` 布局自动处理图标和文本的排列
5. **性能优化**: 字体大小计算时考虑图标占用的空间
6. **注释优化**: 简化注释,去除冗余说明

### 建议的进一步优化:
1. 可以考虑将动画效果单独提取为一个可复用的组件
2. 如果图标样式需求增加,可以创建独立的 `NanIcon` 组件
3. 可以添加加载状态(loading spinner)功能
4. 可以添加禁用状态的特殊视觉效果

## 迁移指南

如果你之前使用的是旧版本的 NanButton,以下是主要变化:

### 新增属性:
- `manualFontSize`: 替代原来硬编码的 18
- `iconSource`, `iconPosition`, `iconSize`, `iconSpacing`: 全新的图标功能

### 保持兼容:
- `autoFitText` 默认为 `true`,保持原有自动调整行为
- 如果不设置 `iconSource`,按钮行为与原来完全一致
