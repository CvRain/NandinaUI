# 文本整形与页面打开成本

> 本文记录「点击一个页面到它出现」这段时间花在哪里，以及为此修掉的两个 O(n²)。
> 它是[布局收敛与文本测量](layout_convergence_and_text_measurement.md)的续篇：那篇解决的是
> **稳态每帧重排**，这篇解决的是**一次性打开成本**。
> 所有数字都可用 `playground/bench/label_bench.cpp` 复现（该目录被 `.gitignore` 忽略）。

## 1. 摘要：页面打开时间

测量方式：真窗口，`route start → master 树构建完成` 记一次，随后记录前几帧的布局耗时。
窗口 1200×800，Wayland + raylib/OpenGL，本机 AMD RX 6750 GRE。

| 场景 | 构建 | 首次布局 | 打开合计 |
| --- | --- | --- | --- |
| Label 页面（最初） | 858 ms | ~119 ms | **~977 ms** |
| Label 页面（去掉两个 O(n²) 后） | 378 ms | ~124 ms | ~502 ms |
| Label 页面（文本记忆化后） | **159 ms** | ~112 ms | **~271 ms** |
| Home 页面 | 31 ms | ~22 ms | ~53 ms |
| 压力页：1 个 Label × 5000 汉字（最初） | **6440 ms** | ~205 ms | ~6.6 s |
| 压力页：同上（现在） | **76 ms** | ~205 ms | ~281 ms |

稳态帧时间保持 2.2 ms / 0 次布局；强制整页重排从 67 ms 降到 55 ms。

复现：

```bash
./buildDir/playground/label_bench label     # 真实 Label 页面
./buildDir/playground/label_bench home      # 对照
NANDINA_BENCH_STRESS_CHARS=5000 ./buildDir/playground/label_bench stress
```

## 2. 两个 O(n²)

两者都只在文本变长时才暴露，短文本上看不出来 —— 这也是它们此前没被发现的原因。

### 2.1 caret 停靠点去重（HarfBuzz 后端）

`normalize_caret_stops()` 先把停靠点按 x 排序，然后逐个在**已收敛结果**里线性查找重复项：

```cpp
for (const auto& stop : line.caret_stops) {
    const auto duplicate = std::ranges::find_if(normalized, ...);   // O(n) × n 次
}
```

候选停靠点约为簇数的 2 倍（每簇一个下游 + 一个上游），一行 5000 字时约 1 万个停靠点 →
上亿次比较。修法：按 `(source_offset, x)` 排序把同一 offset 的停靠点聚到一起，再**只在组内**
比较（组通常只有 1~2 项），最后恢复按 x 排序。语义逐项等价（先出现者胜出、`downstream`
覆盖 `upstream`），复杂度 O(n log n)。

`append_run_caret_stops` 的 1.5 ms 仍在，但 `normalize` 的 5.1 ms 降到约 0.1 ms：
整形路径里这一项原本占 **36%**（`hb_shape` 本身只占 24%）。

### 2.2 逐簇推进重新分段（确定性 fallback 后端）

`next_grapheme_boundary(text, offset)` 原本是：

```cpp
const auto ranges = foundation::utf8::grapheme_ranges(text.substr(offset));
return ranges.empty() ? text.size() : offset + ranges.front().length;
```

取「下一个簇边界」却把**整段剩余文本**重新做了一遍 UAX #29 分段，还每次分配一个 vector。
换行循环对每个簇调用一次 → 整段 O(n²)。5000 字的构建因此要 6.4 秒。

修法：在 `foundation::utf8` 增加**非分配**的
[`next_grapheme_boundary()`](../../nandina/foundation/utf8.hpp)：用与 `grapheme_ranges()` 相同的
`utf8proc_grapheme_break_stateful` 状态机，只解码到边界为止。

这一条特别重要，因为**页面构建期用的是这个 fallback 后端**：`Text` 在构造时先测量一次，
而字体上下文要等节点进入场景树才注入。所以构建期的每一次测量都走 fallback 路径。

## 3. 整形成本曲线（修复后）

`HarfBuzzTextLayoutBackend`，CJK 文本，tight(1173) 约束，固定种子：

| 长度（汉字） | `clip` | `wrap` |
| --- | --- | --- |
| 200 | 0.066 ms | 0.061 ms |
| 1000 | 0.348 ms | 0.737 ms |
| 3000 | 1.131 ms | 2.382 ms |
| 5000 | 1.932 ms | 4.305 ms |

线性，约 0.4 µs/字符（`clip`）/ 0.9 µs/字符（`wrap`）。`wrap` 约 2× 是因为它先整形整段
再逐行整形；`hb_shape` 本身不是瓶颈。

修复前同一张表在 200 字符处是 `clip` 414 µs / `wrap` 793 µs（即约 2 µs/字符、wrap 4 µs/字符），
差额几乎全部来自 §2.1。

## 4. 被实测否决的方案：整形结果缓存

曾实现 `text::CachedTextLayoutBackend`（按完整排版输入做键、带 LRU 上限的装饰器，接进
`FontPipeline`、计入 retained bytes）。实测后**整体回退**：

- 命中率极低：600 次强制重排里 `hits=3333 / misses=71433`。原因是同一节点在 measure 与
  arrange 拿到的紧约束**高度不同**，键几乎不可能重复。
- 失配路径反而更贵：约 +750 µs/次（键串拷贝 + 结果复制 + LRU 扫描）。

结论：成本在**整形内部**，不在「重复调用」上；不要在外面套缓存。这条记录下来的目的是
避免下一个人重走。

## 5. 剩余目标

### 5.1 构建期的重复测量（已实施）

Label 页面构建的 378 ms 里，文本测量占主要部分。加探针统计后事实很清楚：

```
[measure] total=16000 outside_pass=14250 inside_pass=1750 total_ms=1128.5
```

`NanControl::measure_layout_with_basis()` 被调用约 1.6 万次，其中 **89% 发生在布局周期之外**
—— 也就是构建期容器「每加一个子项就 `relayout()` 一次」造成的整棵子树重复测量。真正落在
布局周期里的只有 1750 次。

两条可选路线：

1. **取消急切的 `relayout()`**（构建期只标脏、挂载后一次布局）。实测把 8 个容器的
   `relayout()` 全部改成空实现后，80 个单测里只有 **3 个**失败，且失败点都是同一条既有
   约定：「脱离树的控件在构建后立刻就有尺寸」。这是一条**可见的行为契约**，不该顺手改掉。
2. **按完整排版输入记忆化 `Text::update_metrics()`** —— 本轮采用。

