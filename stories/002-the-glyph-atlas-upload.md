# 002 · 一次页面切换卡顿，和它顺带带出的两个洞

> 2026 年 9 月。相关提交：`78a84b2`、`4b294c5`、`bf9badc`（Page / Router 收口）之后。
> 起因是作者反馈：切到「输入插件」和「加载与空加载」时明显卡一下。

## 一、症状里藏着答案

原话有两句，第二句比第一句重要得多：

> 切换到输入插件和加载与空加载这两个页面的时候，程序明显感觉卡了一下。
> **需要加载过一次后再次加载就不卡了。**

"第一次卡、第二次不卡"是一个很强的指纹：它说明代价是**一次性**的，而且被某种**全局**的东西记住了。
如果卡的是组件构造或页面构建，那第二次进入同样要重建页面（本项目是"每次进入重建、离开销毁"），
就会一样卡——所以这个方向可以直接排除。

作者自己的猜测是"组件加载阻塞了，有缓存后就不卡，类似图片加载"。方向对，但还需要落到具体是哪一层。

## 二、先别猜，插桩

我在字形图集的两条路径上加了计时（`GLYPH_RASTER` / `GLYPH_SYNC`），然后跑 playground 的整轮换页
（`--self-test`，10 次导航）。debug 构建，也就是作者实际在跑的构建：

| 项目 | 次数 | 总耗时 | 单次最大 |
| --- | --- | --- | --- |
| `rasterize_glyph` | 468 | **25.3 ms** | 0.22 ms |
| `update_alpha_texture` | 126 | **2397 ms** | 26.3 ms |

**95% 的时间在图集上传，字形栅格化几乎不要钱。** 单次 26 ms 意味着一次上传就丢掉一两帧，
126 次分散在整轮换页里，正好就是"切过去卡一下"。

顺便修正我自己的第一直觉：我一开始以为 `sync()` 是**每个字形**调一次（那会是 468 次整张上传），
读完 `glyph_run_renderer.cpp` 才发现是**每行**一次——`draw_line()` 先把这一行的字形全部缓存，
再统一 sync。所以是每行一次整张图集重传。

## 三、根因：一行文本一次全图上传

`GlyphAtlasTexture::sync()` 当时长这样：

```cpp
if (uploaded_revision_ == atlas_.revision()) return;
device_.update_alpha_texture(texture_, atlas_.width(), atlas_.height(), atlas_.pixels());
```

而 raylib 后端的实现是：

```cpp
const auto rgba = alpha_to_rgba(alpha);   // 逐像素 push_back 构造一百万个 Color
UpdateTexture(found->second, rgba.data());
```

也就是**每次同步都把整张 1024×1024 图集重新转一遍、整张传到 GPU**。
一次页面首次渲染会引入许多新字形，分布在许多行里，于是每行触发一次全量上传。

这解释了三件事，一件不差：

- **第二次不卡**：图集是窗口级的，字形一旦进去就不再变；重进同一页面零上传。
- **为什么偏偏是那两个页面**：它们首次访问时引入的新字形多。
- **为什么 README 特别惨**：一段长文本是**一个 Label 包成几百行**，几百次全量上传挤在一帧里。
  这是最坏形状，不是巧合。

## 四、修法：只传变的那一块

图集的打包是顺序游标，新写入的字形往往落在一条带上。所以让 `GlyphAtlas` 记住
**自上次同步以来写入区域的并集**（`mark_dirty` 在 `allocate()` 里累计，padding 不算），
`sync()` 只上传这块：

```cpp
if (const auto region = atlas_.consume_dirty_bounds(); region.is_valid()) {
    device_.update_alpha_texture_region(texture_, atlas_.width(), atlas_.height(), region, atlas_.pixels());
}
```

设备接口上加了一个 `update_alpha_texture_region`，**默认实现回退到整张上传**——这样任何设备都
保持正确，只有 raylib 后端需要知道怎么做局部上传。

这里有个坑值得记：`UpdateTextureRec()` 最终走 `glTexSubImage2D`（`subprojects/raylib/src/rtextures.c:4399`
→ `rlgl.h:3606`），它按**紧凑排列**读取这一块。所以不能把整张图集的指针递进去，必须先把子矩形
抽成一个只有该区域的缓冲。我复用了同一个 scratch（`rgba_scratch_`），顺带省掉原来每次上传都分配
一块 vector 的开销。

同一个插桩，同样的整轮换页：

| | 修复前 | 修复后 |
| --- | --- | --- |
| 上传次数 | 126 | 126 |
| 上传总耗时 | **2397 ms** | **5.2 ms** |
| 单次最大 | 26.3 ms | **1.04 ms** |

