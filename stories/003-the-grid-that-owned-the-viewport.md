# 003 · 一个 Grid 接管了整棵树的视口

> 2026 年 9 月，紧接 [002](002-the-glyph-atlas-upload.md)。
> 起因是审阅时留下的那条"已知但没查"的失败：playground 自检里 `menus` 页的绘制断言一直红。

## 一、症状与它误导我的地方

探针的原话是：

```
NAV_TEST: PAINT FAILED (menus) row=title shellPaints=0 overpaints=0
                          firstOverpaint= shellPrims=8 totalPrims=15
```

断言本身是"外壳的图元必须与标题行/导航行的包围盒相交"。`shellPaints=0` 说明一行都没相交。
把行和它子节点的几何打出来，是这样的：

```
row(title)  x=22.0  y=22.0  w=302.6 h=33.0
  [0] 标签  pos=(0.0,0.0)   sz=299.6x33.0
  [1] chip  pos=(309.6,2.5) sz=86.0x28.0      ← 右边缘 395.6 > 行的 302.6
```

**子节点跑到父节点框外面去了。** 于是我很自然地认为是"包围盒算错了"——`global_bounds()`
用的是控件自己的 `size_`，也许它返回的是测量宽而不是布局后的宽。

这个方向是错的，而且它很贵：我沿着它改过探针、改过 `Padding`，都没有用。

## 二、数据把方向掰回来

真正有用的是**逐帧**把外壳各层的宽度打出来：

```
f=2 inputs    pad=646.6x613.6 col=602.6 hdr=602.6 ttl=386.9 nav=602.6
f=3 loading   pad=748.0x384.8 col=704.0 hdr=704.0 ttl=414.9 nav=644.0
f=4 content   pad=714.9x171.2 col=670.9 hdr=670.9 ttl=386.9 nav=644.0
f=5 drag      pad=1240.0x940.0 col=1196.0 hdr=1196.0 ttl=386.9 nav=644.0
f=6 feedback  pad=273.9x194.8 col=229.9 hdr=229.9 ttl=229.9 nav=229.9
f=7 menus     pad=346.6x293.2 col=302.6 hdr=302.6 ttl=302.6 nav=302.6
```

两个发现：

1. `pad = col + 44` 永远成立（44 是左右内边距）——**整个外壳是"内容尺寸"，而不是窗口宽度**。
2. 宽度每页都不一样，而且还会来回跳。这不是"某一页特别宽"，这是**尺寸没有收敛**。

继续往上追，在真正接收 viewport 的那一层插桩：

```
TMP layerpass root=0x...370 size=1240.0x940.0 dirty=1 vp=646.6x613.6   ← viewport 是内容尺寸！
TMP pad       0x...370 size=646.6x613.6 content=602.6x569.6
TMP layerpass root=0x...370 size=646.6x613.6 dirty=1 vp=1240.0x940.0   ← 下一帧又回来了
```

**viewport 自己在振荡。** 而 `layout_root` 在生产代码里只有一个调用点（`NanWindow::tick`），
传的是窗口的逻辑尺寸，恒定 1240×940。于是答案只剩一个方向：**有别的地方也在调用它。**

## 三、根因

```
nandina/widget/grid.cpp:125
    (void)get_tree()->layout_root(size());
```

`Grid::relayout()` 拿**自己的尺寸当 viewport**，去重排**整棵树**。

触发点是 `Grid::on_ready()`——Grid 进入场景树时调 `relayout()`。而页面是导航时在
**已经布局过的树里**构建的，所以这条路径每次进页面都会走一遍。

这解释了三件事：

- **为什么只有 `menus` 页失败**：它是 playground 里唯一用 `Grid` 的页面。
- **为什么"父子尺寸互相矛盾"**：内容根被"紧贴"到 grid 那个假 viewport 上，外壳整列塌缩到
  grid 那么宽；但它内部的行是按上一轮（正确的）约束测量的，子节点仍按自然宽度摆放。
  父框是新的、子位置是旧的，两者当然对不上。
- **为什么每帧都在跳**：窗口每帧 `layout_root(1240×940)` 修好，Grid 又把它按自己的尺寸
  压回去，如此反复。而自检的断言跑在 `on_frame`（process 阶段，**先于**当帧布局），
  看到的正好是"被压回去"的那一帧。

## 四、修法与回归测试

容器自查表（`Column` / `Row` / `Flex` / `Wrap` / `Padding` / `Center`）的约定是
**只重排自身**。Grid 是唯一的例外，现在对齐：

```cpp
void Grid::relayout() {
    mark_layout_dirty();
    if (!is_inside_tree()) {
        return;
    }
    (void)measure_layout(scene::LayoutConstraints::loose());
    layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
}
```

回归测试用普通 `NanControl` 当根（**不用 Column**：`Column::add()` 自己也会调 `relayout()`
把自身改成内容尺寸，那是另一个话题，会掩盖要断言的东西），先布局，再往里插一个 Grid
触发 `on_ready`，然后断言根的尺寸仍然是 viewport。用旧代码跑，断言以
`0.0f == Approx(800.0)` 失败——根被压成了 grid 尚未布局时的 0×0。

## 五、几件记下来的事

1. **沿错误方向做过两个"修复"：改探针、给 `Padding` 加"先重测再摆放"。** 两个都没解决问题，最后都撤掉了。
   探针那次尤其值得记：它的做法（把内容隐藏后再重画一遍，用那段图元当作外壳自己的绘制）看起来
   确实可疑，我据此改成"先取行边界、再隐藏内容"。**但改完失败照旧**——说明我怀疑的那个点根本
   不是原因。Grid 修好之后探针一次就绿，它原本就是对的。**我给一个正常工作的断言编了一个说得通
   的缺陷，然后花了时间"修"它。** 这比单纯改错地方更值得警惕。
   `Padding` 那次的问题是真的（它是唯一一个不先用最终约束重测、就直接摆放子节点的容器），但它不是
   这次的根因，所以我把它撤了单独记下来，而不是混进同一次提交、让改动显得"都必要"。
2. **"只有某一页出问题"是极强的线索**，应该第一时间去找那一页独有的东西。我绕了很久才去
   对比页面差异（`menus` 是唯一用 Grid 的页面），而这一步几乎立刻给出答案。
3. **拿不到结论时，去给"接收可疑参数的那一行"插桩，而不是继续读算法。** 前面几轮我一直在
   推理布局算法（测量顺序、`tight` 约束、自查表），每一轮都能自圆其说但都不对；把
   `_layout_layer_stack` 收到的 `viewport_size` 打出来之后，五分钟就定位了。
4. **一个控件绝不该重排整棵树。** 它不只是"多做了一点工作"——它改了全局状态（viewport），
   于是问题的表现形式与它的位置隔着好几层，非常难认。