选择 2 的理由是它没有「忘了失效」这一类静默错误：键是**完整的排版输入**（文本 + 所有影响
排版的样式字段 + 约束），命中即输入逐字段相同，结果必然相同。颜色不进键，因为它不影响排版，
而 `TextLayoutResult` 里也不含颜色。唯一不在键里的是**后端对象**，所以 `resolve_font()` 与
`set_text_pipeline()` 必须显式清缓存（有测试固定）。

条目数是量出来的，不是拍的（Label 页，构建期命中率）：

| 条目数 | 命中率 | 构建 |
| --- | --- | --- |
| 2 | 8% | 277 ms |
| 4 | 45% | 195 ms |
| **8** | **74%** | **159 ms** |

8 条之后曲线已经变平（而且每条存的是完整排版结果），所以再加一条**字节预算**：超过
`kMetricsCacheByteBudget`（64 KB）的结果只保留一条，避免一个 5000 字的 Text 同时驻留 8 份
几百 KB 的排版结果 —— 压力页实测命中 2/11 次、构建 75 ms，没有回退。

结果：构建 **378 → 159 ms**，强制整页重排 55 → 46 ms，RSS +2 MB。剩下的 26% 未命中是
**真实的输入差异**（构建过程分步应用字号/溢出/换行策略），不是缓存没生效。

### 5.2 对照排除的假设

- `theme::design_system_from_theme()`：控件目前每个实例调用 1~2 次，实测 **0.024 ms/次**，
  不是瓶颈（曾怀疑它，实测后排除）。
- `NanControl::measure_layout_with_basis` 与 `_propagate_invalidate_global()` 的调用次数都在
  万级以下，不是瓶颈（曾怀疑 O(N²) 子树失效，实测后排除）。

## 6. 契约与测试

| 测试 | 断言 | 改坏哪一行会红 |
| --- | --- | --- |
| `tests/foundation_tests.cpp`「next_grapheme_boundary agrees…」 | 逐簇边界与 `grapheme_ranges` 的每个 range 起点逐字节一致；必须前进；越界收敛到 `size()` | 状态机推进顺序写错 → 红 |
| `tests/foundation_tests.cpp`「a long grapheme run…」（budget） | 10000 个汉字的逐簇推进 < 200 ms | 换回 `grapheme_ranges(substr)` → 红 |
| `tests/text_layout_header_tests.cpp`「deterministic wrap…」（budget） | 5000 汉字 + tight 宽度换行 < 500 ms | fallback 后端换回O(n²) 分段 → 红 |
| `tests/font_tests.cpp`「HarfBuzz caret stops dedupe…」（budget） | 8000 簇单行的停靠点数量与顺序正确，且 < 500 ms | 去重换回线性查找 → 红 |

两个 budget 断言的阈值都留了 10× 以上余量，断言的是「没有退化成二次」，不是精确性能；
改坏对应实现时它们确实会红，已在提交说明里逐条核对。

## 7. 空闲 CPU 与常驻内存

### 7.1 空闲 CPU：每帧一次全量布局

实测 showcase 空闲时占满**一个核的 72%**（用户在系统监视器里看到 5–8%，那是 8 核口径）。
逐帧诊断显示：布局根每帧都是脏的，一次布局 55 ms，帧时间 117 ms。

根因与 §2 同类，但在 `Expanded` 上：它的子项**本来就会被指派到与自己测量值不同的尺寸**
（填满父级分配的剩余空间）。`layout_to()` 先清标记、子项的 `set_size()` 随后把祖先链标脏，
于是每一轮布局都留下新的脏标记。侧边栏的 `Expanded(ScrollView)` 让**每个页面**都中招，
包括首页 —— 这也解释了为什么我的单页 bench（不挂 shell）一直是 0 次布局，而真实 showcase 不是。

修法（`NanControl::set_size`）：周期内的尺寸写入是**布局的输出**，不再向上标脏；周期外
（构造、样式变化、外部 setter）照常标脏。周期内控件**自己**显式调用的
`mark_layout_dirty()` 不受影响 —— 那是输入变化，由既有测试
「layout invalidation raised during layout is preserved」固定。

结果：空闲 CPU **72% → 9%（单核口径）**，帧时间回到 16.7 ms 的 vsync 地板，每轮运行只剩
2 次布局。剩下的 9% 是绘制遍历的成本（每帧全量重绘、每个有子节点的容器分配 vector 并排序
z 序、无裁剪剔除），属于 §8 的渲染工作线。

### 7.2 常驻内存：主要是分配器留住的临时对象

showcase RSS 约 157 MB，其中 **95.7 MB 是 file-backed/shared**（GL 驱动、系统库、字体文件），
**60.8 MB 是 private-dirty（匿名堆）**。逐项实测（`NANDINA_BENCH_*` 探针 + `/proc/self/status`）：

| 项 | 实测 |
| --- | --- |
| Label 页构建（166 个节点） | +21.1 MB |
| 同一时刻 `malloc_trim(0)` 之后 | **−19.2 MB**（回到 +1.9 MB） |
| 活对象合计 | 约 2 MB（`Label` 2.5 KB、`Card` 1.9 KB、`Text` 1.5 KB、`Button` 5.5 KB） |
| `theme::DesignSystem` | **15.0 KB/实例**（控件目前每个实例构建一份） |
| `theme::NanTheme` | 852 B（按值存放在每个 Label 里） |

也就是说：页面构建那 21 MB 里**只有约 2 MB 是活对象**，其余是**构建期上千次临时分配被分配器
留住**的页。这和 §5.1 是同一个根因（构建期 1.1 万次测量，每次都要分配布局结果的临时 vector
与 string），所以「减少构建期测量次数」同时回收构建时间与这部分 RSS。

字体图集的实际占用（4 条管线 × 2 个 face）：

| 默认起始尺寸 | CPU alpha | RGBA8 纹理 |
| --- | --- | --- |
| 1024²（旧） | 8 MB | 32 MB |
| 256²（新，按需增长） | 0.5 MB | 2 MB（实测增长到 512² 时约 2 MB / 8 MB） |

实测把默认起始尺寸改成 256² 后：进程 RSS **−6 MB**，渲染结果**逐字节相同**
（截图 sha256 一致）。注意 **RGBA8 纹理那 32 MB 并不体现在进程 RSS 里** —— 它由 Mesa 以
GEM 缓冲分配，属于显存/驱动侧；所以「省 RSS」只能看到 CPU alpha 那一半。

### 7.3 图集格式：单通道要等后端迁移

raylib 后端目前把 1 字节 alpha 展开成 RGBA8（`alpha_to_rgba`）。改成 2 字节 `GRAY_ALPHA`
**在 OpenGL 核心 profile 下不可行**：raylib 把它映射成 `GL_RG8`（rlgl.h:3663），默认着色器
采样得到 `(r, g, 0, 1)`，alpha 恒为 1 —— 字形会变成不透明方块。要用单通道必须配一个
专用字形着色器（把 `.r` 当 coverage），这属于 D4 明确列出的后端迁移工作（那里的结论就是
「CPU atlas 字节」与「backend texture estimate」分开记账）。本轮因此只做起始尺寸。

