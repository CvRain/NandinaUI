# 006 · 那个不可能触发的动画

> 2026 年 10 月，紧接 [005](005-the-animation-system-that-was-never-plugged-in.md)。
> 起因只是"看一眼新写的侧边栏和主页"，本意是复查，结果在新代码里找到两个**互相掩盖**的
> bug：一个让功能完全失效，另一个藏在它后面等着被"修好"。
> 状态：两个都修了，补了一条能抓到第一个的接线测试，并用坏形态验证过它真的会红。

## 一、症状：动画不触发

新侧边栏的诉求是"悬浮/按下时圆角外扩"。读代码时看到的是：

```cpp
// showcase/components/sidebar.cpp（修复前）
auto Sidebar::generate_nav_item_builder(title, key) const -> auto {
    auto& hovered = ui.signal_value(false);   // ← 内层这一对
    auto& pressed = ui.signal_value(false);
    ...
    .on_hover_changed([&hovered](const bool value) { hovered.set(value); })   // 写它
}

auto Sidebar::generate_navigation_buttons() const -> auto {
    auto& hovered = ui.signal_value(false);   // ← 外层这一对
    auto& pressed = ui.signal_value(false);
    auto button = generate_nav_item_builder(...);   // 内部又建了一对
    auto& radius = ui.computed([&] {
        if (hovered.get())    { gain += k_hover_radius_gain; }   // 读外层那对
        ...
    });
    ui.bind(button, widget::visual::container.radius, radius);
}
```

同一个概念有**两对信号**：交互回调写内层，动画读外层。外层从来没有被 `set` 过，所以
`computed` 永不失效，圆角永远停在 `base_radius`。

**动画不可能触发。** 不是"效果不明显"，是一行都没跑到。

## 二、藏在它后面的第二个 bug

如果只有上面这一条，修起来就是"把信号传进去"。但那个 `computed` 的捕获列表是 `[&]`：

```cpp
auto& radius = ui.computed([&] { ... base_radius + gain; });
```

- `base_radius` 是**循环体内的局部量**（`auto button = ...` 下面那几行），每轮迭代结束就死；
- `k_*_radius_gain` 是**成员**，读成员等于隐式捕获 `this` —— 而 `Sidebar` 是
  `main_window.cpp` 里 `set_shell` 工厂中的**临时对象**，工厂一返回就销毁；
- 而 `ui.computed(...)` 注册在**外壳作用域**里，会在依赖变化时被**重新求值** ——
  也就是在两者都死后。

所以顺序是：**因为信号接错了，这段代码从来没有被求值过；一旦有人把接线修对，它立刻开始
读悬垂内存。** 两个 bug 互相掩盖 —— 只修一个，得到的是 UB，不是能用的动画。

这类组合是我这次最想记下来的东西：**失效的功能会顺手掩盖它周围的悬垂访问**，因为那些
访问同样"没机会跑到"。修 bug 之前先问一句"它为什么至今没崩"，往往能捡到第二个。

## 三、为什么测试全绿

当时的测试面看上去是覆盖了的：

- `tests/animation_tests.cpp` 新增了过渡测试（behavior 路径的完整过渡、0 时长瞬时吸附）；
- `tests/interaction_area_tests.cpp` 新增了 `GestureArea` 的复合手势测试。

但前者测的是**插值原语**，后者测的是 `GestureArea` —— **没有一条碰侧边栏的接线**。

更值得记的是：那条过渡测试**本身写对了**。它的捕获列表是

```cpp
auto& radius = ui.computed([&hovered, base] { ... });   // base 按值
```

—— 正是侧边栏缺的那个写法。所以局面是：**测试里编码了正确写法，产品代码偏离了它，
而没有任何东西检查这个偏离。** 测试写对不等于产品写对，这两件事之间没有自动联系。

## 四、能抓到它的那条断言

补的测试走完整条链：真实悬浮事件 → `Button::on_hover_changed` → 信号 → `computed` →
绑定的视觉属性 → 解析出的样式值。关键的一句是中间那个看似多余的断言：

```cpp
tree.dispatch_mouse_move(scene::MouseMoveEvent {button->global_bounds().get_center(), {}});
REQUIRE(hovered.get());                  // ← 就是它
REQUIRE(*radius_now() == Catch::Approx(base));   // 目标变了、当前值还在基值 = 过渡在走
```

