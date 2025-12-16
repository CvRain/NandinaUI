# NandinaUI 展示 Demo 使用说明

## 概述

NandinaUI Demo 展示了组件库的三大核心功能：原始图标、纯按钮和带图标的按钮。通过这个 Demo，你可以直观地看到组件在不同主题下的表现。

## 界面布局

Demo 分为三个主要部分，采用垂直滚动布局：

### 1️⃣ 原始图标展示 (NanIconItem)

展示 NanIconItem 组件的基本用法和主题颜色集成。

#### 使用主题默认颜色
显示所有 7 个内置图标，使用默认的主题文本颜色 (`colorRole: "text"`)：
- Close (关闭)
- Maximize (最大化)
- Minimize (最小化)
- Expand (展开)
- Bird (鸟)
- Birdhouse (鸟屋)
- Bone (骨头)

#### 使用不同颜色角色
展示同一组图标使用不同的 Catppuccin 颜色角色：
- Red (红色)
- Green (绿色)
- Blue (蓝色)
- Lavender (薰衣草)
- Yellow (黄色)
- Pink (粉色)
- Teal (青色)

**观察要点**：切换主题时，这些图标会自动更新为对应主题的相应颜色。

### 2️⃣ 纯按钮展示 (NanButton)

展示 NanButton 的 15 种预设样式。

#### Filled 样式 (8种)
实心填充按钮，背景色与边框色相同：
- `filledPrimary` - 主要按钮 (lavender)
- `filledSecondary` - 次要按钮 (flamingo)
- `filledTertiary` - 第三级按钮 (teal)
- `filledSuccess` - 成功按钮 (green)
- `filledWarning` - 警告按钮 (yellow)
- `filledError` - 错误/危险按钮 (red)
- `filledSurface` - 表面色按钮 (surface1)
- `default` - 默认按钮 (text)

#### Outlined 样式 (7种)
轮廓按钮，透明背景，仅有彩色边框：
- `outlinedPrimary` - 主要轮廓 (lavender)
- `outlinedSecondary` - 次要轮廓 (flamingo)
- `outlinedTertiary` - 第三级轮廓 (teal)
- `outlinedSuccess` - 成功轮廓 (green)
- `outlinedWarning` - 警告轮廓 (yellow)
- `outlinedError` - 错误轮廓 (red)
- `outlinedSurface` - 表面色轮廓 (surface1)

**交互效果**：
- 悬停时：颜色变亮 15%，缩放至 104%
- 按下时：颜色变暗 15%，缩放至 96%
- 点击时：触发"果冻"回弹动画

### 3️⃣ 带图标的按钮 (NanButton with Icons)

展示按钮与图标的组合使用。

#### 左侧图标 (IconPosition.Left)
图标在文本左侧的按钮示例：
- "关闭" + CLOSE 图标 (filledError)
- "确认" + BIRD 图标 (filledSuccess)
- "展开" + EXPAND 图标 (filledPrimary)

#### 右侧图标 (IconPosition.Right)
图标在文本右侧的按钮示例：
- "下一步" + MAXIMIZE 图标 (outlinedPrimary)
- "详情" + BONE 图标 (outlinedSecondary)
- "设置" + BIRDHOUSE 图标 (outlinedTertiary)

#### 仅图标 (IconPosition.IconOnly)
纯图标按钮，无文本标签：
- Filled 风格：4个图标按钮 (50×50)
- Outlined 风格：3个图标按钮 (50×50)

#### 不同尺寸示例
展示相同按钮在不同尺寸下的表现：
- 小：100×35 (字号 10)
- 中等：140×45 (字号 12)
- 大：180×55 (字号 14)

## 主题切换

点击右上角的**主题切换按钮**可以在 4 种 Catppuccin 主题间循环：

1. **Latte** (浅色主题) - 明亮、清新
2. **Frappe** (深色主题) - 柔和、温暖
3. **Macchiato** (深色主题) - 优雅、平衡
4. **Mocha** (深色主题) - 深邃、舒适

按钮显示当前主题名称，如 "主题: Latte"。

## 测试建议

### 主题切换测试
1. 点击主题切换按钮，观察所有组件的颜色过渡
2. 注意颜色动画是否平滑（200ms 过渡）
3. 检查图标颜色是否与主题协调

### 交互测试
1. 悬停在按钮上，观察缩放和颜色变化
2. 点击按钮，观察"果冻"回弹动画
3. 测试不同样式按钮的视觉反馈一致性

### 视觉测试
1. 滚动页面，检查布局是否合理
2. 对比 Filled 和 Outlined 样式的可读性
3. 验证图标与文本的对齐和间距

### 响应式测试
1. 调整窗口大小，观察组件的自适应行为
2. 检查小尺寸窗口下的滚动和布局

## 代码结构

```
Main.qml                    # 主展示页面
├─ 原始图标展示
│  ├─ 默认主题颜色图标
│  └─ 自定义颜色角色图标
├─ 纯按钮展示
│  ├─ Filled 样式按钮
│  └─ Outlined 样式按钮
└─ 带图标按钮展示
   ├─ 左侧图标按钮
   ├─ 右侧图标按钮
   ├─ 仅图标按钮
   └─ 不同尺寸示例
```

## 组件属性快速参考

### NanIconItem
```qml
NanIconItem {
    icon: IconManager.ICON_BIRD    // 图标枚举
    colorRole: "lavender"           // 颜色角色（可选）
    implicitWidth: 40               // 宽度
    implicitHeight: 40              // 高度
}
```

### NanButton
```qml
NanButton {
    text: "按钮文本"                 // 文本
    type: "filledPrimary"           // 样式类型
    vectorIcon: IconManager.ICON_BIRD  // 图标（可选）
    iconPosition: NanButton.IconPosition.Left  // 图标位置
    width: 140                      // 宽度
    height: 45                      // 高度
    autoFitText: false              // 是否自动调整字体
    manualFontSize: 12              // 手动字体大小
}
```

## 常见问题

### Q: 为什么图标不随主题变化？
A: 检查是否设置了 `colorRole: ""`。空字符串会禁用主题颜色，需要手动设置 `color` 属性。

### Q: 按钮文本被截断了？
A: 尝试增加按钮宽度，或启用 `autoFitText: true` 让字体自动缩放。

### Q: 图标在按钮中不显示？
A: 确认 `vectorIcon` 属性设置正确，且 `iconPosition` 不是 `IconOnly` 的情况下需要设置 `text`。

### Q: 如何添加新图标？
A: 在 `icons.json` 中添加 SVG 路径，并在 `IconManager` 中添加对应的枚举值。

## 下一步

- 查看 [ColorSystem_Guide.md](./ColorSystem_Guide.md) 了解配色系统详情
- 查看 [IconSystem.md](./IconSystem.md) 了解图标系统详情
- 查看 [custom_components_guide.md](./custom_components_guide.md) 学习创建自定义组件

## 运行 Demo

```bash
cd /mnt/workspace/Cpp/NandinaUI/TryNandina
cmake --build build
build/appTryNandina
```

Demo 会在窗口中展示，初始窗口大小为 1200×800。可以自由调整窗口大小和滚动查看所有内容。
