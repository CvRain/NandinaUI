# P4.7 合成字重/斜体回退：人工视觉验收（2026-11-11）

> **结论：维护者审阅通过。** 高视口 Label 页 Font style 四行现在可区分：Medium 轻微加粗、
> Bold 明显加粗、Italic 倾斜；Regular 行保持原样。默认字体资产/许可不变，1200×800 页面
> golden 保持 0 像素差异，因此本批不重批基线。

## 证据

| 平台 | 图像 | 采集条件 |
| --- | --- | --- |
| Windows | [label-font-style-windows.png](label-font-style-windows.png) | MSYS2 UCRT64 构建，SDF/GPU 路径，`page_probe` 隐藏窗口 1200×1400 logical、30 帧，175% 会话 |
| Linux | [label-font-style-linux.png](label-font-style-linux.png) | Arch + GNOME Wayland + RADV RX 6750 GRE，SDF/GPU 路径，同一 1200×1400 视口、30 帧 |

两张图均由 `NANDINA_PROBE_WIDTH=1200 NANDINA_PROBE_HEIGHT=1400 NANDINA_PROBE_PAGE=label`
采集；golden 命令不使用这两个变量，固定 1200×800。

## 审阅范围与记录

- 2026-11-11：维护者查看 Windows/Linux 两张高视口图后确认“总体感觉还是挺不错的，效果也挺好”，
  同意继续推进。Medium 的合成强度与 Italic 的倾斜角度被接受。
- 同批已核实：默认 1200×800 的 6 条页面 golden 在 Windows 与 Linux 均为 `changed_pixels=0`、
  `max_channel_diff=0`（Font style 标本位于首屏以下，首屏仅可见未变化的 Regular 行）。
- 契约与实现边界：[合成字重/斜体回退](../../references/font_synthesis.md)；测试
  `[text][synthesis]`（决策表/分级强度/FreeType 栅格/HarfBuzz advance/GlyphAtlas，含红先）。

## 后续

Step 2b 的 role 迁移会让组件真正消费 500/600 字重，届时按正常流程做一次双平台页面 golden
重批（新增 showcase 排版页与正文默认行高也会同时重批）。
