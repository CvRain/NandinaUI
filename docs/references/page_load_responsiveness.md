# 页面加载响应性：文本密集型页面的首帧阻塞

> **状态（2026-11-11）**：**P4.8b 渐进式分节挂载已实施**（`PageContext::defer_section` +
> Router 泵任务 + showcase Label 页迁移）；诊断输入保留在 §1–§2，实施后测量见 §1 末。
> P4.8c（opt-in 构建期批量测量）与长期异步整形仍未实施。触发场景：从 Sidebar 进入
> showcase Label 页（大量 Label/文本段落）时，页面要等构建与首次排版完成才出现，期间
> UI 线程不可交互。

## 1. 现象与实测

复现：showcase 点击 Label 导航，或 `page_probe` 以 `NANDINA_PROBE_PAGE=label` 启动。
表现为进入页面后数百毫秒的黑/旧帧，滚动、点击、切换都无响应，直到页面整体挂载完成。

`page_probe` 进程墙钟（含应用/字体初始化，3 帧；Windows 175% 桌面会话 GPU 路径、
Linux GNOME Wayland + RADV，各取两次稳定值）：

| 页面 | Windows | Linux |
| --- | --- | --- |
| home | ~465 ms | ~155 ms |
| anchors | ~371 ms | ~121 ms |
| **label** | **~778 ms** | **~352 ms** |
| label − anchors（近似页面成本） | **~408 ms** | **~231 ms** |

结论：文本密集页比轻页面多出约 **0.2–0.4 秒**的阻塞；这与历史
[文本整形成本](text_shaping_cost.md) 记录的 Label 页构建 159 ms + 首帧布局 ~112 ms 同源，
但当前页面内容更多，实际用户感知为一帧到数帧的卡顿。进程墙钟包含启动公共成本，因此只看
页面差值，不把它当作精确的页面耗时。

**P4.8b 实施后（2026-11-11，Windows/Linux 同一 probe 口径）**：Windows Label 页首帧
（`FRAMES=1`）相对 anchors 的差值为 **~85 ms**（实施前按 `FRAMES=3` 口径约 ~408 ms）；
Linux 同口径首发为 **~38 ms**（Label 165 ms vs anchors 127 ms，`FRAMES=1`）。`FRAMES=1`
只显示页面壳 + 已构建的简介节，Typography 及之后的节在后续帧出现。完整 30 帧 PNG 与迁移前
golden 为 **0 像素差异**；总整形工作仍是线性的，只是从“导航事务一次完成”分散到多帧。
Linux 短帧 probe 曾在退出时 SIGSEGV（泵任务强引用页面根，页面树拖到 render device 之后
析构），已改为弱引用并加回归测试；修复后 `FRAMES=1/3` 均正常退出。复测命令与首帧/完整截图
记录见 §7。

## 2. 根因

1. **同步构建整页**：`NanRouter::apply_navigation()` 在 UI 线程的 dispatcher task 中调用
   `page->build(context)`，页面作者在 `build()` 里一次创建所有节点；文本节点构造时会测量，
   样式分步应用又触发多次测量（已有每 Text 8 条/64 KB 的输入记忆化，剩余 26% 是真实输入差异）。
2. **首次真实整形**：页面节点进入场景树后才注入字体上下文，首次布局用 HarfBuzz + 字形图集
   真正整形；每个不同字号/字重/斜体组合还会新建 `FontPipeline` 与图集。整形成本本身是线性的
   （约 0.4–0.9 µs/字符），文本节点多、标点多、换行多时线性放大。
3. **没有让出帧**：构建、首次布局与字形上传都发生在同一帧内，UI 无法先画出页面外壳或骨架，
   也不能处理输入。

历史上已经解决的部分（不要重复优化）：逐簇 O(n²)、确定性 fallback 的换行 O(n²)、
每 Text 输入记忆化、空闲时每帧全量布局、图集起始尺寸；整形结果装饰缓存曾实测否决（键命中率低、
失配更贵）。详见 [text_shaping_cost.md](text_shaping_cost.md)。

## 3. 候选方案

