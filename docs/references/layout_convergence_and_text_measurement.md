# 布局收敛与文本测量

> 本文定形「一次布局周期必须收敛」这条契约，以及与之相关的文本测量规则。
> 起因是一次实测：`showcase` 的 Label 页面在稳态下每帧重跑一次全量布局，
> 把全部文本重新整形一遍，实测 80.4 ms/帧（12.4 fps），而同规模的 Home 页面是 7.4 ms、稳态 0 次布局。
> 本文区分「已实测的事实」「已实现的修复」「未覆盖的缺口」和「开放项」；
> 只有标注为已实现的部分才代表代码现状。

## 1. 问题的实测证据

环境：Wayland + raylib/OpenGL，窗口 1200×800（合成器实际给到 1269×1386）。
测量方式：真窗口跑 690 帧，丢弃前 90 帧预热，取稳态 600 帧。
探测程序不在版本管理内（`playground/bench/label_bench.cpp`，`playground/` 被忽略）。

| 页面 | 帧时间均值 | 每帧布局次数 | 布局耗时 | 绘制+其他 |
| --- | --- | --- | --- | --- |
| Label（修复前） | 80.4 ms（12.4 fps） | 1.00 | 71.3 ms | ~9 ms |
| Label（修复后） | 2.5 ms | 0.00 | 0.002 ms | ~2.5 ms |
| Home（修复前/后） | 7.4 ms / 0.6 ms | 0.00 | 0.007 ms | — |

同一帧内连续调用 `NanSceneTree::layout_root()`（中间没有任何外部输入）：

```
修复前  Label: 1, 1, 1     ← 布局自己把树重新标脏，永不收敛
修复后  Label: 0, 0, 0
```

修复前，一次布局跑完后仍有 13 个节点 `layout_dirty()`，构成
`RouterOutlet → ScrollView → Padding → Column → … → Column` 的祖先链，
且这些节点的 `size() == measured_size()`。也就是说：**布局周期内发生过尺寸写入，
但在周期结束时两边数值已经一致** —— 脏标记是布局自己留下的，不是外部改动留下的。

## 2. 根因：measure 与 arrange 对同一个子项给出不同尺寸

`NanNode2D::mark_dirty()` 在标到 layout 相关脏位时，会**沿 `parent()` 链把所有祖先一起标脏**
（`scene/node2d.cpp`）。而 `NanControl::layout_to()` 是**先清自己的脏标记，再执行 `on_layout()`**：

```cpp
auto NanControl::layout_to(foundation::NanRect rect) -> void {
    clear_dirty(layout_dirty_flags);   // 先清
    set_position(rect.get_top_left());
    set_size(rect.get_size());
    on_layout();                       // 子节点在这里通过 set_size() 反标父级与全部祖先
    clear_dirty(DirtyFlags::paint);
}
```

于是只要某个子项在父级清完标记**之后**再写一次尺寸，祖先链就会带着脏标记结束本帧，
下一帧继续全量布局，而每个文本节点都会在这一次布局里真的走一遍 HarfBuzz。
三个独立的尺寸不一致各自都能触发这个循环：

| 机制 | 现象 | 位置 |
| --- | --- | --- |
| **A. `Text` 写入整形结果的原始尺寸** | 紧约束下落 `wrap` 时，`size_` 记录的是最长行的内容宽度、`measured_size_` 记录的是约束后的可用宽度；arrange 再把 `size_` 写回可用宽度 | `widget/primitives/text.cpp` `update_metrics` |
| **B. `ScrollView` 两个阶段用不同的跨轴契约** | `on_measure` 只给上界（`min = 0`），`on_layout` 给紧约束（`min = max = width()`）。内容里带 `max_width` 的节点因此在「自己的上限生效」与「被父级 min 拉宽」之间每个周期跳变 | `widget/scroll_view.cpp` |
| **C. `stretch` 越过子项自己声明的交叉轴尺寸** | `stretched_extent()` 对 `stretch` 直接返回 `available_cross`，把子项的 `percent`/固定宽度丢掉；arrange 宽度与 measure 宽度不一致 | `widget/layout.cpp` `layout_linear_items` / `Wrap` |
| **D. 亚像素浮点噪声被当成几何变化** | measure 与 arrange 用不同运算顺序算同一个尺寸，实测差 1.5e-5（`153.599991` ↔ `153.600006`）；按精确相等判断时每一次翻转都标脏 | `scene/control.cpp` `set_size` |

A 与同层既有写法不一致，这是它最直接的证据：