`hovered.get()` 之所以是锚点：**回调只要写到了别的信号上，它立刻失败。** 前面几步（事件
派发、属性绑定、插值）都不会失败 —— 失效的只有"写错了对象"这一件事。

**并且我用坏形态验证过它真的会红**：把回调改成写一个 `decoy` 信号（复现侧边栏的形态），
测试在 `hovered.get()` 那一行失败（4/5 通过）；还原后 9 条断言全过。没有这一步，我只是
"加了一条通过了的测试"，并不知道它有没有用。

## 五、同一份代码里的另一处：005 §6 的预言落地

`home_page.cpp` 把两个写死的十六进制颜色绑到了 `visual::label.color` 上：

```cpp
inline constexpr std::uint32_t kComponentColor = 0x5c5f77;       // 亮色主题的值
inline constexpr std::uint32_t kComponentHoverColor = 0x4c4f69;
...
.bind(widget::visual::label.color, hovered_color)
```

这正是 [005 §6](005-the-animation-system-that-was-never-plugged-in.md) 说它**主动放弃**的方案 ——
而放弃它的地方（侧边栏）守住了约定：颜色全交给配方，自己只拥有 `radius`。

`home_page` 没守住，后果是 005 §6 逐字预言的：驱动颜色的是只依赖 `is_hover` 的 `computed`，
**"主题变了"这件事进不了响应式依赖**，何况 `PropertyEndpoint` 一旦有值就永久遮罩配方字段，
连 `Label::on_theme_changed` 都救不回来。所以一旦补上明暗切换，这一页会停在亮色。

目前还不是用户可见的 bug —— showcase 里还没有外观切换入口。但它是"补上那一天就炸"的地雷，
而且埋在**第一页**。

修法是让步而不是绕过：颜色改走语义角色 `color_token()`（Label 会在主题变更时重解析 token），
悬浮反馈只保留**与外观无关**的字号。保留颜色悬浮的出路只有一个 —— 补上"主题版本信号"，
也就是 005 排序里的第 6 条。**这条发现因此把 6 的优先级提高了**：它不是缺个便利，是缺个前置，
没有它，任何"实例级样式定制"都只能在"过期"和"写死"之间选。

## 六、比测试更强的那个修法

测试能抓住这个 bug，但更好的做法是让它**不可表达**：把两个信号作为参数传进构建函数。

```cpp
auto generate_nav_item_builder(
    const std::string& item_title,
    const app::PageKey& page_key,
    reactive::Signal<bool>& hovered,
    reactive::Signal<bool>& pressed
) const -> auto;
```

这样"在这个函数内部另建一对信号"就写不出来了 —— 函数的职责被收窄成"把按钮接到这两个
信号上"。测试是补网，签名是堵路；能堵路的时候不要只补网。

同理，捕获列表也改成显式的按值捕获：

```cpp
[&hovered, &pressed, base_radius,
 hover_gain = k_hover_radius_gain, pressed_gain = k_pressed_radius_gain]
```

`[&]` 在这个位置是危险的，因为注册出去的 lambda 生命周期**长于**创建它的作用域，而 `[&]`
让"捕获了一个不能活那么久的东西"看起来和捕获一个作用域持有的信号一模一样。

## 七、给下一个人的话

- **功能失效的地方，往往同时藏着还没被触发的 UB。** 修之前先问"它为什么至今没崩"。
- **测了原语不等于测了接线。** 能跑通的插值、能解析的样式，和"回调写的是不是动画读的那个"
  是三件不同的事，只有最后一件能让功能真的动起来。
- **测试写对了而产品偏离了，是一种没有任何机制的失败模式。** 要么让偏离不可表达（改签名），
  要么让测试断言那个"中间变量确实变了"（`REQUIRE(hovered.get())`）。
- 加完回归测试，**用坏形态跑一遍确认它会红**。否则你只是加了一条绿色的测试。
- `[&]` 在"注册到更长生命周期"的 lambda 里应该被当成可疑写法审一遍；需要按值就写出来，
  别让捕获的代价藏在 `[&]` 后面。
- 定下"不写原始色"这类约定时，要同时想清楚**有没有合法替代**。如果框架缺一个前置、
  作者只能写死，那约定就会在最显眼的地方被破 —— 破在 showcase 第一页，比破在任何地方都糟。