### 7.4 剩余的内存项（按收益排序）

1. **减少构建期测量次数**（§5.1）：同时省时间与约 19 MB 分配器驻留。
2. **单通道图集 + 专用字形着色器**：显存减半，随后端迁移做。
3. **同一 face 的图集跨管线共享**：现在 4 条管线各持 2 个 face 的图集，同 face 被重复分配。
4. **`DesignSystem` 按主题共享一份 —— 已实施（2026-11-23，§14）**：`default_design_system()`
   的基线改为进程级静态构建一次，新增 `design_system_shared_from_theme()` 的 8 槽有界缓存，
   33 类控件共 67 处构造/主题路径改用共享不可变快照；控件不再各持 15 KB 副本（每主题最多
   8 份）。单次 0.024 ms 仍是内存项而非 CPU 项的结论不变；实例数放大后的切页贡献见 §14.1/§14.3。

## 8. 断行机会（已实施）

### 8.1 问题

`wrapped_ranges()` 原先只做「能塞几个簇就塞几个」，没有任何断行机会的概念，
于是英文会从单词中间断开。Label 页面上肉眼可见：`cont|inues`、`availa|ble`、
`when t|he user needs`。

### 8.2 规则（UAX #14 的实用子集）

新增 `nandina/text/line_break.{hpp,cpp}`，判定「某个 grapheme cluster **之后**能否断行」：

1. 空格 / 制表符之后 —— 拉丁文本的主要断点；
2. 连字符、短/长破折号（U+2010..U+2015、U+2212）之后 —— `well-known`；
3. 宽字符之后 —— CJK 表意文字、假名、谚文、全角标点等（U+1100..U+11FF、
   U+2E80..U+33FF、U+3400..U+9FFF、U+A000..U+A4CF、U+AC00..U+D7A3、U+F900..U+FAFF、
   U+FE30..U+FE4F、U+FF00..U+FF60、U+20000..U+3FFFF）。这些书写系统本来就可以在任意
   字符之间断行，所以每个字符都是断点。

**找不到机会时退回按簇断行** —— 超长单词与 URL 必须能在词中间断开，否则会溢出盒子。

两个后端都用这条规则：HarfBuzz 的 `wrapped_ranges()` 在放不下时回退到本行最后一个机会
（并重算那一段的宽度）；fallback 后端在按字符数切完之后回退到 `last_break_opportunity()`。

### 8.3 效果与测试

Label 页截图对比：`continues`、`width is limited`、`content area` 都改成按词断开，
CJK 段落照常逐字换行。测试在 `tests/text_layout_header_tests.cpp`（不依赖字体）：

| 测试 | 钉住什么 |
| --- | --- |
| 「line break opportunities follow spaces, hyphens and wide characters」 | 规则表：空格/制表/连字符/en dash/CJK/假名/谚文/全角标点为真；拉丁字母、句点、组合序列、emoji、空串为假 |
| 「last break opportunity picks the latest allowed break」 | 取最靠后的机会；无机会时返回 `begin`（调用方保留原切分点）；越界范围被夹住 |
| 「deterministic wrap breaks at word boundaries」 | 集成：`alpha beta gamma` 断成 `"alpha "` / `"beta gamma"`；无空格的长串仍按字符断；CJK 每行 10 个汉字 |

### 8.4 已知限制

这不是完整的 UAX #14：没有 Line_Break 属性表，也不做「禁止在收尾标点之前断行」这类成对
规则。实际后果是**一行可能以收尾标点开头**（例如 `」`、`，`），以及不做西文连字断词
（hyphenation）。要补齐需要引入属性表，属于后续工作。

## 9. 未纳入本次

- 构建期 relayout 策略（§5.1）：会改变「未挂载控件的尺寸何时有效」这条既有行为，需单独定案。
- `wrap` 先整形整段再逐行整形（约 2× 浪费）：需要增量断行，属于文本后端内部重构。
- 完整 UAX #14（属性表 + 收尾标点规则）与西文断词（hyphenation）：见 §8.4。
- 绘制遍历的 z 序缓存与裁剪剔除：见 [跨平台后端探测](cross_platform_backend_probe.md)。

## 10. 页面加载阻塞（2026-11-11 诊断与 P4.8b 实施）

用户反馈文本密集页面（showcase Label）进入时要等待加载完成，期间无法操作。实测页面成本
（`page_probe` 进程墙钟差值，Windows/Linux）：Label 比 anchors 多约 **0.2–0.4 s**，与本文
§5.1 的构建测量和首次真实整形同源。根因是 Router 在 UI 线程同步构建整页并在同一帧完成
首次布局，没有先画外壳/骨架或让出帧。

**P4.8b 已实施（2026-11-11）**：`PageContext::defer_section` 让页面把重节注册为“占位 +
builder”，Router 在首帧后每帧最多构建一节并在原兄弟位置替换占位；showcase Label 页只在
导航事务里构建简介节，Typography/Font style/颜色/Wrapping/Overflow/Alignment/Reactive/
Semantics 在后续 8 个 dispatcher drain 中挂载。Windows probe 首帧页面成本从约 0.2–0.4 s
降到约 **85 ms**，完整 1200×800 页面与迁移前 golden 为 0 像素差异。其余候选方案（构建期批量
测量、长期异步整形）与验收见 [页面加载响应性](page_load_responsiveness.md)。

## 11. GlyphAtlas 窄条增长缺陷（2026-11-11 修复）

现象：按侧栏顺序首页 → 锚点 → 标签 → 排版 → 图片 → 输入与焦点访问后，输入页大量中文
字形缺失；日志出现
`GlyphAtlas: glyph 25x22 does not fit a 4096x4096 atlas; skipping it`，并显示实际图集只有
**256×4096**。文本布局本身没有错，缺的是字形位图。

根因：`GlyphAtlas::grow_to_fit()` 只扩展“当前这次分配需要的轴”。一次行尾换行后，
`required_width` 回落到单个字形宽度，而 `required_height` 超过当前高度；于是图集只向纵向
翻倍，宽度长期停在初始 256，最终成了一条 256×4096 的窄条，横向 94% 空间从未使用。

修复：扩容时**两轴一起翻倍**（上限仍为 `kMaxDimension = 4096`）。已缓存的
`pixel_bounds` 不变；回归用例
`[text][atlas][growth]` 用 32×32 起始、全部窄于初始宽度的字形强制纵向压力，要求扩容后
宽度也增长（红先：修复前宽度停在 32、图集成 32×128 长条）。真实 175% showcase 复验：
按上述顺序访问后无 `GlyphAtlas` 错误，输入页中文完整；图集实际增长为方形尺寸。

