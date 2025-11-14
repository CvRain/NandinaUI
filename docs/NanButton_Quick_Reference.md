# NanButton 快速参考

## 核心改进 ✨

### 1. 字体控制
```qml
// 自动适应(默认)
NanButton {
    text: "自动"
    autoFitText: true  // 默认值
}

// 手动控制
NanButton {
    text: "手动"
    autoFitText: false
    manualFontSize: 16
}
```

### 2. 图标支持
```qml
// 左侧图标
NanButton {
    text: "保存"
    iconSource: "qrc:/icons/save.svg"
    iconPosition: NanButton.IconPosition.Left
    iconSize: 20
}

// 右侧图标
NanButton {
    text: "下一步"
    iconSource: "qrc:/icons/next.svg"
    iconPosition: NanButton.IconPosition.Right
}

// 仅图标
NanButton {
    iconSource: "qrc:/icons/settings.svg"
    iconPosition: NanButton.IconPosition.IconOnly
    iconSize: 24
}
```

## API 速查表

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `autoFitText` | bool | true | 自动调整字体 |
| `manualFontSize` | real | 18 | 手动字体大小 |
| `iconSource` | url | "" | 图标路径 |
| `iconPosition` | enum | Left | Left/Right/IconOnly |
| `iconSize` | real | 24 | 图标大小 |
| `iconSpacing` | int | 8 | 图标文本间距 |

## 常用场景

### 工具栏按钮
```qml
Row {
    spacing: 4
    NanButton {
        width: 40; height: 40
        iconSource: "qrc:/icons/save.svg"
        iconPosition: NanButton.IconPosition.IconOnly
    }
    NanButton {
        width: 40; height: 40
        iconSource: "qrc:/icons/open.svg"
        iconPosition: NanButton.IconPosition.IconOnly
    }
}
```

### 主操作按钮
```qml
NanButton {
    text: "上传文件"
    width: 180
    height: 60
    iconSource: "qrc:/icons/upload.svg"
    iconPosition: NanButton.IconPosition.Left
    autoFitText: true
}
```

### 导航按钮
```qml
Row {
    spacing: 16
    NanButton {
        text: "上一步"
        iconSource: "qrc:/icons/back.svg"
        iconPosition: NanButton.IconPosition.Left
    }
    NanButton {
        text: "下一步"
        iconSource: "qrc:/icons/next.svg"
        iconPosition: NanButton.IconPosition.Right
    }
}
```

## 代码优化亮点 🎯

1. **提取函数** - `getInteractiveColor()` 避免重复代码
2. **类型安全** - 使用 `enum IconPosition` 替代字符串
3. **智能计算** - 字体大小考虑图标空间
4. **清晰结构** - Row 布局自动处理排列
5. **向后兼容** - 所有新属性都有合理默认值

## 依赖
- `Qt5Compat.GraphicalEffects` - 用于图标颜色覆盖
- 如果不可用,移除 `ColorOverlay` 即可

## 下一步
查看完整文档:
- `NanButton_Usage_Example.md` - 详细使用指南
- `NanButton_Optimization_Report.md` - 优化报告
- `NanButton_Demo.qml` - 可视化演示