```cpp
// primitives/editable_text.cpp —— 先收敛再写入
const auto measured = text_.measure_layout(constraints);
set_size(measured);
return measured;

// text_field.cpp —— 同上
const auto measured = constraints.constrain(foundation::NanSize(natural_width, height_));
set_size(measured);
return measured;
```

即：**节点尺寸必须等于本次 measure 契约报告的值**。`Text` 原本是这条约定的例外。

C 与 CSS / Godot 的既有语义相反：被 stretch 的子项仍受自己 `max-width`/`width` 的约束，
子项显式声明的尺寸优先于父级的"填满"意图。项目内的证据是 showcase 自己：
Label 页的 "Wrapping" 一节标注 `Assigned width: 68% of the content column`，而在修复前
那一段的百分比**完全不生效**（被 stretch 拉满），是一处静默失效。

## 3. 归属层结论

| 事实 | 归属 | 含义 |
| --- | --- | --- |
| 「测量结果必须收敛到本次约束内再写入节点尺寸」是 measure 契约 | `scene::NanControl` + 各图元的 `on_measure` | `Text` 按同层 `EditableText` / `TextField` 的既有写法对齐，不新增机制 |
| 「同一容器/滚动容器在 measure 与 arrange 必须使用同一份跨轴契约」 | 各容器自己的两个阶段 | `ScrollView` 的两处必须同源；容器不得在 arrange 阶段改变 measure 已经确定的宽 |
| 「子项显式声明的交叉轴尺寸优先于父级 stretch」 | `widget::layout` 的线性排列 | 只认 `LogicalLength` / `PercentLength`；`fill` 由父级分配、`content` 由内容决定，两者保持原语义 |
| 「亚像素差异不算几何变化」 | `scene::NanControl::set_size` | 阈值只影响是否标脏，不影响尺寸取值 |
| 「相同排版输入不重复整形」 | `text::ITextLayoutBackend` 的装饰器 | **本轮未实现**，见 §6 的实测结论 |

不引入第二套几何：本文不改 `measured_size_` 与 `size_` 的字段含义，也不新增布局阶段。

## 4. 契约

### 4.1 测量收敛（图元侧）

1. `on_measure(constraints)` 的返回值必须已经落在 `constraints` 内；写入自身尺寸时使用**同一个值**。
2. 原始内容尺寸（例如文本未换行时的行宽）只能作为查询结果暴露
   （`Text::measured_text_width()` / `layout_result()`），不得写入节点尺寸。
3. 测量不得有绘制副作用，也不得依赖上一次测量的残留。
4. 小于 `scene::layout_size_epsilon`（1e-3 逻辑像素）的尺寸差异按「没有变化」处理：
   尺寸值照常精确更新，但不得据此标脏祖先链。

### 4.2 周期收敛（容器与场景树侧）

1. `layout_root(viewport)` 返回后，凡是在本次调用中被 `layout_to()` 摆放、且其 `size()`
   仍等于该次摆放所指派的尺寸的节点，必须不再处于 `layout_dirty()`。
2. 从构造期遗留的 loose 状态落到真实视口时，允许有限轮收敛（实测 ≤ 2 轮）；
   此后**连续多次**相同参数的 `layout_root()` 必须返回 `0`。
3. 外部改动（setter、响应式赋值、树改动）仍然照常向上失效，不受本条影响 ——
   本条只回收「布局周期自己制造的」脏标记。
4. 若某个节点在周期内被摆放后尺寸又真的变了（超过 §4.1.4 的阈值），它保持脏，
   下一次调用继续布局；这就是需要第二次布局时的唯一合法出口。
5. 容器不得在 arrange 阶段给子项一个与 measure 阶段不同的**交叉轴**尺寸；
   两个阶段必须由同一份基准推出。

## 5. 诊断

- 收敛失败时不允许静默：`layout_root()` 的返回值本身就是「跑了几轮」的可观察量，
  连续多帧非 0 即为缺陷。测试直接断言这个量，不依赖计时。
- 图元测得的尺寸与报告尺寸不一致时，属于 §4.1 违约，应在图元层修，不要在场景树层兜。
- 不引入「布局最大轮数」之类的静默截断：宁可继续布局，也不要悄悄交付残缺几何。

## 6. 已验证的故障注入与未覆盖的缺口

`tests/layout_convergence_tests.cpp`（suite `unit`）逐条钉住上面的机制。
注入方式与结果（"改坏哪一行会红"）：