**上限仍然存在**：4096² 是刻意的内存上限（8 位 alpha 16 MB CPU + 后端 RGBA 展开），
真正写满时仍会跳过字形而不是崩溃；长期方案是多图集页或字形淘汰，不在本批。

## 12. 文本复杂度守卫：结构单测 + 手工 release benchmark（2026-11-16）

**背景**：2026-11-16 的 CI ASan job 在 `tests/font_tests.cpp` 的
`REQUIRE(elapsed < 500.0)` 上失败（实测 556.86 ms），但该提交没有文本实现改动，
复跑即绿。单测在 ASan/UBSan 与共享 runner 上测量实际耗时不稳，不能作为门禁；
同类的三处 wall-clock 上限都按同一原则处理。

**单测只断言结构契约**（上限断言已删除）：

| 单测 | 结构断言 |
| --- | --- |
| `[text][harfbuzz][caret]` `HarfBuzz caret stops dedupe a long line`（`font_tests.cpp`） | 一行 8000 簇 → 1 行；停靠点数 = 簇数 + 末尾停靠点；首尾 offset 正确；按 x 有序 |
| `[foundation][utf8][grapheme]` `a long grapheme run advances one cluster at a time`（`foundation_tests.cpp`） | 每次 `next_grapheme_boundary` 必须前进；恰好 10000 个簇 |
| `[text][header]` `deterministic wrap splits a long run into bounded lines`（`text_layout_header_tests.cpp`） | 5000 字长串换行后行数 > 1，每行有停靠点，总宽不超过约束 |

**性能保护迁移到手工 release benchmark**：
`tests/benchmarks/text_performance_benchmark.cpp`（Meson 目标
`text_performance_benchmark`，**不注册为 `meson test`**，因此不进入 CI 矩阵）。
程序对上述三个工作负载做预热（默认 2 次）+ 多次采样（默认 9 次），报告
p50/p95/max，默认 p95 阈值 **2000 ms**（比原 500 ms 宽 4 倍）；阈值违规退出码 1，
`--max-p95-ms 0` 关闭阈值。用法：

```bash
meson compile -C buildDir text_performance_benchmark
./buildDir/tests/text_performance_benchmark
./buildDir/tests/text_performance_benchmark --samples 15 --warmup 3 --max-p95-ms 3000
```

2026-11-16 本机实测（debug 构建，p95）：

| 工作负载 | 普通构建 | ASan/UBSan 构建 |
| --- | --- | --- |
| caret 停靠点去重（8000 簇） | 30.9 ms | 102.8 ms |
| grapheme 前进（10000 簇） | 1.05 ms | 4.50 ms |
| deterministic 换行（5000 字） | 3.07 ms | 12.2 ms |

结论：CI 不再用墙钟断言做性能门禁；benchmark 是发布前/手工回归检查，
不声称存在稳定的 CI 性能门禁。GPU/SDF 与页面性能仍按既有约定留在本地/release 检查。

## 13. 拖拽分栏（Resizable）与文本整形热路径响应性优化（2026-11-23）

### 13.1 现象与四层乘法根因

在包含 `TreeView` + `ListView` + `DataTable` + `Accordion` 的多栏数据工作台（`playground` `data` 页）中拖拽外层 `Resizable` 横向分隔柄时，桌面交互帧率骤降至约 20 FPS。复现排查定位到四层叠加瓶颈：

1. **同帧高频 `MouseMoveEvent` 未合批**：`NanWindow::poll_and_dispatch_input()` 将一帧内来自高回报率鼠标的 10+ 条相邻移动事件逐条派发。
2. **输入期同步重排 + 双轴紧约束递归重测**：`Resizable::set_ratio()` 在 `FramePhase::input` 内每收到一次移动事件就立即执行一次 `layout_panes()`，同时 `mark_layout_dirty()` 在帧末 `layout_root()` 又再重排一次（10 次移动触发 11 次全子树重排）；嵌套 `Resizable` 与 `Expanded` 在双轴紧约束下 `on_measure()` 仍递归重测子项，引发 `2^depth` 倍增测量。
3. **`Text` 指标缓存对无关约束过严**：`Text::find_metrics_cache()` 原先要求 `(min_width, max_width, min_height, max_height)` 四元组完全相等，导致 measure 与 layout 之间每帧必然失配，拖拽每变动 1 px 宽度即使短文本完全不换行也会全部重整形；同时 `set_layout_backend()` / `apply_default_text_pipeline()` 切换后端时遗漏 `clear_metrics_cache()`。
4. **`HarfBuzzTextLayoutBackend` 回退选字逐字整形**：存在回退字体时，`font_for_text()` 为**每一个字符/字素簇**构造新 `hb_buffer_t`、调用 `FT_Set_Pixel_Sizes` / `hb_ft_font_changed` 并完整执行一次 `hb_shape()`，且在 `shape_fallback_run()` 与 `clusters()` 中频繁分配临时容器；纯 LTR 段落（ASCII / CJK）也无条件经过 FriBidi 分配与重排。

### 13.2 四层优化方案

| 层级 | 文件 | 改动摘要 |
| --- | --- | --- |
| 输入合批 | `nandina/app/nan_window.cpp` | `poll_and_dispatch_input()` 合并同帧内相邻 `MouseMoveEvent`（保留最终 `screen_pos` 并累加 `delta`），严格保持与按下/抬起/滚轮/键盘/IME 事件的相对顺序 |
| 延迟排版与去递归 | `nandina/widget/resizable.cpp`、`nandina/widget/layout.cpp` | `Resizable::set_ratio()` 在 `FramePhase::input` 中仅 O(1) 刷新 `last_geometry_` 并标脏，由帧末 `layout_root()` 统一排版一次（树外与非输入期维持即时落位）；`Resizable::on_measure()` 与 `Expanded::on_measure()` 在双轴紧约束下直接返回约束尺寸，消除嵌套重测 |
| 宽无关文本缓存 | `nandina/widget/primitives/text.hpp` / `.cpp` | `MetricsCacheEntry` 记录 `natural_width` / `natural_height` / `unwrapped` / `generation`；匹配时忽略仅作用于最终 `constrain()` 的 `min_width`/`min_height`/`max_height`，并对未换行且自然宽 + 1 px ≤ 新 `max_width` 的条目直接复用，同槽命中跳过 glyph/caret 向量拷贝；补齐 `set_layout_backend()` 与 `apply_default_text_pipeline()` 的缓存清理 |
| FreeType / HarfBuzz 热路径 | `nandina/text/freetype_font_face.cpp`、`nandina/text/harfbuzz_text_backend.cpp` | `FreeTypeFontFace` 缓存 `active_pixel_height` 避免重复 `FT_Set_Pixel_Sizes`；`HarfBuzzTextLayoutBackend` 复用持久 `scratch_buffer`、按槽位 `last_pixel_height` 延迟调用 `hb_ft_font_changed`、单码点簇走内联 UTF-8 解码 + 4096 上限 `single_cp_font_cache_` + `glyph_index()` 快速选字（多码点 ZWJ/变体选择符/组合标记保持完整整形优选）、`shape_fallback_run()` 改用零分配 `next_grapheme_boundary()`、`clusters()` 改用扁平排序 `std::vector` 替代 `std::map`、纯 LTR 段落跳过 FriBidi |

