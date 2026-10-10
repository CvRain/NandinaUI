# 主题变化发不出去：一个只能写的开关

> 状态：待决，未动代码。决定见 [待决设计草案](../docs/references/reference_draft.md)。

给侧边栏 footer 做亮/暗切换时，我写了一个看起来很干净的开关：

```cpp
const bool dark_now = themes.appearance() == theme::ColorAppearance::dark;   // 构建时读一次
auto appearance_switch = ui.make<widget::Switch>("暗色模式", dark_now)
    .on_change([manager](const bool dark) {
        manager->set_preference(dark ? ThemePreference::dark : ThemePreference::light);
    });
```

写完就发现它是**单向**的：开关能写偏好，但外观从别处变化时它不会跟着更新。我在代码注释里
把它记成了已知边界，但这不是这个开关的问题 —— 是 `ThemeManager` 没给外壳任何可订阅的口子。

## 一、现状：有 revision，有观察者，但没有面向应用的信号

`ThemeManager` 的公开面里与"变化"有关的只有：

- `revision()` —— 一个单调递增的计数，只能轮询；
- `add_observer` / `remove_observer`（`ThemeObserver`）—— 但它是**给 `SceneTree` 用的**：
  树收到 `on_theme_revision_changed` 后向挂载节点广播 `on_theme_changed`。

`ShellContext` 把 `ThemeManager&` 交给了外壳，但外壳拿不到"变化"。于是任何"外壳要对外观变化
做出反应"的需求都会撞同一堵墙：切明暗后需要换图标、需要更新开关的勾选、需要显示"跟随系统"
的当前解析结果 —— 都做不到，只能等外壳重建。

## 二、试过的验证方式（以及它为什么不够）

为了确认初始状态至少是对的，我临时在构建外壳之前把偏好设成 dark，再截图：开关确实是勾选的。
后来我又把 `checked` 硬编码成 `true` 与 `false` 各跑一次，两张图逐像素比较相差 359 像素 ——
说明勾选状态**确实**会反映到绘制上，我最初"开关没勾上"的判断是看错了图。

这组实验说明的是：**构建时读取是对的，缺的只是构建之后的更新通道**。所以这不是要修一个 bug，
而是要补一个能力。

## 三、为什么不在外壳里自己绕

可以自己搞一个 `ThemeObserver` 注册进去，把 `revision()` 转成一个 Signal。但那是把框架该做的
事下推给每个消费者：谁需要响应外观变化，谁就得写一遍观察者注册/注销/生命周期。而且
`revision()` 的语义是"快照换了"（可能只是 tokens 变了），而外壳真正想知道的通常是
**解析后的 `ColorAppearance` 变了没有**。

## 四、待决的几点

1. 暴露 `Signal<ColorAppearance>`，还是让 `ThemeObserver` 也能给应用用（或两者都给）；
2. `revision()` 与信号的语义分工：信号是"外观变了"还是"主题变了"；
3. `reduced_motion` 是否走同一条通道（它同样是偏好驱动的解析结果）；
4. 信号与 `SceneTree` 已有的广播如何避免重复通知。

细节写在 [待决设计草案](../docs/references/reference_draft.md)。顺带一句：这个口子补上之后，
story 009 里那个开关的"已知边界"就能删掉 —— 两件事共用同一条通道。
