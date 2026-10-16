# 把生产后端换成 SDL3：三条宿主契约，两个自己写出来的 bug

> 2026-10-16，D4 后端替换落地。**P3 已完成**：装了 SDL3 的构建默认走 SDL3，真实应用在 SDL
> 宿主下运行并通过三套验证；P1 的多后端 shader 产物与若干 P2 消费者面仍在收尾。
> 正式契约见[平台与后端契约](../docs/references/platform_backend_contract.md)，
> 探测证据见[跨平台后端探测](../docs/references/cross_platform_backend_probe.md)，
> 前情（探测入库与真实页面渲染）见 [story 015](015-the-probe-that-was-not-in-version-control.md)。

## 一、动手前已知的两个代价

探测阶段已经把「`NanWindow` 同时持有窗口、输入、帧驱动与设备」的两个代价写进了契约：

1. **设备不可注入** —— raylib 设备的调用序列无法包装或插桩，跨后端对照只能做像素级，
   做不了逐图元 trace。
2. **平台行为与框架行为混在一起** —— 「宿主必须做什么」只能靠读 575 行实现才知道。

第 2 条在探测里已经咬过人一次：最小宿主漏了 `dispatcher().drain()`，点击回调照常执行、
路由却纹丝不动，现象是「按了没反应」；又漏了 `process/advance_animations`，浮层淡入停在
opacity 0，现象是「对话框已打开但看不见」。所以这次拆分的边界是照着**探测踩过的坑**划的：
平台层只管窗口、输入翻译、呈现、剪贴板、用本窗口上下文创建设备；框架层（场景树、布局、
渲染遍历、主题、路由、浮层）完全不感知平台。

## 二、拆分本身

`IPlatformWindow`（`app/platform_window.hpp`）定义六个方面：生命周期、指标、输入、呈现、
剪贴板、设备工厂。输入用 `scene::` 的事件类型承载（`FrameInput`），所以平台层不需要认识
场景树，宿主也不需要认识平台的事件模型。

`detail/raylib_platform_window` 是从 `NanWindow` **逐项搬出来**的（关闭语义、高 DPI 尺寸、
鼠标增量、按键队列、字符队列、修饰键快照都必须一致）；`detail/sdl_platform_window` 交给了一个
子代理并行写，它同时要解决键码翻译：框架的键码是 GLFW 码（`widget/key_codes.hpp`），SDL 给的
是小写 `'a'`=97，映射到 65 才算对。子代理回来时报告了两件事，一件是我写错的、一件是命名空间
不一致 —— 这正是并行要付出的对账成本。

**我自己移植时写反了一行**：`RaylibPlatformWindow::create()` 把 `FLAG_WINDOW_RESIZABLE` 设在了
`!config.resizable` 上。它是子代理在对照两个实现时发现并报回来的，不是测试抓到的：窗口测试不
检查「不可调整大小的窗口是否真的不可调整」。

## 三、错误报告的第一个 bug 来自 CI 而不是截图

切默认之后 `window-lifecycle` 立刻红了：

```
window screen size must be finite and positive
```

根因：SDL 平台的 `should_close()` 在 `destroy()` 之后仍返回 false（它只跟踪 `SDL_EVENT_QUIT`），
于是主循环继续 `tick()` 一个已销毁的窗口，读到 0×0 尺寸。raylib 那边不会暴露这个问题，因为
`WindowShouldClose()` 在关窗后本来就是真 —— **语义必须对齐的是"关窗之后"这个边界**，而不是
平时的返回值。修了两处：平台在 `!ready_` 时返回 true，`NanWindow::tick()` 也在 `!opened_` 时
直接返回。

这类 bug 值得记一笔：**它只在"宿主循环 + 真实窗口 + 关闭路径"三者同时存在时才出现**，探测
（没有 NanWindow 循环）和单元测试（没有真实窗口）都碰不到。

## 四、切默认才敢确认的三件事

1. **真实应用能跑**：`NANDINA_WINDOW_BACKEND=sdl3 ./buildDir/showcase/nandina_showcase` 打开 SDL
   窗口（`renderer=gpu`、64 MB RSS、约 5% 单核），侧边栏与 Home 页渲染正确。
2. **三套验证在 SDL 为默认后端下全绿** —— `window-lifecycle` 现在走的就是 SDL 宿主，
   这比"探测里画得出来"强得多。
3. **baseline 工具必须钉住后端**：`playground/bench` 是 raylib 基线采集器，还用 raylib 的
   `TakeScreenshot()` 存证。切默认后它必须显式声明 `WindowBackend::raylib`，否则会在 SDL
   窗口上调 raylib 的截图 API。这是"默认值变了，工具要跟着改"的典型。

## 五、当时不知道什么

- 不知道 SDL 的 `should_close()` 语义要在**销毁后**与 raylib 对齐（第三节）；
- 不知道浮层淡入依赖 `advance_animations()`：探测曾把「hover 圆角不动」记成框架问题，
  补上动画阶段后它正常过渡（8 → 8.69 → 9.89 → 10）。**这条更正比原来的观察更值钱**：
  宿主少跑一个阶段，症状会伪装成框架缺陷；
- 不知道 `overlay_count=1` 也可能"什么都看不见"—— no-shell 模式下树根是 RouterOutlet，
  OverlayHost 不在树里，浮层被 present 到一个没人绘制的宿主上；
- 不知道 `SDL_shadercross` 在这台机器上要额外拉 DXC 的**嵌套子模块**（DirectX-Headers，
  否则 `d3d12shader.h` 找不到），还要给 vendored SPIRV-Tools 放宽一个 GCC 的
  `-Werror=array-bounds` 误报。

## 六、还没做完的

- **P1 多后端产物**：`SDL_shadercross`（提交 `1ff05bec…`）正在本机构建验证，产物与加载证据
  要写回探测记录；在那之前"DXIL/MSL 可用"仍是未验证；
- **P2 消费者面**：SDL 平台不提供图片解码器（`make_image_decoder()` 返回 nullptr），
  所以 `Image` 在 SDL 上不加载图片 —— 这条差异目前只在契约表里，还没有变成 capability 查询；
  `draw_arc` 已在 SDL 设备里实现，但当前 showcase 没有页面用到 Spinner/ProgressBar，
  所以没有页面级证据；
- **逐图元 trace 设备**：设备现在可注入了，但那个 trace 设备本身还没写；
- **Web**：探测层面已有效验收（能区分失败原因），但 Nandina 应用在 Web 上还完全没开始。