### 13.3 实测数据与回归验证

使用 `playground --resize-bench`（Debug `-O0`，`1240×940`，`SDL_VIDEODRIVER=dummy` 软件光栅化基线约 13.5–13.7 ms/帧）在优化前后对比：

| 场景 | 优化前（总耗时 / 纯拖拽增量） | 优化后（总耗时 / 纯拖拽增量） | 拖拽增量加速 |
| --- | --- | --- | --- |
| `steady-idle`（0 次移动/帧） | 13.47 ms/帧 | 13.62 ms/帧（3 次均值） | 基线持平 |
| `drag-1-move-per-frame`（1 次/帧，+15 px） | 25.75 ms（+12.28 ms） | 17.44 ms（+3.82 ms） | **3.2×** |
| `drag-10-moves-per-frame`（10 次/帧，模拟高回报率鼠标） | 109.11 ms（+95.64 ms） | 17.17 ms（+3.55 ms） | **26.9×** |

新增确定性结构回归（不含墙钟断言，均完成红先验证）：

- `[app][input][coalesce]`（`tests/window_input_tests.cpp`）：锁定同帧相邻 `MouseMoveEvent` 合并、累计 `delta` 与跨按钮事件保序（红先退化为逐条 7 事件失败）。
- `[widget][resizable][perf]`（`tests/resizable_tests.cpp`）：锁定嵌套 `Resizable` 在 `FramePhase::input` 连续 10 次 `set_ratio()` 期间 0 次子项排版、帧末 `layout_root()` 后各仅 1 次 measure + 1 次 layout（红先退化为 23 次 measure / 11 次 layout）。
- `[widget][text][cache]`（`tests/widget_tests.cpp`）：锁定 `min_width`/`min_height`/`max_height` 变动、未换行文本宽上限变化零额外整形，窄化触发换行与后端切换正确失效重算（红先 5 次宽变动产生 6 次多余后端调用）。
- Linux CPU 7 页 golden 门禁（`tools/visual_golden_ci.py --renderer sdl2d`）：7/7 PASS，`deterministic=true`，所有页面文本字形无回归。

## 14. 切页阻塞与窗口实时缩放顺滑度优化（2026-11-23）

### 14.1 现象与五处根因

在完成 §13 的 `Resizable` 分栏拖拽优化后，桌面复核继续暴露两个问题：(a) 从其他页切换到
`playground` 数据工作台（`page=data`）阻塞约 0.7 秒（本地插桩 Debug `-O0` 实测首次 704.9 ms、
二次热图集 656.1 ms，离开也耗时 173–180 ms）；(b) 拖拽系统窗口边框改变宽度时，普通页面
出现抽搐感，`page=data` 出现快缩漂移与快拉大停滞瞬移。

1. **虚拟化视口在预排阶段按全量内容撑开**：`ListView` / `DataTable` / `TreeView` 声明
   `height(fill)` 后，离树被 `Padding`/`Surface` 包裹触发 `relayout()`（`loose()` 无界约束），
   `on_measure()` 仍按全量行数返回 8,000+ px，导致 `on_layout()` 在 `get_tree() == nullptr`
   下同步物化全部 240 行（~2,640 个节点），首帧真实布局后再裁掉 95%。树内 `on_ready()` 的
   loose relayout 属于同一类**预排**（预排 = `get_tree() == nullptr || !layout_pass_active()`）。
2. **`StyleContext` 无变化级联与销毁期重算**：`NanNode::_resolve_style_context()` 未对未变化
   上下文剪枝；`_propagate_enter_tree()` 在每层递归整棵子树（$O(\text{depth} \times N)$）；
   `remove_and_delete()` 经 `remove_child()` 对即将析构的子树逐个重算
   `ResolvedStyleContext` 并回调 `on_style_context_changed()`。框架自身不设置 `StyleContext`，
   所以默认页面里省掉的是解析与逐节点回调（值不变时 `Text` 不会重新整形）；只有作者通过
   `StyleContext` 覆盖字体/字号时该回调才会触发文本重整形。
3. **`default_design_system()` 按控件实例重复构造**：33 类控件构造/主题路径共 67 处调用
   `design_system_from_theme(theme)`；该函数每次按值拷贝整张默认配方表（§5.2/§7.4 实测
   约 0.024 ms/次、每个快照约 15 KB），数据页在虚拟化修复前还会物化数千节点、每实例至少
   一次调用，挂树后又被 `ThemeManager::design_system_shared()` 的权威快照覆盖。
4. **`NanWindow::tick()` 窗口尺寸 1 帧错拍**：原实现在 `poll_and_dispatch_input()`
   （调用 `SDL_PollEvent()`）之前采样 `screen_size()`/`drawable_size()`，缩放帧用事件前的
   旧尺寸排版却提交到新尺寸表面。另补两处防御：`SdlPlatformWindow` 同时处理
   `SDL_EVENT_WINDOW_RESIZED`（固定 SDL 3.4.18 在有效 resize 时也会发
   `PIXEL_SIZE_CHANGED`，此为冗余保护）；零尺寸（最小化/零高窗口）帧跳过布局/绘制，
   而不是把 `make_window_metrics()` 的 `invalid_argument` 抛进宿主主循环。
5. **SDF 状态缓存帧内淘汰刷队列**：SDL 3.4.18 销毁仍属于当前未 flush 命令队列的
   `SDL_GPURenderState` 时会先 flush；窗口改宽使屏幕坐标键大面积失配，引发帧内频繁淘汰与
   flush，表现为缩放抽搐。

### 14.2 修复摘要