顺便补了一条我这个改法依赖的不变量：**一张图集只能由一个纹理桥接上传**。脏区域是"取走并清空"的，
两个纹理共享同一张图集时第二个会漏掉那些像素、画出陈旧字形。`GlyphRunRenderer` 现在会拒绝重复绑定。

## 五、作者追问：写满会怎样

修完之后作者问了一句："那你说的图集写满会崩，要不要也修？"

查下去发现原来是这样：

```cpp
if (width > width_ || cursor_y_ + height > height_) {
    throw std::runtime_error("GlyphAtlas is full");
}
```

这个 throw 的路径是 `draw_line` → `cache_glyph` → `allocate`，也就是**在渲染过程中抛**，
而没有任何地方接住它 → 穿出 `tree.render()` → `tick()` → 终止进程。图集默认 1024×1024，
约三四千字形的容量；作者试长 README 时只是卡、没崩，说明还在容量内——但这是运气，不是设计。

这和我在 Page / Router 那轮修的第 1 项是同一类问题：**在没人能接住的地方抛异常**。
（见 [001](001-the-router-detour.md)，以及 `docs/references/page_and_router.md` 第 4 节。）

## 六、我在自己的修复里捅了一个洞

改成"放不下就跳过这个字形、不抛"之后，测试直接 SIGSEGV：

```
[error] GlyphAtlas: glyph 88x106 does not fit a 64x64 atlas; skipping it
SIGSEGV - Segmentation violation signal
```

原因是我只改了 `allocate()` 的返回语义，忘了它的调用方：

```cpp
if (bitmap.width > 0 && bitmap.height > 0) {
    bounds = allocate(bitmap.width, bitmap.height);   // 现在可能返回无效矩形
    const int target_x = static_cast<int>(bounds.get_left());   // 无效矩形 → 0
    const int target_y = static_cast<int>(bounds.get_top());    // → 0
    for (...) {
        pixels_.begin() + (target_y + row) * width_ + target_x    // 把 88×106 写进 64×64
    }
}
```

**原来抛异常时这段是安全的（根本走不到），改成返回无效矩形之后它就成了越界写。**
教训很直白：把一个"抛异常"改成"返回哨兵值"，必须把所有调用点重新看一遍——哨兵值不会被编译器
拦住，而异常会。修法是在拷贝前加 `if (bounds.is_valid())`。

## 七、扩容：为什么现在才划算

跳过字形只是不崩，不是解决。真正的解法是扩容，而**扩容之所以现在才值得做，恰恰是因为上一步**：
以前每次同步都是全量上传，扩容意味着每次上传都更贵；现在按脏矩形传，扩容后的日常代价不变，
只有扩容那一刻要整张重传一次（罕见）。

实现上有一条很好的性质：**图集只加宽/加高、不重排**，所以已有字形的 `pixel_bounds` 全部保持有效，
扩容只需按新行距把旧像素复制过去。于是：

- `GlyphAtlas::grow_to_fit()` 按需翻倍到上限；
- `GlyphAtlasTexture::sync()` 发现尺寸变了就重建纹理并整张上传一次；
- 上限做成构造参数（默认 `kMaxDimension = 4096`），既让应用能收紧，也让"放不下"这条路径
  能用一个小上限便宜地测到。

顺带把缓存的内存估算从"配置尺寸"改成"图集当前像素数"（`FontPipeline::atlas_pixel_count()`）：
扩容之后按配置值估算会低估真实占用，缓存预算就不诚实了。

## 八、写测试时踩到的字体陷阱

测试字体是 harfbuzz 的子集字体 `Inconsolata-Regular.ab.ttf`——**只有 a 和 b 两个字形**。
我一开始用 `W` 当第二个字符，断言莫名其妙地失败：`W` 拿到的是空的 `.notdef`，位图为空，
不写入图集、也不产生脏区域。查了半天才想到看一眼字体文件名。

以后写文本相关测试要挑 `a` / `b`，或者用真正的字体。

## 九、记下来的几件事

1. **"第二次不卡"这种症状，先找被全局记住的东西**，而不是去看每次都要跑的那段代码。
2. **先把猜测变成数字再动手。** 我原本以为是"每个字形一次上传"，插桩之后才知道是每行一次；
   而真正的大头也不是我以为的字形栅格化（25 ms），是 GPU 上传（2397 ms）。
3. **哨兵返回值替代异常时，必须重新检查所有调用点**——这一条是这次唯一一次把自己写崩的地方。
4. **一次修复可以解锁下一次修复。** 脏矩形上传本身是性能优化，但它顺手把"扩容"从"会让每次
   上传都更贵"变成了"只贵一次"，于是扩容才成为一个划算的选择。
5. **在渲染路径上不要抛异常。** 这是本项目第二次栽在同一件事上（第一次是页面构建，
   见 001）；共同点是"调用方早已返回、没人能接住"。