| 方案 | 预期收益 | 成本/风险 |
| --- | --- | --- |
| **A. 渐进式分节挂载**（推荐） | 首帧立刻出现页面外壳/首节，其余节在后续帧按预算挂载；用户可先看到并操作 | 需要 Page/BuildContext 的延迟挂载 API、占位容器与生命周期/取消语义；页面作者需迁移 |
| B. 按视口虚拟化页面内容 | 只构建可见区域，长页面成本与首屏解耦 | 章节高度不固定，虚拟化/滚动稳定性复杂；showcase 场景不一定值得 |
| C. 异步整形/增量布局 | 把整形移出 UI 线程，理论最优 | HarfBuzz/FreeType face 非线程安全；图集上传必须在 UI 线程；大改文本管线 |
| D. 构建期批量模式（抑制急切 relayout、样式原子应用） | 减少重复测量；可能把构建再降 20–40% | 改变“脱离树的控件立刻有尺寸”的既有契约；需要 opt-in 与回归 |
| E. 页面实例/构建结果缓存 | 二次进入页面变快 | 首次进入无帮助；缓存失效/主题/路由参数复杂 |
| F. 跨节点排版结果共享缓存 | 重复文本/相同输入命中 | 历史 in-node 整形缓存已否决；跨节点命中率未测，需先加计数器评估 |

## 4. 实施顺序与当前状态

1. **P4.8a 测量基线**：本轮用 `page_probe` 的进程墙钟差值（轻页/重页、`FRAMES=1/3/30`）
   建立了可重复的对比口径；独立的 router/page 计时日志（例如 `NANDINA_PAGE_TIMING`）尚未
   加入，若后续需要逐节预算再补。
2. **P4.8b 渐进式分节挂载（已实施，2026-11-11）**：`PageContext::defer_section` 注册
   “占位 → builder”的节；Router 在页面首帧后每帧最多构建一节、在原兄弟位置替换占位；
   换页/销毁丢弃未执行节；builder 异常上报；无 dispatcher 宿主同步完成。showcase Label 页
   只同步构建简介节，Typography/Font style/颜色/Wrapping/Overflow/Alignment/Reactive/
   Semantics 共 8 节按帧挂载。`[app][router][deferred]` 覆盖首帧、逐帧、同步 fallback、
   换页丢弃、异常继续与**页面生命周期**；红先把泵任务改成空实现，3 个用例失败（`runs` 停在
   0）；生命周期修复前 Linux 短帧 probe（`FRAMES=1/3`）在退出时 SIGSEGV（泵任务强引用页面根，
   页面树被拖到 render device 之后才析构），改为弱引用根 + 锁后使用，并加
   `pending deferred pumps do not extend the page lifetime` 回归（红先恢复强引用后该用例失败）。
3. **P4.8c 构建期测量收敛**（未实施）：样式原子应用 / opt-in 批量构建，先量后改，
   不破坏“构建后即有尺寸”的契约。
4. **长期**：若分节后仍有单节超大文本，再评估异步整形或按需段落布局；
   `CachedTextLayoutBackend` 的历史结论仍有效，不要在没有新测量前重试。

## 5. 验收标准（P4.8b）

- `page_probe` 记录：Label 页首帧（外壳+已构建节）页面成本 ≤ 100 ms（Windows；已测 ~85 ms），
  全部节在有限帧内挂载（8 个延迟节 = 8 个 dispatcher drain）；轻页面（home/anchors）不回退。
- 导航切换/快速连点/离开页面无 use-after-free、无悬挂任务（`retiring a page drops pending
  deferred sections` 覆盖）；builder 抛错只影响该节并上报。
- 占位替换保持兄弟顺序（`replace_child` 的“删除后追加”语义不满足，Router 用 remove +
  insert at index）；滚动位置在分节挂载后不回跳。
- 语义树包含所有最终节点；6 条页面 golden 迁移前后 0 差异（当时基线；2026-11-11 本地化重批后每平台 8 条）。
- 文档记录 API、取消语义、占位策略，并更新本页与 `text_shaping_cost.md` 的交叉链接。

## 6. 与 P4.6 Step 2b 的关系

Step 2b 会新增 showcase 排版展示页、正文默认行高变化并让组件消费 role，文本节点数还会增加；
如果先完成 P4.8b，新页面可以直接用分节挂载开发，避免之后再次迁移；因此建议把 P4.8b 排在
Step 2b 的视觉迁移之前（P4.7 字重/斜体已完成，视觉验收不受影响）。

## 7. 证据

- Windows/Linux `page_probe` 墙钟测量：2026-11-11，见 §1；
- 历史文本整形成本曲线与已否决缓存：`text_shaping_cost.md` §3–§5；
- showcase Label 页包含 Typography / Font style / Semantic colors / Wrapping / Overflow /
  Alignment 六节，构建期节点数与测量次数见 `text_shaping_cost.md` §5.1。