| 领域 | 文件 | 改动 |
| --- | --- | --- |
| 虚拟化视口预排保护 | `list_view.cpp`、`data_table.cpp`、`tree_view.cpp` | `size_spec().height` 为 `FillLength`/`PercentLength` 且 `max_height` 无界且处于预排时返回 `constraints.min_height`；真实布局 pass 内的无界父容器（`ScrollView`/`Stack`）保持既有内容高语义，避免视口在布局期塌陷为 0 行（见 [数据组件 §9.9.2](data_components_and_virtualization.md#992-预排阶段的虚拟化视口保护2026-11-23)） |
| 样式上下文剪枝与销毁免重算 | `style_context.hpp`、`node.hpp`、`node.cpp` | `ResolvedStyleContext` 默认 `operator==`；抽 `_apply_style_context()` 并在值未变时短路**整棵子树**（这才是 $O(\text{depth}\times N)$ 重复工作的来源）；`_propagate_enter_tree()` 只解析本节点并依赖自顶向下子遍历；`remove_and_delete()` 经 `detach_child(child, false)` 跳过离树重算（`remove_child()` 仍重置；被外部 `shared_ptr` 保留的节点重新挂载时由 `insert_child()` 重解析） |
| 设计系统快照共享 | `typography_scale.hpp`、`tokens.hpp`、`theme.hpp`、`design_system.hpp`/`.cpp`、`widget/*.cpp`（33 文件） | `default_design_system()` 静态基线只构建一次；新增 `design_system_shared_from_theme(const NanTheme&)`（8 槽互斥有界缓存、锁外构建、NaN 非法主题不入缓存）复用不可变快照；33 类控件共 67 处改用共享快照，`system_explicit_`/`on_theme_changed()` 行为不变 |
| 缩放采样时序与零尺寸防御 | `nan_window.cpp`、`sdl_platform_window.cpp` | `poll_input()` 后立即刷新视口映射（本帧输入坐标）；动画后、`layout_root()` 前重新采样 `screen_size()`/`drawable_size()` 并同步映射（布局/语义/绘制同源）；零尺寸帧安全跳过；`SdlPlatformWindow` 同时处理 `SDL_EVENT_WINDOW_RESIZED` |
| SDF 参数 state 延迟销毁 | `sdl_device.cpp` | 淘汰的 `SDL_GPURenderState` 先解绑、入 `retired_states`（上限 4×缓存容量 = 1024），下一帧 `begin_frame()`/析构批量销毁；构造时 `reserve` 防止淘汰回调分配失败；`cached` 仍是缓存条目数（≤256） |

### 14.3 实测与回归验证

**证据性质**：`playground/` 被 `.gitignore` 忽略、不入库；下表计时来自本地临时插桩
（Debug `-O0`、`SDL_VIDEODRIVER=dummy`、同机前后对比），不是 CI 证据，也没有可复现的
tracked benchmark。真实窗口的缩放顺滑度同样只有人工观察，见 §14.4。

| 场景 | 优化前 | 优化后 | 加速 |
| --- | --- | --- | --- |
| 首次进入 `page=data`（含 2 次 512² 图集扩容） | 704.9 ms | 61.3 ms | **11.5×** |
| 二次进入 `page=data`（热图集） | 656.1 ms | 33.1 ms | **19.8×** |
| 离开 `page=data` → `content` | 173.5 ms | 30.7 ms | **5.7×** |
| 离开 `page=data` → `loading` | 179.8 ms | 32.0 ms | **5.6×** |
| 12 次切页总耗时 | 1,892.8 ms | 332.8 ms | **5.7×** |

新增/补强的确定性结构回归（每条均做过红先验证）：

- `[list-view|data-table|tree-view][virtualization][off-tree]`：离树预排不物化全模型；
  补强 `tree.set_root()` 之后、`layout_root()` 之前的 `measured_size() == 12 px`（insets）
  断言，锁住"预排"包含树内 `on_ready()` loose relayout；`list-view` 另覆盖 `percent` 高度、
  `min_height` 保留、`ContentLength` 全量测量不变、离树显式 `layout_to(rect)` 仍物化可见窗口。
- `[list-view|data-table|tree-view][virtualization][pre-layout]`：真实布局 pass 中的无界父容器
  （`ScrollView` 直接子项 / `Column` 嵌套）仍按内容高测量、不塌陷为 0 行；红先：把条件退化
  为"仅按 `max_height` 无界判断"即返回 0 行失败。
- `[scene][style][perf]`：默认上下文的 insert/enter 零回调、显式覆盖每节点恰好一次、
  相同 `StyleContext` 重放零回调、`remove_and_delete()` 不触发离树回调、修改中间节点只重算
  自身子树；红先：去掉值相等剪枝后 leaf 多出 5 次回调。
- `[scene][style][detach]`：被外部 `shared_ptr` 保留的节点重新挂载必须重解析整棵子树、
  退出树通知仍传播。
- `[theme][design-system][cache]`：同值不同对象命中同一指针、palette 维度参与键、
  与 `design_system_from_theme()` 的 tokens/light/dark 值等价、NaN 非法主题不污染缓存、
  `default_design_system()` 返回独立副本。
- `[app][resize][timing]`：`poll_input()` 内 resize 后同一 tick 用新尺寸排版；
  `[app][resize][viewport]`：同一 tick 的指针事件用 poll 后的新视口映射；
  `[app][resize][invalid-size]`：0×0/零高窗口（含固定 viewport 配置）跳过布局不抛异常、
  尺寸恢复后继续排版。

验证矩阵（2026-11-23，Linux x86_64，本机；全部在最终代码树上重跑）：

| 范围 | 命令 / 证据 | 结果 |
| --- | --- | --- |
| 主套件 | `meson compile -C buildDir && meson test -C buildDir` | **146 OK / 0 FAIL** |
| ASan/UBSan unit | `meson compile -C buildDir-asan && LSAN_OPTIONS=detect_leaks=0 meson test -C buildDir-asan --suite unit` | 121 OK + 1 skip（`window-lifecycle` 既有驱动侧 LSan skip） |
| no-RTTI unit | `meson test -C buildDir-no-rtti --suite unit` | 123 OK / 0 FAIL |
| no-SQLite unit | `meson test -C buildDir-nosqlite --suite unit` | 118 OK + 1 skip |
| 纯工具 | `meson compile -C buildDir-tools && meson test -C buildDir-tools` | 8/8 OK |
| 门禁 | `python3 tools/release_check.py --ci --build-dir buildDir` | **8/8**（format/headers/artifacts/golden-manifest/test-suites/docs/tool-dependencies/whitespace） |
| Linux CPU 7 页 golden | `SDL_VIDEODRIVER=dummy python3 tools/visual_golden_ci.py --renderer sdl2d ...` | **7/7 pass**、`deterministic=true`，最大 changed fraction 0.0347%（单通道容差 ±3）；CJK 回退 = `NotoSansCJK-Black`，sha256 `3cffb102…`（与 CI 固定面一致） |

格式：仅对变更文件用固定 clang-format **23.1.1** 格式化，`tools/check_format.py` 通过。


### 14.4 剩余边界

- SDF 延迟销毁只有本地手工观察（Debug `-O0`、真实窗口实时缩放）；`retired_states` 队列与
  单帧 >1024 淘汰的帧内 drain 分支没有自动断言，Windows/D3D12 未复验，Linux/Vulkan 是唯一
  实测驱动。契约更新见 [渲染路径契约 §9.2/§10.4](render_device_path_contract.md#104-参数-state-延迟销毁2026-11-23)。
- 真实窗口实时缩放顺滑度、`SdlPlatformWindow` 的 `SDL_EVENT_WINDOW_RESIZED` 分支属于人工验收；
  dummy 驱动与 mock 平台都测不到内容缩放变化。
- 零尺寸帧防御只覆盖 `NanWindow::tick()` 的布局/绘制跳过；平台层事件语义仍由各后端负责。
- 8 槽共享快照是有界 FIFO（非 LRU、无失效入口）：超过 8 个不同主题轮换会抖动；含 NaN 的
  非法主题每次返回独立快照、不写缓存。
- `remove_and_delete()` 后仍被外部持有的节点，在重新挂载前 `resolved_style_context()` 可能
  仍反映旧父链（已写入 `nandina/scene/node.hpp` 的头注释）。
- Windows 最大化经 `RESIZED` 更新 `logical_size_` 后，再触发 `DISPLAY_SCALE_CHANGED` 时
  "按逻辑尺寸恢复窗口"的语义未定义/未验（低风险，属既有平台记账行为）。
- 同类残余拷贝：`internal::DialogPanel`/`SheetPanel` 现已于 2026-11-27 统一迁移到 `default_design_system_shared()` 进程级不可变共享快照（连同 `ThemeManager` 初始注册与 `ThemeSchemeContent` 只读模板）。
- 本批未改动 SDF/GPU 页面 golden 路径；GPU golden 需要真实桌面交换机，仍归人工/发布检查。

## 15. 窗口边缘拖拽延迟、固定尺寸组件抽搐与快拖跳变修复（2026-11-26）

### 15.1 现象与三条耦合根因

在 Linux GNOME Wayland（`mutter 51.1`、`wp_fifo_manager_v1`、`libdecor 0.2.5`、Vulkan SDF）实机拖拽窗口边缘缩放时，存在三类耦合异常：

1. **固定尺寸组件缩放抽搐**：vendored SDL 3.4.18 的 `SDL_CreateGPURenderer(device, window)` 在 `GPU_CreateRenderer` 中未注册 `WindowEvent`（`subprojects/SDL3/src/render/gpu/SDL_render_gpu.c`），窗口收到 `PIXEL_SIZE_CHANGED` 后 `data->backbuffer` 仍停留在上一帧尺寸 `(W_old, H_old)`，而 Vulkan swapchain 已重建为 `(W_new, H_new)`。`GPU_RenderPresent` 先把旧 `backbuffer` 按 `SDL_GPU_FILTER_LINEAR` 线性拉伸拷贝到新 swapchain 提交上屏，**随后**才调 `CreateBackbuffer(W_new, H_new)`，导致放大时固定尺寸组件先被拉伸变大一截、下一帧弹回原尺寸，缩小时先被压缩变小一截、下一帧弹回原尺寸。
2. **多文字页面停手后窗口如平滑相机缓滑追赶**：`VULKAN_INTERNAL_RecreateSwapchain`（`SDL_gpu_vulkan.c`）只调 `vkDeviceWaitIdle()`（约 `0.3 ms` GPU 绘制耗时，不等合成器 vblank），同时清空 `inFlightFences` 并创建全新图像使 `vkAcquireNextImageKHR` `0 ms` 返回；叠加 `libdecor 0.2.5 < 0.3.0` 不置 `wind->resizing`、立即 ACK configure，主循环以高于刷新率的频率连续提交数十个中间尺寸帧进入 `VK_PRESENT_MODE_FIFO_KHR` + Mutter `wp_fifo_v1` 队列，鼠标停下后以每 vblank 1 帧严格排空（实测停手后首帧 present 阻塞达 `86–323 ms`）。
3. **折行文本逐像素重塑与快拖跳变**：`Text::find_metrics_cache()` 此前只对**未发生折行的单行文本**做跨宽复用；一旦 `lines.size() >= 2`，窗口每改宽 `1 px` 就缓存失配重跑 `HarfBuzzTextLayoutBackend::layout()`，且 `HarfBuzzTextLayoutBackend` 在 `max_shaped_width()` 与整段预折行探测中重复构造 `std::string` 副本与 `caret_stops` / `visible_text` 元数据。前几帧同步排版 + `libdecor` Cairo 边框重绘 + swapchain 重建阻塞主线程期间，SDL 的 `RemoveSupersededWindowEvents`（`SDL_windowevents.c`）把积压的中间 `RESIZED` / `PIXEL_SIZE_CHANGED` 合并丢弃，表现为先慢挪一两帧随后直接跳到鼠标终点。

### 15.2 分层归属与修复摘要

不修改 `subprojects/SDL3`，全部在框架自有层按向下的依赖方向落地（`app → widget → text → foundation`）：

| 层级与文件 | 契约形状与改动 |
| --- | --- |
| `app`：`platform_window.hpp`、`nan_window.cpp` | `IPlatformWindow` 新增默认空实现 `prepare_frame(const NanColor& clear_color)`；`NanWindow::tick()` 在输入轮询与动画推进后重采样窗口尺寸（零尺寸跳过不变），并在 `device_->begin_frame()` / `clear()` **之前**调用 `platform_->prepare_frame(clear_color)`。 |
| `app`：`detail/sdl_platform_window.hpp`/`.cpp` | GPU 路径改为平台显式 `SDL_ClaimWindowForGPUDevice` + 无窗口 `SDL_CreateGPURenderer(gpu_device_, nullptr)`（claim 失败如 `SDL_VIDEODRIVER=dummy` 自动回落既有 `SDL_CreateRenderer(window_, nullptr)`）；平台自持 `SDL_TEXTUREACCESS_TARGET` 离屏纹理 `frame_target_` 并在绘制前由 `prepare_frame()` → `ensure_gpu_frame_target()` 按最新窗口像素尺寸对齐（初始零尺寸保底 `1×1`、绑定失败自动恢复旧目标，切换目标后立即重设 HiDPI `apply_render_scale()`）；`present()` 用纯函数 `make_present_transfer_plan()` 按逐轴最小公共尺寸 `1:1`（`SDL_GPU_FILTER_NEAREST`）blit 到 swapchain，未覆盖右/下边缘先按背景色清屏，彻底根除跨尺寸拉伸。同时设置 `SDL_SetGPUAllowedFramesInFlight(gpu_device_, 1)`，并在 `vsync=true` 的 `120 ms` 活跃缩放窗口期内临时切到 `MAILBOX`（Wayland 无 `MAILBOX` 时回退 `IMMEDIATE`）配合显示器刷新周期节流（窗口隐藏/最小化未能取得 swapchain 图像时同样按刷新周期节流防止离屏空转），新尺寸帧直接替换未上屏旧帧，静止超过 `120 ms` 后自动恢复 `VSYNC`。 |
| `text`：`text_layout.hpp`、`harfbuzz_text_backend.cpp` | `TextLayoutResult` 新增 `wrap_width_floor` / `wrap_width_ceiling` 与 `has_wrap_width_range()`（默认 `[0, 0)` 表示未认证）；`HarfBuzzTextLayoutBackend` 为纯 LTR（无软连字符 `U+00AD`）常规折行段精确计算不改变任何词断点与溢出态的半开区间 `[wrap_floor, wrap_ceiling)`（下界取折行簇区间宽度与重塑后各行实测宽度的上界，上界取任意行吸纳下一不可拆词元或硬切簇所需宽度的最小值）；同时移除 `shape_line()` / `shape_fallback_run()` 无用 `std::string` 拷贝，并为 `max_shaped_width()` 与整段预探测引入 `include_metadata=false` 跳过废弃 `caret_stops` 排序与 `visible_text` 拷贝（若整段未超宽则在原位补齐单行元数据，避免二次 `hb_shape`）。 |
| `widget`：`primitives/text.cpp` | `Text::find_metrics_cache()` 新增 `same_wrapped_lines` 分支：当缓存条目带有有效 `has_wrap_width_range()` 且有限 `max_width ∈ [wrap_width_floor, wrap_width_ceiling)` 时直接复用缓存字形与行度量。 |

### 15.3 实测数据与回归验证

真实桌面窗口实测（Linux GNOME Wayland `mutter 51.1`、RADV RX 6750 GRE、Debug `-O0`、`vsync=true`、40 帧连续 8px 拖拽缩放 + 停手后首帧静止提交延迟；采样后已回滚 `showcase/application.cpp` 临时插桩）：

| Showcase 页面 | 修复前缩放 layout 均值 | 修复后缩放 layout 均值 | 修复前停手后首帧阻塞（积压排空） | 修复后停手后首帧耗时 |
| --- | --- | --- | --- | --- |
| `home` | 1.62 ms | **1.02 ms** | 234.6 ms | **13.4 ms**（75 Hz 单帧周期） |
| `label` | 2.36 ms | **1.75 ms** | 159.1 ms | **13.4 ms** |
| `typography` | 3.54 ms | **1.98 ms** | 85.9 ms | **13.4 ms** |
| `surface` | 1.67 ms | **0.99 ms** | 112.4 ms | **13.4 ms** |
| `collapsible` | 1.69 ms | **1.25 ms** | 189.2 ms | **13.4 ms** |
| `anchors` | 0.42 ms | **0.38 ms** | 323.3 ms | **13.4 ms** |
| `focus` | 0.89 ms | **0.64 ms** | 296.4 ms | **13.4 ms** |

新增 5 条会红的回归测试：

- `[app][present][transfer]`（`tests/sdl_platform_input_tests.cpp`）：验证 `make_present_transfer_plan()` 在等尺寸、swapchain 更大、swapchain 更小、反向轴错配与零尺寸下的 `1:1` 拷贝宽高与边缘清屏标志；改回按 swapchain 尺寸拉伸或漏判零面积/清屏标志即红。
- `[app][resize][prepare-frame]`（`tests/window_input_tests.cpp`）：验证 `NanWindow::tick()` 在 `poll_input` 内 resize 后按 `prepare_frame → begin_frame → clear` 严格顺序传入新尺寸与背景色，且零尺寸帧跳过 `prepare_frame`；删掉 `NanWindow::tick()` 中的 `platform_->prepare_frame(clear_color)` 即红。
- `[app][window][resize-target]`（`tests/window_lifecycle_tests.cpp`）：真实 SDL 窗口从 `320×240` 改大到 `440×300` 的当帧，`end_frame` 读回渲染目标尺寸即等于窗口新像素尺寸，且固定 `80×40` 控件在 `x=40` 保持前景、`x=90` 保持背景（不随窗口横向拉伸）；去掉 `NanWindow::tick()` 的 `prepare_frame()` 或 `ensure_gpu_frame_target()` 即退回旧尺寸失败。
- `[text][harfbuzz][wrap-cache]`（`tests/font_tests.cpp`）：对真实字体的多行折行段遍历 `[ceil(floor), ceil(ceiling)-1]` 每一整数像素断言行切分、字形数与光标停靠点完全一致，词中查询宽度仍签发完整词断点区间（跨过 `wrap_width_ceiling` 或低于 `wrap_width_floor` 时断行真实改变），空行多段纯 LTR 正常签发、BiDi/RTL 与软连字符 `U+00AD` 段落按契约不签发，且单行 `wrap` 延迟补齐元数据的 `visible_text` 与 `caret_stops` 等价。
- `[widget][text][cache][wrap]`（`tests/widget_tests.cpp`）：`101` 步逐像素缩放拖拽只在实际跨词断点处调用后端（`≤ 5` 次而非 `101` 次），每像素尺寸与行内容同无缓存排版一致，且未认证区间（默认 `[0, 0)`）的后端仍逐宽重测；删掉 `Text::find_metrics_cache()` 的 `same_wrapped_lines` 分支即红。

### 15.4 剩余边界

- Linux GNOME Wayland + Vulkan 已实机验证；Windows（Vulkan GPU 路径；`SdlPlatformWindow` 当前未接入原生 D3D12 `SDL_GPUDevice`，无 Vulkan 时回退 `SDL_CreateRenderer`）与独立 Xorg 会话下的窗口边缘拖拽手感和 `MAILBOX` 切换仍待实机人工复验。
- `libdecor < 0.3.0` 客户端标题栏自身的 Cairo 缓冲分配与 SDL 上游 `RemoveSupersededWindowEvents` 合并队列中已被超越的历史尺寸事件属于窗口系统/SDL 正常最新尺寸折叠语义；主线程单帧耗时降入毫秒级后不再产生肉眼可见的起步卡顿跳变。
- 折行宽度区间证书仅对 `TextOverflow::wrap`、纯 LTR 常规分析段（`simple_ltr`）且不含软连字符 `U+00AD` 的结果签发；含复杂 BiDi/RTL 混排或软连字符的段落保持默认 `[0, 0)` 不签发区间，按原路径精确重算。
