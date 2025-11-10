# NanButton 动画效果指南

## 🎨 已实现的动画效果

### 1. **主题切换颜色过渡动画** ✨
当切换主题（Latte → Mocha 等）时，按钮颜色会平滑过渡，而不是突然变化。

**技术细节**：
- 使用 `ColorAnimation` 实现颜色渐变
- 持续时间：200ms
- 缓动曲线：`Easing.OutCubic`（柔和的减速效果）
- 同时作用于背景色、边框色和文字颜色

**触发方式**：
点击顶部的"测试按钮"，会循环切换四种 Catppuccin 主题：
- Latte（浅色）
- Frappe
- Macchiato
- Mocha（深色）

---

### 2. **鼠标悬浮颜色变化动画** 🖱️
当鼠标悬停在按钮上时，颜色会变亮 15%，创造出高亮效果。

**技术细节**：
- 颜色亮度调整系数：`1.15`（即增加 15% 亮度）
- 动画持续时间：200ms
- 缓动曲线：`Easing.OutCubic`
- 配合缩放动画（放大到 1.04 倍）

**实现原理**：
```qml
function adjustColorBrightness(color, factor) {
    var r = Math.min(255, Math.max(0, color.r * 255 * factor))
    var g = Math.min(255, Math.max(0, color.g * 255 * factor))
    var b = Math.min(255, Math.max(0, color.b * 255 * factor))
    return Qt.rgba(r / 255, g / 255, b / 255, color.a)
}
```

---

### 3. **点击时颜色变化动画** 🎯
当点击按钮时，颜色会变暗 15%，增强按下的反馈感。

**技术细节**：
- 颜色亮度调整系数：`0.85`（即减少 15% 亮度）
- 动画持续时间：200ms
- 缓动曲线：`Easing.OutCubic`
- 配合现有的"果冻"缩放动画

**动画组合效果**：
1. **按下瞬间**：缩小到 0.96 倍 + 颜色变暗
2. **回弹阶段**：放大超过 1.06 倍（果冻效果）
3. **回到悬浮**：保持 1.04 倍 + 颜色变亮
4. **离开悬浮**：回到 1.0 倍 + 恢复原始颜色

---

## 📊 动画参数说明

### 颜色过渡参数
| 参数 | 值 | 说明 |
|-----|-----|-----|
| `duration` | 200ms | 颜色过渡持续时间 |
| `easing.type` | `Easing.OutCubic` | 缓动曲线，开始快结束慢 |
| `hoverBrightness` | 1.15 | 悬浮时亮度增加 15% |
| `pressBrightness` | 0.85 | 按下时亮度降低 15% |

### 缩放动画参数（已存在）
| 参数 | 值 | 说明 |
|-----|-----|-----|
| `baseScale` | 1.0 | 基础缩放比例 |
| `hoverScale` | 1.04 | 悬浮时缩放比例 |
| `pressScale` | 0.96 | 按下时缩放比例 |

---

## 🎬 动画流程图

### 主题切换流程
```
用户点击切换主题
    ↓
ThemeManager.setCurrentPaletteType()
    ↓
发出 paletteChanged 信号
    ↓
NanButtonStyle.updateColor() 被调用
    ↓
发出 styleChanged 信号
    ↓
QML 中的 Connections 接收信号
    ↓
更新 currentBackgroundColor/BorderColor/ForegroundColor
    ↓
ColorAnimation 自动触发
    ↓
颜色平滑过渡（200ms）
```

### 交互动画流程
```
鼠标悬浮
    ↓
hovered = true
    ↓
颜色计算：adjustColorBrightness(baseColor, 1.15)
    ↓
缩放动画：scale → 1.04
颜色动画：color → 变亮 15%
    ↓
鼠标按下
    ↓
down = true
    ↓
颜色计算：adjustColorBrightness(baseColor, 0.85)
    ↓
缩放动画：scale → 0.96
颜色动画：color → 变暗 15%
    ↓
松开鼠标（果冻动画序列）
    ↓
1. 压缩到 0.96 (70ms)
2. 超弹到 1.06 (140ms)
3. 回归到 1.04 (120ms) - 如果仍在悬浮
```

---

## 🎨 效果预览

### 不同主题下的按钮样式
运行程序后，你会看到：
- **14 种按钮样式**（filled 和 outlined 各 7 种）
- **4 种主题配色**（可通过顶部按钮切换）
- **所有按钮同步切换**（带平滑颜色过渡）

### 交互效果
1. **悬浮效果**：按钮微微放大 + 颜色变亮（视觉上"浮起来"）
2. **点击效果**：按钮压缩 + 颜色变暗 → 回弹果冻效果
3. **主题切换**：所有按钮颜色同步平滑过渡

---

## 🔧 自定义动画参数

如果想调整动画效果，可以修改 `NanButton.qml` 中的属性：

```qml
// 调整悬浮效果
property real hoverBrightness: 1.15  // 改为 1.2 会更亮
property real hoverScale: 1.04       // 改为 1.08 会更大

// 调整按下效果
property real pressBrightness: 0.85  // 改为 0.7 会更暗
property real pressScale: 0.96       // 改为 0.92 会压缩更多

// 调整动画速度
// 在 Behavior on color 中修改 duration
duration: 200  // 改为 300 会更慢，改为 100 会更快
```

---

## 💡 技术亮点

1. **响应式颜色管理**：通过 `Connections` 监听 `styleChanged` 信号，实现主题切换时的自动更新
2. **颜色亮度算法**：动态调整 RGB 值，保持颜色色调不变，只改变亮度
3. **多层动画协调**：缩放、颜色、透明度多个动画同时工作，互不干扰
4. **性能优化**：使用 `Behavior` 自动管理动画，避免手动控制的复杂性

---

## 🚀 运行测试

1. **编译项目**：
   ```bash
   cd /mnt/workspace/Cpp/NandinaUI/TryNandina
   cmake --build build --target appTryNandina
   ```

2. **运行程序**：
   ```bash
   ./build/appTryNandina
   ```

3. **测试步骤**：
   - 点击顶部"测试按钮"切换主题，观察颜色过渡效果
   - 将鼠标悬停在任意按钮上，观察颜色变亮和放大效果
   - 点击按钮，观察颜色变暗、压缩和果冻回弹效果
   - 在不同主题下重复测试，验证所有主题下动画都正常工作

---

## ✨ 总结

现在你的 NanButton 组件拥有：
- ✅ 主题切换时的平滑颜色过渡（200ms）
- ✅ 鼠标悬浮时的颜色高亮效果（+15% 亮度）
- ✅ 点击时的颜色按下效果（-15% 亮度）
- ✅ 配合现有的缩放和果冻动画
- ✅ 四种 Catppuccin 主题完美支持

所有动画都是平滑、流畅、协调的，给用户带来极佳的交互体验！🎉
