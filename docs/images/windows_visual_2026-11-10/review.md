# Windows 人工视觉验收证据（2026-11-10）

采集环境：Windows 11 26200，主屏 2880×1800 @175%（逻辑窗口 1200×800 → 客户区 2100×1400 物理像素），
Radeon 660M + D3D12，showcase 构建 `buildDir/showcase/nandina_showcase.exe`。
采集前把窗口移到屏幕内 (400,100)，避免右下角系统通知污染；因此所有图都是纯客户区、不含标题栏
与桌面内容。每张页面图都经脚本校验“侧栏当前项 = 页面名”（10/10 通过）。

## 维护者审阅结论（2026-11-10）

逐页与逐状态审阅全部通过；侧栏选中态切换正确，中文行高/换行/间距/暗色对比度符合预期。
发现两点，均已处理或记录：

1. **Label 页 Font style 标本无差异（Regular / Medium / Bold / Italic）**：确认为内置默认字体族
   只注册 Caskaydia Cove Regular 一个 face，`set_font_weight/slant` 按最近 face 解析且没有合成
   字重/斜体。已记录为 P4.7（多 face 资源或合成回退），并在组件文档标注。
2. **「可编辑字段」缺少 hover 反馈**：已补 TextField 默认 hover 配方（accent 底 + ring 边框，
   focused 时复位为页面底色 + input 边框）。`state-field-normal-light.png` / `state-field-hover-light.png`
   / `state-field-focus-light.png` 为本轮修复后的采集。单测覆盖：鼠标进入/离开、禁用不产生 hover、
   默认配方 hover 底与边框变化、focused 优先复位（`[widget][text-field][hover]` /
   `[theme][text-field][hover]`）。

其余审阅意见：侧栏导航 hover 略弱但可接受；输入与焦点页右侧 “Ta/笔” 露出为页面自身滚动区裁切，
属预期；按钮按下态无 hover 叠加、涟漪完整、动画流畅；对话框遮罩/圆角/阴影/中文行高与 Esc 焦点
恢复均符合预期。

## 总览

- `contact-pages.png`：5 个页面 × 亮/暗两种外观（2 行 × 5 列）
- `contact-states.png`：交互状态 6 张（导航 hover、输入框 hover、输入框 focus、按钮 pressed、
  对话框打开、对话框关闭焦点恢复）

## 逐页文件

| 页面 | 亮色 | 暗色 |
| --- | --- | --- |
| 首页 | `page-home-light.png` | `page-home-dark.png` |
| 锚点 | `page-anchors-light.png` | `page-anchors-dark.png` |
| Label | `page-label-light.png` | `page-label-dark.png` |
| Image | `page-image-light.png` | `page-image-dark.png` |
| 输入与焦点 | `page-input-focus-light.png` | `page-input-focus-dark.png` |

状态：`state-nav-hover-light.png`、`state-field-normal-light.png`、
`state-field-hover-light.png`、`state-field-focus-light.png`、
`state-button-pressed-light.png`、`state-dialog-open-light.png`、
`state-dialog-closed-light.png`。

## 复验方式

```powershell
cd D:\CLang\NandinaUI
.\buildDir\showcase\nandina_showcase.exe
```

侧栏顺序：首页 / 锚点 / Label / Image / 输入与焦点；暗色开关在侧栏底部。
图片均为 175% 物理像素 1:1 采集，可直接与当前构建对照。