| 注入 | 结果 |
| --- | --- |
| 把 `Text::update_metrics` 还原成 `set_size(layout_.size)` | 6 个用例中 5 个失败 |
| 把 `layout_linear_items` 的 stretch 判断还原成"只看 alignment" | 6 个用例中 4 个失败 |
| 把 `set_size` 的亚像素阈值还原成 `> 0.0F` | 亚像素用例失败 |
| 把 `ScrollView::on_measure` 还原成只给上界 | **全部用例仍然通过** |

最后一条是**已知缺口，不是已覆盖**：上面的最小场景在两个阶段得到的几何恰好相同，
因此复现不出机制 B。机制 B 的必要性由真窗口基线复现证明：

```bash
# 还原 ScrollView::on_measure 的跨轴契约后
./buildDir/playground/label_bench label
#   back-to-back layout passes: 1, 1, 1
#   layout: mean=68.8 ms   passes_per_frame=1.000
# 保留该契约
#   back-to-back layout passes: 0, 0, 0
#   layout: mean=0.002 ms  passes_per_frame=0.000
```

即缺这条契约会让真实页面回到 68 ms/帧。后续应补一个能复现机制 B 的 headless 用例，
或把这条探测升级为可在 CI 运行的确定性测试；在那之前，机制 B 的回归保护来自人工/基线探测。

## 7. 开放项

1. **`max_width` 会被父级紧约束压过。** `constrained_axis()` 里 `minimum = max(parent_min, own_min)`、
   `maximum = max(minimum, min(parent_max, own_max))`，于是父级的紧 `min` 可以越过子项自己的 `max`。
   当前 showcase 的 `.max_width(chrome.content_max)` 因此**不生效**（内容列仍按视口宽度排）；
   测试里有一条断言记录了这个现状。要不要改成「子项自己的上限胜出」（CSS / Godot 的语义）
   属于可见行为变更，需要单独定案后再改 reference 与断言。
2. **浮点噪声的根因没有消除。** 阈值只是让它不再引发无限重排；measure 与 arrange
   为什么会对同一个量算出 1.5e-5 的差异，尚未定位到具体算式。
3. **整形本身很贵：约 2 µs/字符。** 实测（`playground/bench/label_bench.cpp` 的整形探针，
   确定性输入、强制缓存失配）：

   | 文本长度 | `clip` | `wrap` |
   | --- | --- | --- |
   | 8 | 13.3 µs | 12.9 µs |
   | 32 | 37.3 µs | 37.1 µs |
   | 96 | 142.2 µs | 142.0 µs |
   | 200 | 413.6 µs | 792.6 µs |

   固定开销约 13 µs，之后近似线性。这解释了为什么一次完整布局仍要约 65 ms
   （66 个标签 × 每个被测量 2~3 次）：**布局频率已经被消除，单次布局的成本还没有。**

   本轮**试过并否决**了「整形结果缓存」：按完整排版输入做键、带 LRU 上限的装饰器实测
   命中率极低（600 次强制重排里 hits=3333 / misses=71433），原因是同一个节点在 measure 与
   arrange 拿到的紧约束高度不同、键几乎不可能重复；而失配路径要多付约 750 µs
   （键串拷贝 + 结果复制 + LRU 扫描），整体是**负收益**。下一步应在 `HarfBuzzTextLayoutBackend`
   内部找那 2 µs/字符的来源（候选：逐码点 `FT_Get_Char_Index` 的字体选择扫描、
   整段 bidi、按行重复整形），而不是在外面套缓存。

## 8. 与 D6 的关系

[D6 §9](authoring_dsl_and_content_measurement.md#9-核心前置有限宽度下文本内容自然长高) 已经列出目标契约
（「measure 可重复、无绘制副作用」「整形布局缓存只在会改变行断点/字形度量的输入改变时失效」，
「measure 与 arrange 使用同一份确定的尺寸/约束」）。本文是它的一条实施前置：
先把「一次周期必须收敛」落地并加门禁，再谈两阶段 width-for-height 与 `Section` 组合语法。
本文不改变 D6 的待审阅状态。

## 9. 未纳入本次

- 局部重绘 / 脏矩形（Slint `DirtyRegion` 形状）：本文只保证布局收敛，绘制仍是全量遍历。
- 绘制遍历的 z 序缓存与裁剪剔除：见 [跨平台后端探测](cross_platform_backend_probe.md) 的记录。
- 无序绘制列表与后端无关的批处理：属于渲染后端迁移的独立工作线。
- 两阶段 width-for-height 协议：D6 定案后再定形。
