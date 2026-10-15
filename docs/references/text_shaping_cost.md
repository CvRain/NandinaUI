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
4. **`DesignSystem` 按主题共享一份**：现在每个控件 15 KB，一个页面就有上百次构造
   （实测单次 0.024 ms，所以这是内存项而不是 CPU 项）。

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
