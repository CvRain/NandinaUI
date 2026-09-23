# 新增一个组件

本文补的是**实现机制**：新增组件要动哪些文件、按什么顺序。质量标准（测试门槛、语义要求、
主题覆盖优先级）在 [组件公共契约](component_contract.md)，两者配合使用。

## 先判断：这个组件需要配方吗

这是最容易走错的一步。配方（recipe）让主题作者与实例覆盖能调整组件的**造型**字段。它带来
一笔固定开销：**字段必须在四个地方同步**（见下），漏一处不会报错，只会让那个字段**静默地
无法被主题覆盖**。

所以先问：这个组件有需要被主题调整的造型吗？

| 情况 | 做法 | 例子 |
| --- | --- | --- |
| 只用现成语义角色，无可调造型 | **不要建配方**，直接引用 `ColorToken` / `ScalarToken` | 纯文本、纯图标占位 |
| 有填充/边框/圆角/度量需要被主题或实例调整 | 建配方，走下面的四步 | Button、Card、ProgressBar |
| 造型是"结构"而非"样式"（例如进度条轨道 vs 填充的相对位置） | 配方里放 `BoxStyle` 组合，而不是平铺标量 | ProgressBar、Tabs |

**没有配方也能满足契约**：契约要求的是语义状态与主题**可覆盖性**，而不是必须有 `XRecipe`。
仓库里 `Image` 就没有配方。不确定时，先做无配方版本；真的接到"主题作者想调但调不了"的
需求时再补配方，那时字段清单也更明确。

## 有配方时的四步同步

以一个 `Foo` 组件为例，**四处缺一不可**：

| # | 文件 | 加什么 |
| --- | --- | --- |
| 1 | `nandina/theme/visual_state.hpp` | `enum class FooVisualState`（normal / hovered / pressed / focused / disabled 中适用的） |
| 2 | `nandina/theme/design_system.hpp` | `FooRecipe`（具体值，`BoxStyle` / `TypeStyle` / `ControlMetrics`）＋ `FooRecipeRule`（**每个字段都是 `std::optional`**）＋ `ResolvedFooStyle`（解析后的具体 `NanColor` / `float`）＋ `FooRecipes`（`base` + `rules`）＋ `DesignSystem::components` 成员 ＋ `resolve_foo` 声明 |
| 3 | `nandina/theme/design_system.cpp` | `apply_rule(…, FooRecipeRule&, …)` 重载 ＋ `resolve_foo()`（base → 过滤后的规则按序覆盖 → disabled 变换）＋ `default_foo_recipe()` ＋ 在默认 `DesignSystem` 里注册 |
| 4 | `nandina/theme/design_system.cpp`（默认值） | 默认值**只用语义角色**：`ThemeColor::token(ColorToken::…)` / `ThemeScalar::token(ScalarToken::…)`。确需字面量时写注释说明为什么现有 token 不够 |

`apply_rule` 的重载最容易漏 —— 漏了它，`FooRecipeRule` 里的字段会在类型上存在、在行为上
完全不生效。写完请用一次"改字段 → 断言解析结果变化"的测试确认（见下）。

## 然后：组件本身的挂载点

| 文件 | 做什么 |
| --- | --- |
| `nandina/widget/foo.hpp` / `.cpp` | 组件实现。照一个同类的现有组件写（展示类看 `progress_bar` / `label`，交互类看 `button` / `checkbox`） |
| `nandina/meson.build` | 在 `module_widget` 的 `files()` 里登记头文件与源文件 |
| `nandina/widget/builtin_component_traits.hpp` | `ComponentTraits<Foo>`：让 `ui.make<Foo>(…)` 可用；需要绑定时一并给出 reactive 入口 |
| `tests/foo_tests.cpp` + `tests/meson.build` | 测试目标与 `test('foo', foo_tests, suite: 'unit')` 登记；**必须带 `suite`**，见下 |
| `docs/components/foo.md` + `docs/components/README.md` | 使用参考；索引表里加一行并写明状态（`可用` / `实验性`） |
| `docs/references/component_roadmap.md` | 若该组件属于某个阶段，更新阶段进度 |

## 组件实现要点

- **主题解析**：持有 `std::shared_ptr<const theme::DesignSystem>` 快照，在
  `on_theme_changed()` 里重解析；`resolved_style()` 返回解析后的具体值给 painter。
  不要缓存 `NanColor` 字面量。
- **实例覆盖**：`set_override(FooRecipeRule)` 只覆盖明确指定的字段，系统切换后保留并跟随
  新快照重解析。**不要**给每个颜色/圆角加平铺 setter（契约第 7 节）。
- **绘制**：优先复用 `widget/primitives/` 里的 painter（`BoxPainter` / `FocusRingPainter` /
  `RipplePainter` …）。`on_draw` 里不要直接写坐标魔法数。
- **尺寸**：`on_measure()` 返回测量尺寸；无界约束下的行为要明确定义（看同类组件怎么处理）。
- **语义**：`semantics_properties()` 至少给出 role 与 label；交互组件还要有 action。
  新增交互组件请对照契约第 6 节。

## 测试门槛

契约第 10 节列了完整清单。新增组件时至少覆盖：

1. 默认构造、`on_measure` 与布局落位；
2. 交互主路径（鼠标 + 键盘）——非交互组件跳过键盘；
3. 适用状态（disabled / read-only / invalid）；
4. **主题切换与 `set_override` 都能改变解析结果**（这一条同时守住配方四步同步）；
5. 语义属性与语义 action；
6. 从场景树移除时的清理（如果持有资源或注册了回调）；
7. 边界值：0、负数、空字符串、超长文本。

## 测试 suite：新组件自动进入 sanitizer 覆盖

`tests/meson.build` 里每个 `test()` 都必须声明 `suite`：

| suite | 用途 | 谁跑 |
| --- | --- | --- |
| `unit` | C++ 单测（Catch2） | 常规任务 + ASan/UBSan 任务（`meson test --suite unit`） |
| `integration` | Python CLI / 构建工作流测试（慢） | 常规任务与 sqlite-fallback 任务 |

新增组件一律用 `suite: 'unit'`。裸写 `test('foo', foo_tests)` 不会报错，但该测试会**静默失去
ASan/UBSan 覆盖** —— 这正是本项目曾经让 21 个组件测试脱离 sanitizer 一年的原因。CI 的
`Assert every test declares a suite` 步骤会在构建前拦下漏声明的测试。

## 常见坑

- **`apply_rule` 重载漏写** → 字段看起来存在，但主题改不动（静默失效）。
- **在组件里写 `NanColor::from_hex(...)`** → 主题切换后不跟随，暗色下大概率不达标。
  （契约与设计系统 skill 都把这条列为铁律。）
- **`std::optional` 字段忘写** → 一旦给了默认值，`set_override` 就变成"总是覆盖"，规则层
  无法表达"只覆盖这一项"。
- **改了配方默认值但没同步文档** → `docs/components/*.md` 里列出的字段表要跟着改。
- **忘记 `meson.build` 登记** → 头文件能被包含（因为不是安装式的），但 `.cpp` 不会编译，
  症状是链接期 `undefined reference`。
- **测试里跨 dispatch 持有 `layout_result()` 的引用** → 引用指向控件内部存储；一次
  `dispatch_mouse_button` 会触发重新布局并 move-assign 掉 `lines`，之后再读就是
  heap-use-after-free（`text_area_tests` 曾如此）。要跨 dispatch 使用就先**取副本**。
