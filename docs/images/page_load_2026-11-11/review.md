# P4.8b 渐进式分节挂载：人工视觉验收（2026-11-11）

> **结论：维护者审阅通过。** Label 页首帧只出现页面壳与简介，其余节在后续帧按占位逐节出现；
> 每帧 1 节的节奏被接受，整体表现正常。最终页面与迁移前逐像素一致。

## 证据

| 平台 | 首帧 | 完整页（30 帧） |
| --- | --- | --- |
| Windows | [label-first-frame-windows.png](label-first-frame-windows.png) | [label-full-windows.png](label-full-windows.png) |
| Linux | [label-first-frame-linux.png](label-first-frame-linux.png) | [label-full-linux.png](label-full-linux.png) |

采集：`page_probe` 1200×800 隐藏窗口、SDF/GPU 路径；Windows 175% 会话，Linux GNOME Wayland
+ RADV RX 6750 GRE。首帧成本（`FRAMES=1` 相对 anchors）：Windows ~85 ms、Linux ~38 ms；
完整 30 帧页面与迁移前 approved golden 为 0 像素差异。

## 审阅范围与记录

- 2026-11-11：维护者查看首帧/完整页截图后确认“效果不错没有什么问题，整体表现挺好的”，同意
  继续推进；每帧 1 节的渐进节奏被接受。
- 同批修复：Linux 短帧 probe 的 SIGSEGV（泵任务强引用页面根，页面树在 render device 之后
  析构）已改为弱引用，并有 `pending deferred pumps do not extend the page lifetime` 回归。
- 契约：[页面与路由](../../references/page_and_router.md) §4.1、
  [页面加载响应性](../../references/page_load_responsiveness.md)、
  [P4.8 实现记录](../../references/development_roadmap.md)。
