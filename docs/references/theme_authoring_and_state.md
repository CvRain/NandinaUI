# D2：主题方案、作者入口与状态订阅

> **状态：首批窄投影已随 `8920f3a`、第 3 步目录与选择已随 `995e6ae`、第 4 步观察
> 通道与作者入口已随 `4746173`、第 4a 步 CPU 合成器与诊断已随 `fef829a`、第 4b 步
> 作者颜色入口已随 `3e2542a`、第 4c 步 foundation 命名颜色字面量已随 `f383201`
> 提交推送。**
> 第 4d 步 TOML 命名颜色字面量复核补修完成，随本批统一提交；
> 真实后端像素读回和注册门槛尚未实施。§14–§19 记录契约、实现和复核；
> 更早小节保留讨论语境。
> 所有标注为伪代码的示例只表达作者体验，不可直接当成现有接口使用。
> 当前实现以 [设计令牌与主题系统](design_tokens.md) 和源码为准；原始问题见
> [D2 工作稿](reference_draft.md) 与 [story 010](../../stories/010-the-theme-signal-that-cannot-be-subscribed.md)。

2026-10-10 审阅补充：建议先交付不含身份的只读窄投影，再扩展独立方案模型；补充匿名快照的
现存身份问题、legacy 入口收敛的定性、持久化标识契约与自动配色诊断。`revision` 不称为内容版本，
`ReadProperty` 不等于可追踪的 Signal，对比度校验也不直接照搬默认主题的所有视觉门槛。

本轮颜色契约审阅已确认：按场景声明背景栈与不透明终点、创作/判定/风格指标分离、阈值比较
不使用格式化后的数值；纯颜色合成器归 foundation，不与 OKLCH 插值合并。当前源码支持
编码值合成作为 raylib 路径的候选配置，最终对应关系仍需真实像素读回证明。本次仅归档，
尚未提取合成器或执行该端到端测试；详细规则见 §6.5–6.8。

## 1. 从“开关不同步”到主题模型

起点是侧边栏的亮暗开关：构建时能读到外观，用户能通过它写偏好，但外观从系统或其他入口
变化后，开关没有方便且受生命周期保护的订阅路径。

只增加一条“亮暗变了”的通知不够。后续讨论引入了三个实际需求：

1. 开发者可以注册主题，不限于内置默认风格、butter 或固定枚举。
2. 新主题不必同时提供亮暗两套方案；用户可以跨家族搭配，或者在多个暗色方案间选择。
3. 主题作者应通过统一入口提供配色与样式，不应逐个改组件；第三方组件能消费主题，
   但这不等于框架已经支持任意第三方配方的集中注册。

因此 D2 需要同时明确“选什么”“如何解析”“如何创作”和“变化通知谁”，而不是先加一个
可写 `Signal<ColorAppearance>`，再让所有后续需求挤进它。
这不要求订阅能力等待全部身份与家族决策：§5.5 给出可先落地、语义稳定的最小只读投影。

### 讨论形成的方向

- 具体主题方案独立存在，家族只是可选分组，不强制一亮一暗。
- 用户选择与当前生效结果分开；跨家族亮暗搭配是一等用例。
- 状态只读可订阅，写入仍经 `ThemeManager`；事件用于副作用，不替代响应式读取。
- 主题作者修改语义色、尺度或配方，组件消费解析结果；不逐个改 widget primitive。
- 第三方组件的主题配方扩展单独设计，不把 D2 扩大为插件系统。

后文的失败策略、派生方式和阶段安排属于建议，须随正文一起审阅；具体待决项集中在 §10。

## 2. 已核实的实现与能力边界

| 当前事实 | 位置 | 对 D2 的含义 |
| --- | --- | --- |
| `DesignSystem` 内嵌 `light` / `dark`，共享 tokens、typography、components | [design_system.hpp](../../nandina/theme/design_system.hpp) | 不是任意数量的独立完整方案；暗色专属尺度或配方不能靠换色板表达 |
| `NanTheme` 是 tokens + 单一 palette 的值对象 | [theme.hpp](../../nandina/theme/theme.hpp) | 现有 `register_theme(NanTheme)` 不是完整第三方设计系统注册接口 |
| 参考色阶生成与直接语义色声明两条作者路径均存在 | [theme.hpp](../../nandina/theme/theme.hpp) | 导入配色不必先填写整套参考色阶 |
| `ThemeFamilyDefinition` 编译出双调色板快照，便利字段中有 `card_rules` | [builtin_themes.hpp](../../nandina/theme/builtin_themes.hpp) | 该便利结构不是所有组件配方的通用作者接口；完整快照仍有更多字段 |
| `ThemeObserver`、`add_observer` / `remove_observer` 已公开 | [theme_manager.hpp](../../nandina/theme/theme_manager.hpp) | 不是“完全不能订阅”，缺的是方便、安全、语义明确的应用侧入口 |
| `SceneTree` 已订阅主题 revision 并通知挂载节点 | [scene_tree.cpp](../../nandina/scene/scene_tree.cpp) | 不能再要求应用监听事件后手动遍历刷新组件 |
| `revision()` 也可能因有效 reduced-motion 改变而发布 | [theme_manager.cpp](../../nandina/theme/theme_manager.cpp) | 不严格等于“快照指针换了”；不能沿用旧稿的简化定义 |
| `apply()` 清空家族并写入 `active_name_ = "application"` | [theme_manager.cpp](../../nandina/theme/theme_manager.cpp) | 匿名来源今天就会暴露像注册标识的名字；§9 单独验收，不只是未来设计问题 |
| legacy 与新 API 共用 `system_` / `effective_`、当前选择和 revision 链路 | [theme_manager.cpp](../../nandina/theme/theme_manager.cpp) | legacy 退出是入口收敛，不是两套独立状态之间的数据迁移 |
| TOML 支持主题、家族、字体等，样式规则目前覆盖 Button / TextField | [style_document.cpp](../../nandina/theme/style_document.cpp) | 已有文件导入入口，但未完整覆盖全部配方 |
| `ComponentRecipes` 是列举内置组件的固定结构 | [design_system.hpp](../../nandina/theme/design_system.hpp) | 第三方配方尚不能作为任意类型直接注册进去 |
| `BuildContext::make<T>()` 支持 traits 或接收构建上下文的构造路径 | [build_context.hpp](../../nandina/widget/build_context.hpp) | 自定义组件构建有入口，但与配方注册是两件事 |
| `Property::as_readonly()` 返回仅有 get/observe 的 `ReadProperty`，内部存裸指针 | [property.hpp](../../nandina/reactive/property.hpp) | 可复用读写能力分离的形状；它不负责依赖追踪或延长源对象寿命 |
| `nan_contrast_ratio()` 按契约忽略 alpha | [contrast.hpp](../../nandina/foundation/contrast.hpp) | 自动校验须先合成到已知底色，不能直接比较半透明色值 |

原工作稿与 story 中“只能写不能订阅”“revision 就是快照换了”的描述保留问题发生时的语境；
本次设计以本节更精确的边界为准，不据此声称旧观察者接口不可用于应用。

## 3. 概念模型：方案、家族、选择、结果

### 3.1 具体主题方案

一个方案可以独立使用，并具有：

- 稳定标识：用于引用、选择和持久化，不以显示名称或对象地址充当身份。
- 显示名称：用于界面，允许本地化，不决定身份。
- 外观分类：亮色或暗色，描述方案本身，不随操作系统状态重新解释。
- 完整的可解析主题内容：语义色、尺度、排版与配方；作者不必手写全部字段，可从基础方案派生。
- 可选家族归属；内容更新需要可观察的版本或等价变更身份。

稳定标识不是框架内置 enum。具体 C++ 类型仍待定，但**持久化文本形态是公开兼容面**，不能
留给每个应用自行拼接。身份 API 开放前须定义：

- 命名空间是否必需、合法字符与分隔符、大小写敏感性和规范化规则；不做未声明的自动大小写折叠。
- 文本 parse/format 的往返规则、无效文本诊断，以及格式演进时如何显式迁移。
- 删除后不得把同一标识分配给语义无关的新方案，避免旧设置恢复到另一个主题。同一方案的
  卸载重装或恢复不应被误判为禁止复用；标识治理与是否保留运行时删除记录仍待定。
- 设置保存模式、方案标识和必要偏好；默认不保存 manager 的 `revision()`。运行期失效标记
  不是跨进程内容版本。如未来支持锁定主题包版本，须定义独立版本及其恢复策略。

直接应用匿名快照时应明确表示“未注册来源”，不能伪造可持久化的注册标识。调试或显示名称
可以非空，但不能占用注册身份字段；测试应断言来源与身份，而不笼统禁止一切非空显示字符串。
现有 `active_name_ = "application"` 正是要退出的混用，不要求先保留它再建立新身份系统。

### 3.2 家族

家族只组织方案，可以没有亮色方案、包含多个暗色方案，反之亦然。例如本次讨论中的目录：

```text
默认主题
  light       亮色
  dark        暗色
个人主题
  midnight    暗色
Catppuccin
  latte       亮色
  frappe      暗色
  macchiato   暗色
  mocha       暗色
```

家族可以提供明确的推荐搭配，但不能按注册顺序自动选一个暗色变体。
“属于哪个家族”也不能决定用户必须从同一家族选出另一种外观。

### 3.3 用户选择

建议用一个选择模型表达两种互斥状态，避免多个独立 setter 留下互相覆盖的模式：

| 模式 | 内容 | 行为 |
| --- | --- | --- |
| 固定具体方案 | 一个方案标识 | 始终使用该方案，不跟随系统外观 |
| 按外观选择 | 亮色方案、暗色方案、`system/light/dark` 偏好 | 先解析外观，再选择对应方案 |

固定方案的外观来自方案元数据，不是系统当前外观。按外观选择时，两个槽位应匹配各自的
亮暗分类。切换模式时如何保留上次使用的搭配属于设置体验，尚未决定公开存储形状。

推荐默认由应用预置“默认亮色 + 默认暗色”，用户可只替换一个槽位。默认搭配属于应用策略，
不是每个主题包必须附带的内容。

对不完整或无效的按外观配置，建议返回可诊断失败并保持原状态；不静默生成缺失方案、不按
目录顺序补位。若应用希望回退到默认方案，需显式选择该策略。持久化设置中的失效标识同理。

### 3.4 当前生效结果

从已提交选择、宿主系统偏好与主题目录解析出当前方案、外观及有效内容。它是派生结果，
不是第二份可独立修改的事实来源。

动效偏好同样区分用户请求和解析后的 `reduced_motion`，但不与主题家族绑定。
D1 的“始终显示焦点提示”不属于主题状态：可以复用订阅基础设施，不能混进外观偏好。

## 4. 两个实际选择场景

以下均为伪代码。`configure`、方案句柄及快捷 setter 的名称和签名尚未定稿。

### 4.1 自定义暗色，保留默认亮色

```cpp
themes.configure({
    .preference = ThemePreference::system,
    .light = default_light,
    .dark = my_midnight,
});
```

系统进入亮色时使用默认亮色，进入暗色时使用个人暗色；切到显式 dark 偏好后保持个人暗色。
如果只想固定使用个人暗色，直接选择固定方案即可，不要求提供任何亮色配对。

### 4.2 一亮多暗，记住用户选择

```cpp
themes.configure({
    .preference = ThemePreference::system,
    .light = catppuccin_latte,
    .dark = catppuccin_mocha,
});

themes.set_dark_theme(catppuccin_frappe);
```

- 当前为暗色：立即从 Mocha 切到 Frappé，虽然 `appearance` 没变，当前主题仍然变了。
- 当前为亮色：继续显示 Latte，只更新暗色选择；之后进入暗色时使用 Frappé。
- 亮暗来回切换不应把暗色选择重置回家族推荐值。

设置界面可以分别提供“外观模式”“亮色方案”“暗色方案”，家族作为选择器中的分组即可。
二态暗色开关表达当前有效外观：用户操作它可切到显式 light/dark，但不能独自表达“跟随系统”；
后者需要独立入口或三态选择器。

## 5. 状态读取、事件与发布契约

### 5.1 区分观察维度

以下区分不代表必须公开四个独立、可变的 Signal：

| 维度 | 可见性与阶段 | 典型消费者 | 何时需要变化 |
| --- | --- | --- | --- |
| 选择状态 | 目标公开只读；不在首批窄投影内 | 设置界面、持久化 | 模式、偏好、亮暗槽位改变，即使画面没变 |
| 生效状态 | 公开只读；首批仅 §5.5 两字段 | 外观开关、图标、应用逻辑 | 当前方案、外观、内容版本或相关有效偏好改变 |
| 主题目录 | 按需公开只读；动态选择器阶段定形 | 动态主题选择器 | 注册、移除、改名、替换目录内容 |
| 内部呈现失效 | 内部机制，不作为作者可发布的事件开放 | SceneTree 与组件 | 需要重新解析样式或更新相关呈现行为 |

`on_theme_change` 倾向专指当前生效主题的身份、外观或内容变化，用于应用副作用；仅修改未
生效的暗色选择不触发它。保存设置必须观察选择状态，不能只依赖主题变化事件。
reduced-motion-only 变化通过有效动效状态被观察，是否同时包含在更广义的状态事件中仍待
API 定形；不能因事件命名遗漏现有组件更新需求。

| 操作 | 选择变化 | 当前主题变化 | 目录变化 |
| --- | --- | --- | --- |
| 暗色下 Mocha → Frappé | 是 | 是 | 否 |
| 亮色下改暗色槽位 | 是 | 否 | 否 |
| system 且当前暗色 → 显式 dark | 是 | 否 | 否 |
| 跟随系统时系统亮 → 暗 | 否 | 是 | 否 |
| 固定方案时系统外观改变 | 否 | 否；原始系统状态可变 | 否 |
| 同标识替换当前方案内容 | 否 | 是 | 是 |
| 注册或更新未使用方案 | 否 | 否 | 是 |
| 激活无效标识 | 否 | 否；返回失败 | 否 |

同名不等于同内容；不得仅比较 `appearance`、名称或标识就吞掉当前方案内容替换。
重复提交等价状态不应制造无意义的事件；具体内容等价判定方式待定。

### 5.2 只读响应式入口与事件并存

建议作者体验如下，仍为伪代码：

```cpp
auto state = ui.theme_state();
auto dark = computed([state] {
    return state.get().appearance == ColorAppearance::dark;
});

ui.on_theme_change([](const ThemeChange& change) {
    record_theme_change(change.before, change.after);
});
```

目标约束：

- 状态读取能参与绑定或 computed；作者不能通过状态句柄反向写入，写入统一经 manager。
- 事件支持多订阅者，不是后注册覆盖前注册的单 callback 槽位。
- 初始状态通过读取获得，订阅本身不伪造一次“变化”；初始 effect 求值与事件通知分开。
- 原始订阅返回 RAII token；构建上下文便利入口把连接纳入作用域，不让作者重复写注销代码。
- `before/after` 与变更分类必须具有清楚的所有权；不把临时字符串的 `string_view` 留给延迟消费。
- 不向订阅者开放发布能力。现有 `Event::emit()` 是 const，返回 `const Event&` 并不能限制 emit。
  推荐让 Event 保持私有，仅公开 `subscribe(handler)` 或只读视图的 `observe(handler)`，返回 RAII token；
  不为限制 emit 单独再造一套事件包装体系。

上例的 `state` 按值捕获不自动证明生命周期安全；新状态句柄必须有相应寿命契约。
现有非 owning 的只读信号视图不能未经处理就宣称支持跨 manager 销毁使用。

状态视图优先沿用 `Property::as_readonly() -> ReadProperty<T>` 的 get/observe 能力分离形状。
但现有 `Property::get()` 只返回值，`ReadProperty::get()` 只转调它，不像 `Signal::get()` 那样
调用 Graph 的依赖追踪。因此不能直接把 `ReadProperty` 放进上面的 computed 就承诺它会更新。
若保留应用侧 computed/bind 用法，须由受作用域保护的适配器桥接到现有 Signal/ReadSignal，
并用“外部修改 → computed 重新求值”的测试证明通路；不另造一套响应式引擎。

### 5.3 归属与生命周期方向

倾向保持 `ThemeManager` 不依赖应用的 `Graph`，由框架提供受作用域保护的响应式适配器；
不要求每个应用自己写 observer → Signal 桥接。将 Graph 注入 manager 仍是可比较的替代方案，
最终采用哪个应在实施前明确，不在本次汇总中伪装成已落地决定。

理由是**所有权与生命周期**，不是分层禁止：[模块依赖规则](module_dependency.md) 中 reactive
在基础层，theme 在主题层，`theme → reactive` 是合法向下依赖。manager 由 application 持有，
而外壳订阅与 effect 有各自的较短寿命；无 Graph 的 manager 可以由适配器接入所需 Graph，
Graph 注入方案则须证明所有观察关系与源对象都在 Graph 析构前清理。两种方案按这个代价比较。

适配器只是 manager 状态的只读投影，不建立第二个可写模型。必须覆盖：

- UI 作用域先销毁：解绑观察与 effect，后续变化不回调失效对象。
- manager 先销毁：适配器得知失效，连接安全退出，不留下悬垂访问。
- 多个外壳或窗口订阅同一 manager：互不覆盖、互不替对方注销。
- 不捕获短命 `BuildContext&`；服务引用和句柄也必须有明确寿命证明。

### 5.4 原子提交与通知顺序

一次变更应先校验，再形成完整的新选择/解析结果/有效快照，提交后才发布。观察者不能读到
“新外观 + 旧主题内容”或“新外观 + 旧动效偏好”的半更新状态。

建议各投影共享同一已提交状态及事务身份，响应式桥接在受控批次更新；具体批处理方式需结合
现有 reactive 模型验证。不同通道各履其职，不表示一次切换只能通知一个消费者。真正应避免
的是同一 SceneTree 因新旧桥接重复广播、同一 effect 因半更新读到不一致结果。

组件仍走既有内部下发路径，应用事件不承担手工刷新全部组件的职责。
只改选择但生效内容不变，不应为了更新设置 UI 强制组件重新解析样式。

还需区分重复通知与**通知范围过宽**：目前只改有效动效偏好也会发布 revision，SceneTree 随之
向挂载节点广播 `on_theme_changed`。这是现有更新通路，不代表调色板、tokens 或配方内容改变。
是否把内部失效拆成外观/内容/动效等维度列入 §10；首批窄投影可以保留保守广播，不能为了省
重解析而让 reduced-motion 消费者失去更新。后续收窄必须验证动效及时生效与无关样式不重解析。

回调中再次改主题、回调抛异常、通知过程中解绑或销毁对象的策略均须在实施前定形。
不能直接继承现有同步 Event 的行为就认为已解决重入和异常隔离；内部下发与外部回调的
精确先后顺序也需写成可测试契约。

### 5.5 首批可独立交付的窄投影

接受“先补订阅、不等待身份定形”的拆分方向。最小载荷仅包含以下两项，类型名为伪代码：

```cpp
struct ThemePresentationState {
    ColorAppearance appearance;
    std::uint64_t revision;
};
```

- `appearance` 是当前有效外观，不是用户偏好。
- `revision` 是来自 manager 的**不透明呈现修订标记**，不是纯内容版本；包括现有动效变化。
  只在同一 manager 生命周期内用于判断观察值是否变了，不持久化、不推断变化类别或内容相等，
  不假定每次读取一定只增加 1。
- 不放方案身份、家族、用户选择或假注册名；这些字段与通知语义等各自定形后再扩展。
- 保持首批字段语义不变。后续新增字段仍须审查聚合初始化、比较和通知范围的影响，不能把
  “加字段”直接当作任意 C++ 使用方式下都无兼容成本的保证。

此批解决外壳跟随实际外观与呈现修订，不承诺完整设置持久化：system 暗色 → 显式 dark 若
有效外观与 revision 都不变，窄投影不通知；后续选择状态通道补上这类语义。
首批 observe/subscribe 表示“窄投影变了”，不能冒用 §5.1 中更窄的纯主题身份/内容事件语义。

最小载荷并不免除发布点审计：现有 `sync_to_effective()` 的具名路径会调用 `commit()`，后者
在快照指针相同时提前返回；具名同快照下只改 appearance 的路径不能假定已有 revision 通知
一定覆盖。实施前测试 apply、具名 activate、family、系统偏好和动效入口，必要时在生产者端
补齐有效外观变化的发布，而不是用 UI 轮询绕过。订阅也不能凭空修复已有的内容替换路径缺陷。

首批交付仍须满足 §5.2–5.4 的权限、寿命和一致性要求；具体窄接口的重入/异常策略先定，
而不要求把全部身份和第三方配方设计一起完成。

## 6. 主题作者如何导入或制作方案

### 6.1 修改哪一层

Primitive token 是基础设计值，不是 `widget/primitives` 里的 Text、Pressable 等节点。
主题作者可能调整前者，通常不应修改后者代码。

| 目标 | 作者修改 | 不需要做 |
| --- | --- | --- |
| 换配色 | 语义色板，或生成它的参考色阶/策略 | 重写每个组件配方 |
| 改整体密度、圆角、字号 | 相关 tokens / 排版配置 | 遍历每个组件实例 |
| 改按钮、卡片等造型 | 对应类型化配方规则 | 修改交互逻辑 |
| 新增槽位、布局结构、输入行为 | 组件实现 | 把结构变化硬塞进主题 |

当前排版角色的实际消费范围等限制仍以 `design_tokens.md` 为准；有配置类型不等于所有消费者
已经接通。需要区分“已注册”“已生效”“组件已消费”。

### 6.2 推荐作者流程

1. 选择基础方案，继承未修改部分。
2. 提供语义色，或从参考色阶生成语义色。
3. 按需覆盖尺度、排版和组件配方。
4. 校验并注册一个完整的具体方案。
5. 固定使用它，或放进用户的亮暗搭配。

拟议伪代码：

```cpp
auto midnight = derive_theme(default_dark);
midnight.colors = imported_semantic_colors;
midnight.tokens.radius.md = 10.0F;
midnight.components.card.add_rule(soft_card_rule);

auto midnight_id = themes.register_theme(
    "personal.midnight",
    std::move(midnight)
);
themes.set_dark_theme(midnight_id);
```

导入外部配色时，作者仍要决定 background / foreground、primary / primary_foreground、
accent / accent_foreground、border、ring 等角色映射。颜色名字不能保证用途相同，自动映射
不能替代对前景背景、焦点提示与交互状态的检查。

纯配色应是完整主题构建的一条便捷路径，不再建立另一套运行时主题系统。
C++ 与文件导入最终应收敛到同一校验和提交语义；完整 TOML schema 的扩展另行分期，不能
把现有局部解析器说成已经支持全部配方。

### 6.3 派生、快照与解析

第一版建议采用构建时派生、注册时形成不可变快照，不引入运行时继承链：

- 改个人暗色的尺度或配方，不污染默认亮色或默认暗色。
- 后续替换基础方案，不暗中重写已注册的派生方案；重新派生应是显式动作。
- 完整快照不等于提前把所有 token 引用烘焙成组件颜色副本；配方保留语义引用，解析时取值。
- 未覆盖字段从明确的基础方案补齐；缺失必需内容应可诊断，而不是设置成功却没有效果。
- 内部可共享不变数据，但不能重新暴露“两个独立方案实际共享可变配方”的问题。

统一入口不意味着所有变化都只标 paint：颜色、字体、间距与几何的实际影响不同。
切换后须正确重新解析与失效；更精细的重绘/布局优化另行证明，不能沿用 L2 动画的 paint-only 要求。
显式实例覆盖应保持既有优先级，未覆盖字段则继续跟随新主题。

### 6.4 自动配色诊断与注册门槛

采纳“校验必须可自动执行”的方向，不把对比度留到人眼验收。注册/替换前运行校验，对选定
校验档案中的必需规则失败返回诊断，并保持旧目录、当前选择和生效状态不变。直接应用快照
也应有一致的校验策略，不能成为未声明的绕过入口。诊断至少包含：

- 方案标识或匿名来源、外观、规则标识和适用状态；
- 前景/背景角色路径、合成底色、实测比值与所需阈值；
- 每层作者输入的原始 OKLCH（含 alpha）、转换/裁剪后的实际 in-gamut sRGB（含 alpha），
  以及最终参与判定的有效前景/有效背景；注明色域、合成与量化策略，不只输出一个颜色值；
- 严重级别，以及未能静态检查时缺失的上下文。

已有 [contrast.hpp](../../nandina/foundation/contrast.hpp) 可提供计算，
[主题配色测试](../../tests/theme_palette_contrast_tests.cpp) 可提供已知角色对与阈值起点。
但不能原样复制默认主题全部断言，变成所有第三方主题的统一拒绝规则：

1. 计算器忽略 alpha。半透明前景/背景必须逐层合成到已知的不透明底色；背景依赖图片或实例时，
   应报告“上下文不足”，不能拿未合成值宣称通过。合成终点与判定空间按 §6.5–6.6 的已确认规则；
   严格档案如何处理无法验证项须明确。
2. 正文前景、焦点提示、装饰边框、禁用态不是同一用途。先声明要测哪一对角色、哪种状态和
   阈值；不把每种颜色与 background 的比值都当作硬门槛。
3. 现有测试里默认主题与参考色阶家族就有不同要求；不能用主题名称隐式豁免，也不能未经处理
   就提高阈值导致既有家族全部无法注册。门槛与已有例外应显式审阅、诊断并安排修正。
4. 默认风格的表面明度方向和层级差值不是所有设计语言的通用规则。可纳入特定风格检查，
   不与基础可读性门槛混为一谈。
5. 语义色板通过不证明组件配方、实例字面量覆盖后的所有状态都通过；能静态解析的配方应逐步
   纳入检查，其余继续由组件自动测试和真实场景验收覆盖。

建议区分结构错误、已声明必需可读性规则错误、风格建议与上下文不足；前两类阻止提交。
首批默认校验档案、允许的显式例外与严格模式范围在 §10 定形。校验器不擅自改色或静默放行，
失败诊断也不等同于“整套 UI 已通过无障碍认证”。

### 6.5 已确认：合成终点与场景身份

每条校验规则必须声明前景、从近到远的背景栈、状态与明确的底色，不能由不同校验器各自猜测。
例如下列链描述“卡片内弱化区域的文本”，不是 muted 角色在所有组件中的固定父子关系：

```text
muted_foreground → muted → card → background
```

- 默认页面校验以明确不透明的 `background` 为根。透明窗口若依赖宿主桌面，必须另给宿主底色；
  不能因为角色名叫 background 就假定它不透明，也不能暗中补白色或黑色。
- 查找背景栈时遇到完全不透明的一层，可停止解析更底层；再从该底向前景方向逐层合成。
  这条提前终止针对背景栈，不是遇到不透明文字就忽略文字下面的背景。
- 最终底色仍透明且宿主未知，或背景依赖未提供的图片、渐变位置等输入，返回“上下文不足”，
  不宣称通过；严格模式可拒绝该配置，但不能把未知伪报成实测对比度失败。
- 同一角色在页面、卡片、浮层等位置分别声明、校验。注册阶段检查声明的场景，不自动推断
  任意运行时节点的实际背景，也不保证未声明场景通过。

**同一方案在不同实际背景下得出不同结果是合理的；同一规则被不同校验器随意选择背景才是不合理的。**

判定对象固定为：

```text
有效背景 = 按声明场景合成背景栈
有效前景 = 前景颜色覆盖到有效背景后的颜色
对比度   = contrast(有效前景, 有效背景)
```

不能拿文字直接与最底层 background 比较而跳过中间层。组透明度、混合模式与离屏合成不应
未经声明就当作普通逐层 source-over；首批覆盖不了的场景同样报告上下文不足。

### 6.6 已确认：空间分工、精度与诊断

| 环节 | 契约 |
| --- | --- |
| 创作与存储 | 可以继续使用 OKLCH，保留作者值用于诊断 |
| 透明合成 | 空间与 source-over 语义须由校验配置明确指定，并与目标渲染路径核对 |
| 可读性门槛 | 对最终不透明的 sRGB 前景/背景线性化，计算相对亮度与比值；不使用 OKLCH L 的差值代替 |
| 风格建议 | 可使用 OKLCH 明度方向、层级差值等，标明指标与单位，不与可读性阈值混用 |
| 动画插值 | 独立于透明合成；现有颜色插值仍按 OKLCH 及最短色相弧处理 |

**透明合成空间与动画插值空间是两个独立决定，不追求统一。**
[NanColor::mix](../../nandina/foundation/nandina_color.cpp) 插值 OKLCH 的 L/C/H 和 alpha，
[motion::lerp(NanColor)](../../nandina/foundation/motion/tween.hpp) 调用它。它表达动画中间颜色，
不是前景覆盖背景的合成；禁止为了“只保留一个混合函数”将两者替换为同一个算法。

阈值比较使用未格式化的计算值；展示可以四舍五入，判定不可以。以要求 4.5 为例，4.499
不能因为显示为 4.50 就通过；像素核对的量化容差也不能挪来放宽可读性门槛。

色域处理必须成为共享、显式策略。当前
[颜色转换](../../nandina/foundation/nandina_color_space.hpp) 在 OKLab → linear RGB 时裁剪通道，
[SDL3 适配器](../../nandina/render/backends/sdl_device.cpp) 的普通颜色转换还会进入 8-bit
`NanHexRgb`；这说明浮点颜色、量化颜色和屏幕读回不能混为同一个输入。
首版是否沿用当前裁剪与各绘制路径如何纳入量化，需与后端核对后定形；不能为校验器单独换一套
更“理想”的色域映射而不修改渲染契约。

诊断应保留原始 OKLCH，另列转换/裁剪后 sRGB；不能从最终 sRGB 反推 OKLCH 冒充原始输入。
若导入阶段已有规范化，应区分作者输入与规范化存储值。阈值失败应同时给出足够精度的判定值，
避免“显示 4.50 却失败”无法解释。

### 6.7 已确认：唯一合成器与职责归属

**纯颜色合成器归 foundation，与对比度计算同层；主题角色和背景场景的解释归 theme。**
具体头文件名和函数签名在实现前定形，职责不再让各调用者自行选择：

- foundation 提供唯一的 CPU 颜色合成实现，输入是颜色和明确的合成配置，不认识 Card、token
  或 SceneTree。按色彩配置选择算法也集中在这里，不在每个校验器内各写一个 helper。
- theme 根据规则解析角色、检查背景栈与终点，然后调用 foundation；如需背景栈便利函数，
  foundation 只能接受纯颜色序列，不接收组件或主题角色。
- 现有 [theme_palette_contrast_tests.cpp](../../tests/theme_palette_contrast_tests.cpp) 的局部
  `composite_over` 应退出，改为消费共享实现；新校验器与新测试不得再复制生产算法。
- 合成空间不是调用方不可见的实现细节：使用显式参数或明确命名的固定策略，并让诊断和后端
  核对测试记录同一配置。增加新后端不暗改既有策略的语义。

首版可将基础 primitive 限定为“straight-alpha 前景覆盖不透明背景”，与当前测试 helper 的
前提一致；从不透明根向上展开背景栈时，每一步都能满足该前提。不应把该公式直接宣称为
任意透明背景、预乘 alpha 或离屏组透明度的通用合成。若扩展这些输入，必须另外定义输出
alpha 与预乘规则。默认 GPU blend 的 RGB 结果也不自动证明其 alpha 通道符合通用 source-over。

共享生产实现不等于测试只能自证：foundation 单测仍需手算或独立固定的期望值及代数性质，
端到端测试则以实际读回作为另一条证据，不能让期望值和被测路径共享同一个错误。

### 6.8 后端合成空间：源码依据与端到端验收

当前 [sdl_device.cpp](../../nandina/render/backends/sdl_device.cpp) 显式设置 `SDL_BLENDMODE_BLEND`，
普通颜色先量化到 `NanHexRgb`。旧 raylib/rlgl 源码已退役，不能再作为当前后端的依据。
**编码 sRGB 值上的合成仍需按 SDL 实际输出路径验收**，不是因为“线性才正确”
就应被替换的权宜实现。

但单个文件没有设置不等于已经证明所有运行环境、渲染目标与宿主状态；以上记录是源码核对，
不是像素实测。合成空间最终以目标路径读回核对为验收依据，不靠色彩学偏好作决定。

后续主题校验阶段增加真实后端集成测试：

1. 通过实际 RenderDevice 绘制路径，在已知不透明底色上绘制已知半透明纯色块；至少覆盖
   一层与多层背景，并选取能明显区分编码值/线性合成的颜色和 alpha。
2. 刷新绘制批次并读回目标 framebuffer 的像素，采样纯色内部，避开边缘抗锯齿、文字覆盖率、
   阴影与缩放采样。使用明确的读取时机、坐标方向和目标格式，不拿 RecordingDevice 调用
   记录或桌面截图替代真实像素。测试专用读回适配不得绕过被验证的生产绘制路径。
3. 对同一声明输入调用共享合成器，比较读回 RGB。显式记录入口颜色和 alpha 的量化、目标格式
   及容差依据；不能在看到失败后扩大容差。容差必须小到仍能拒绝错误的另一种合成空间。
4. 默认目标与离屏目标如均被宣称支持，就分别测试；不以离屏通过推定窗口目标相同。不透明根
   的 RGB 核对不宣称覆盖透明窗口的最终桌面合成或任意 alpha 通道语义。
5. 无窗口/GPU 环境记录为跳过或未验证，不计为通过；在支持的环境进入 integration 验证。
   D4 更换后端或颜色配置时沿用该测试，失败应触发策略/后端契约复核，而非放宽门槛。

每条核对记录后端、构建配置、渲染目标及声明的色彩配置，避免将一条环境的通过外推成所有
平台通过。**共享 CPU 合成器与诊断已按 §16 落地；真实后端像素读回尚未执行，因此
diag 结论仍不能当作注册门槛已通过。**

## 7. 自定义组件与主题的边界

### 7.1 组合已有组件

账户卡片等业务组合可以通过构建函数或自定义节点组织 Card、Label、Button，暴露数据、槽位
与语义事件。子组件继续消费主题，通常不需要新配方，也不需要修改框架中央主题结构。
traits 可定义在组件自己的包里，不要求加入内置 traits 文件。

### 7.2 独立控件

现有组合无法满足时，再实现状态、输入、测量、布局、绘制、焦点与无障碍；接入构建上下文、
主题通知和生命周期。默认样式优先消费现有语义 token，只有确实需要可覆盖造型时才建配方。
主题变更后可重新解析，不长期缓存脱离主题的颜色字面量。

测试要求见 [组件公共契约](component_contract.md) 与 [新增一个组件](adding_a_component.md)。
后者的中央配方四处同步流程是贡献内置组件的现行做法，不是现成的第三方配方注册协议。

### 7.3 第三方主题覆盖：后续能力，不在本轮实施范围

目标原则：组件提供基于通用语义角色的默认配方；主题可以选择提供组件专用覆盖，不必认识
所有第三方组件。组件也不应该为每一个主题家族写分支。

```text
第三方组件默认配方 → 通用语义角色 → 当前主题解析
```

组件专用主题覆盖应在明确优先级下参与配方解析，而不是要求主题重复定义组件全部样式。
当前固定的 `ComponentRecipes` 尚无此通用注册能力。后续可研究类型化配方键，但必须先解决
默认值、类型安全、覆盖优先级、存储生命周期与 no-RTTI 约束，不能只添加无约束字典。
D2 此次只保留边界，不承诺组件插件、动态库加载、任意自定义 token 或实时继承图。

## 8. 建议交付顺序

以下不是已批准排期，也不自动改变 D1 / D3 / D4 的优先级。审阅完成后，每阶段按
[开发流程](development_loop.md) 先定 API、再实现、证明测试会红、最后运行验证关卡。

1. **窄投影定形**：仅确定 §5.5 的两字段载荷、只读入口、桥接所有权及该批通知契约。
2. **先交付订阅**：审计现有发布点，接入作用域适配、绑定/观察与 showcase 亮暗开关，
   验证生命周期和无重复广播；不等待身份模型，不宣称选择状态与纯主题事件均已完成。
3. **独立方案与选择解析**：定形身份文本与目录、固定/按外观模式、跨家族搭配、失败原子性；
   在同一状态基础上收敛 legacy 入口，补充匿名来源与持久化契约。
4. **扩展状态与统一作者入口**：补充选择/身份/目录的观察能力和明确的主题事件；完成派生、
   结构校验与注册入口，迁移内置主题，明确文件导入范围；内部按维度广播另行验证。

   **4a：独立主题校验子项。** 提取 foundation 合成器、迁移局部 helper、按场景自动诊断，
   完成真实后端像素读回核对后确定合成空间与色域策略，再接通配色注册门槛。该子项依赖作者
   入口，但不阻塞第 1–2 步的窄投影，也不并入第 3 步方案模型；完成前只声明结构校验可用，
   不把注册成功宣传成配色门槛已通过。

   **4b：作者颜色入口。** 类型化 primitive 色阶访问、显式语义角色覆盖与文件导入的一致
   校验/提交；具体 API 与复核记录见 §17，独立于 4a 尚未完成的真实后端验收与注册门槛。

5. **完整应用验收与归档**：更新 showcase 多方案设置，覆盖真实组件切换；再更新教程与状态。

可按风险细分，但不能先公开依赖“一家族两变体”的新 API，再把独立方案当后续补丁。
项目处于快速演进期，不把永久保留旧公开入口作为设计前提；破坏性调整需同步迁移仓库内调用点
并记录迁移方式。当前 legacy 与新 API 已共用状态，退出主要是入口收敛与调用点调整，不是
把两套运行状态合并。新独立方案模型确有数据形状调整，但应与 legacy 退出分开评估；任何
过渡桥接都继续使用这一条状态链路，不再制造第二份可写模型。

## 9. 验收矩阵与故障注入建议

**本节是未来测试要求，不是已运行的结果。** 实施时记录实际改坏位置、失败输出与恢复后的验证。

| 场景 | 必须证明 | 故意改坏何处应失败 |
| --- | --- | --- |
| 首批窄投影：外观与动效 | 外观变化更新开关；仅动效变化也更新 revision，但不谎称内容版本变化 | 只按 appearance 去重整个投影或只监听快照指针 |
| 具名同快照切外观 | appearance 改变仍有通知，computed/绑定实际更新 | 让同指针 commit 短路吞掉外观发布，或省略 Graph 桥接 |
| 匿名快照身份 | 未注册来源不暴露可持久化注册 ID，不能与注册的同名方案混淆 | 把 `application` 或任意字符串填入匿名来源的注册身份字段 |
| 标识持久化 | 文本往返一致；失效标识有诊断，不解析成另一个新方案；不保存运行期 revision | 隐式大小写折叠、将已删除 ID 分配给另一主题或把 revision 当版本存盘 |
| system 暗色 → 固定 dark 偏好 | 设置 UI 更新，当前方案不变，不额外触发主题变化事件 | 只在 appearance 改变时通知选择状态 |
| 同为暗色的 Mocha → Frappé | 当前身份、颜色与事件一起更新 | 只比较 appearance |
| 亮色下修改暗色槽位 | 当前画面不变，设置可保存，下次暗色选对方案 | 只保存当前激活方案 |
| 默认亮色 + 个人暗色 | 跨家族来回切换且选择不丢失 | 强制按当前家族重选 |
| 单独暗色固定使用 | 无亮色方案也可使用，系统变化不影响它 | 注册时强制配对 |
| 无效标识/不匹配槽位 | 返回失败，无部分状态变化、无成功事件 | 校验前修改选择或发布通知 |
| 当前标识内容替换 | 颜色/配方更新，即使 ID 和 appearance 不变 | 按同 ID 提前返回 |
| 非当前目录项变化 | 目录更新，不重解析当前组件 | 注册即强制激活或广播主题变化 |
| 同事务更新系统偏好 | 回调和绑定只读到一致的外观、内容、动效状态 | 在字段更新之间发通知 |
| 订阅与程序同步开关 | 初值正确，外部切换更新，setter 不冒充用户 on_change | 只构建时读取或 setter 回发用户事件 |
| 重复设置与多订阅者 | 等价设置幂等，各消费者恰当收到一次通知 | 新旧桥接同时刷新同一消费者 |
| 生命周期 | 作用域/manager 任一先销毁均安全；解绑后无回调 | 去掉注销或销毁通知处理 |
| 只读权限 | 不能写状态或伪造主题事件 | 暴露可写 Signal 或可 emit 的 Event |
| 派生与覆盖 | 暗色定制不污染亮色，实例覆盖保留、其余字段跟随 | 共享可变基础或将全部字段永久冻结 |
| 语义色到组件 | 多类组件实际消费新角色，状态与对比度可检查 | 配方写死颜色或漏掉主题重解析 |
| 注册前配色诊断 | 必需角色对不达标则失败，诊断含角色/阈值，旧目录与生效状态不变 | 跳过规则或先注册再校验 |
| 半透明色校验 | 在已知底色上合成后比较；上下文不足不伪报通过 | 直接把含 alpha 的颜色传给忽略 alpha 的计算器 |
| 场景与合成终点 | 同场景可复现，不同背景分别核对；不透明中间层阻断底层影响 | 随意换底色，或绕过中间层直接比 background |
| 未知根 | 根仍透明且宿主未知时报告上下文不足 | 暗中补黑/白底或把未知当通过 |
| 判定空间与阈值精度 | sRGB 线性化后计算比值，临界值不因格式化通过 | 用 OKLCH L 差值替代比值，或先四舍五入再比较 |
| 唯一合成器 | 单层/多层合成符合独立固定期望，所有 CPU 校验消费同一实现 | 改坏共享实现或恢复局部重复公式；后者也需代码审查守住 |
| 合成与动画分离 | source-over 对应合成配置，动画仍按 OKLCH 最短色相弧 | 用 NanColor::mix 代替合成，或改成合成公式推进动画 |
| 渲染端到端核对 | 真实后端读回 RGB 与声明配置吻合，容差能拒绝错误空间 | 将合成器或渲染路径单独切到另一空间 |
| 原始与判定值诊断 | 保留输入 OKLCH，同时输出色域处理后 sRGB 和最终有效前景/背景 | 只打印裁剪后颜色，或从它反推并冒充原始值 |
| 内部按维度收窄（后续） | 仅动效变化及时影响动效消费者，不重解析无关样式 | 直接丢弃动效通知，或继续无差别调用全量样式解析 |

回调重入、异常与通知期间销毁的注入用例须在 §10 策略确定后补入。尺寸和字体变更还应验证布局
正确性，不能只数事件或查颜色。

代码阶段运行主套件、ASan/UBSan、no-RTTI，并确认 suite 注册；有窗口侧影响时更新实际演示。
手工验收包括：外部切换后控件同步、多暗色选择记忆、跨家族切换、文本/禁用/焦点状态可读性。
手工观感不替代自动化证据，自动化通过也不等于观感已经人工确认。

## 10. 下一轮审阅需要确认的事项

1. **选择 API**：固定方案与按外观模式的具体类型、模式切换时保留哪些偏好、无效槽位是否采用
   本文建议的拒绝且保留旧状态策略。
2. **注册与身份**：标识文本的命名空间、大小写/分隔符、parse/format 与迁移；同 ID 内容替换、
   删除后的禁止转用与同方案恢复、匿名来源、版本持久化边界。legacy 退出按“同一状态的入口
   收敛”估算，不按双状态迁移估算；独立方案模型的数据调整另列。
3. **作者入口与颜色契约**：本轮颜色数学部分只剩两项待确认：
   **合成空间**（编码值方案已有当前源码依据，等真实后端读回定案）与
   **色域策略**（明确当前裁剪是否保留，并与渲染路径的转换/量化核对）。
   §6.5–6.7 的背景终点、上下文不足规则、空间职责分离、未格式化值判定和唯一合成器归属
   已确认，不再重复列为待选方案。既有作者 API 的构建时派生、首批校验档案/阈值与显式例外、
   严格模式对未知项的处理、TOML 范围仍在各自实施阶段定形，不因两项颜色决策而被视为已完成。
4. **状态桥接**：按所有权而非伪分层约束选择 manager 无 Graph 或 Graph 注入；是否沿用
   ReadProperty 形状，如何接入依赖追踪，以及只读句柄的寿命与失效行为。
5. **事件协议**：首批两字段窄投影的命名与订阅契约；后续选择/身份/内容/动效通道分别定形，
   不把宽泛 revision 冒充纯内容事件。
6. **通知时序**：内部广播、只读投影与外部事件的先后顺序；回调重入是排队还是拒绝，异常如何
   不阻断必要更新，通知过程中销毁如何安全处理；内部广播是否及何时按外观/内容/动效维度
   收窄，如何证明无关样式不重解析且实际动效仍更新。
7. **交付范围**：是否按 §8 先交付窄投影；首批不等待全部身份决策，后续分批完善设置持久化与
   作者入口；颜色合成与读回核对归第 4 步之后的独立校验子项。第三方配方扩展继续独立排期，
   不与主题创作入口混为一项。

以上保留原设计讨论；首批第 4–6 项中与窄投影有关的桥接、订阅和通知策略已按 §11 实施与
复核补修，其余身份/选择/作者入口仍未实施，具体建议见 §12。不把未实施 API 写入入门教程。

---

## 11. 首批实施记录（P4.3 D2 窄投影，2026-11-05）

### 2026-11-06 复核补充：首批通知协议

本批采用 manager 不持有 Graph、作用域适配器桥接的方案。一次同步发布分为内部观察者通知、
全部只读投影写入、响应式 effect 提交、外部订阅四段；内部观察者仍沿既有 SceneTree 路径，
不新增第二轮组件广播。投影写入期间延迟 effect，因此同一事务中多个投影先形成一致载荷，
订阅和 effect 才可读取；窗口 tick 的外层 deferred effects 仍在 reactive 阶段提交。

发布期间允许读状态、订阅/解绑、销毁投影/作用域或 manager；新订阅不补发初始变化，已解绑
或已失效的订阅不再回调。观察登记使用独立身份，移除后在相同地址新建的观察者不继承旧通知。
销毁投影不会销毁正在通知的内部状态；`readonly()` 为借用视图，仅在投影所属作用域仍存活时
可读，Graph 必须活过投影与本次发布。manager 先销毁时投影失效但保留最后载荷。

通知期间通过 manager 再次写入（包括注册/样式/偏好等入口）在任何状态修改前抛
`std::logic_error`；本批选择拒绝重入，不把新主题请求插入当前发布。某个内部观察者、effect
或外部订阅抛异常时继续其他必要通知和订阅，发布结束后向调用方重抛首个异常，已提交状态不
回滚。单个 effect 抛错时，该 Graph 当前队列仍按既有异常语义停止；本协议保证其他观察者、
投影和外部订阅继续投递，不扩展为“图内所有 effect 异常后继续执行”的新响应式模型。

按 §8 第 1–2 步交付；载荷、只读入口与桥接所有权按 §5.5 定形，不等待身份/选择/目录模型：

- **载荷**：`theme::ThemePresentationState { ColorAppearance appearance; std::uint64_t revision; }`
  ——有效外观 + 不透明呈现修订标记（含动效变化），不带方案身份、家族、偏好或假注册名。
- **只读投影**：`theme::ThemePresentation`，由 `ThemeManager` 观察者驱动，内部 `Signal` 把状态
  桥接给 reactive；`get()`/`appearance()`/`revision()` 是依赖追踪读取，`readonly()` 交给子组件，
  `subscribe(handler)` 返回 RAII `Subscription`。订阅不补发初始变化；等价状态去重。
- **归属与作用域**：`ReactiveScope::own<T>()` 提供作用域拥有的非响应式服务（按类型唯一，
  先于 scope 信号销毁，便于释放外部观察者）；`BuildContext::theme_state()` 用页面回调作用域
  暴露唯一实例，派生区域共享。`ThemeManager` 不依赖 `Graph`。
- **生命周期**：manager 先销毁 → `valid()` 转 false，保留最后载荷；作用域先销毁 → 注销观察者；
  两端任一先销毁均不悬垂访问、不再回调。
- **发布点审计**：`apply`、具名 `activate`、遗留/全快照家族、`set_preference`、系统偏好与
  动效入口均验证窄投影变化；具名同快照的显式/系统外观变化在生产者补发，活动家族替换提交
  新内容；等价家族注册、相同生效快照切家族和只改未生效槽位不制造呈现 revision。
- **应用验收**：Showcase 侧边栏亮暗开关跟随投影（外部外观变化回写 `checked`，静默 setter
  不冒充用户 `on_change`）。

首批不包含：选择状态、方案身份/目录、纯主题内容事件、`ThemeObserver` 退役、作者颜色入口、
颜色合成与校验（§8 第 3–5 步与 4a）。上述形状待维护者定形后再扩展载荷；扩展仍须复核
聚合初始化、比较与通知范围。实现与验证见
[D2 实施记录](reference_draft.md#p43-d2-首批窄投影实施记录2026-11-05)。

## 12. 第 3 步 API 建议（2026-11-06 定形，实现见 §13）

本节建议已获维护者接受，并作为第 3 步实施契约；当前交付与补修边界见 §13，仍保持未提交。

| 议题 | 建议取舍 |
| --- | --- |
| 方案身份 | 强类型 `ThemeSchemeId`，规范文本为 `namespace:name`，例如 `org.example:midnight`、`nandina:default/light`；必须有命名空间，大小写敏感，仅接受规范小写 ASCII，不自动折叠或裁剪 |
| 文本语法 | namespace 为点分段，每段 `[a-z][a-z0-9-]*`；name 为斜线分段，每段 `[a-z][a-z0-9_-]*`；总长不超过 255 字节，拒绝空段、`.`/`..`、空白和 Unicode。`parse` 返回带偏移的 typed error，`text()` 无损返回规范文本；持久化格式另带 schema 版本 |
| 内容形状 | `ThemeSchemeContent` 只含一套 `NanColorScheme`，以及完整 tokens、typography、components；`ThemeScheme` 另持有 ID、显示名、固定 light/dark 分类与可选家族 ID。内容不可变，家族只分组，不隐式解析亮暗搭配 |
| 选择形状 | 用 `std::variant<FixedThemeSelection, AppearanceThemeSelection>` 表达两种互斥模式；固定来源是 scheme ID 或显式未注册快照，按外观存 light/dark 两个注册 ID 和 `ThemePreference`。切换时提交完整选择值；记住另一模式的历史配置属于应用设置，不藏在 manager 中 |
| 失败行为 | 先验证所有 ID 与槽位分类，再一次性提交；未知 ID、缺槽、分类不匹配均返回 `std::expected<void, ThemeSelectionError>`，目录、选择、生效快照和 revision 不变。系统变化在固定模式下不改方案或其外观分类 |
| 注册/替换/移除 | `register_scheme` 仅新增，重复 ID 返回错误；`replace_scheme` 显式替换同一逻辑方案。当前内容替换必须发布，即使 ID/外观不变；非当前项变化只推进目录。正在被选择任一槽引用的项拒绝移除，先改选择再移除 |
| 删除与恢复 | 框架不从内容哈希推断逻辑方案身份，不在内存中维护无界删除墓碑；ID 永远不得转配给另一方案由发布者保证。同方案重装可复用 ID；跨进程设置恢复到未知 ID 时诊断失败，由应用显式选择回退 |
| 匿名来源 | `UnregisteredThemeSource { content, appearance }` 是固定模式的显式来源，与注册 ID 互斥；仍通过同一个 select 入口提交，不在已注册 selection 之外另藏一次 apply 状态。它没有 scheme ID，也不填充 `application` 等假 ID；匿名内容不作为可恢复的注册选择持久化 |
| 通知基础 | 沿用本批 manager 无 Graph + 作用域投影；两字段窄投影语义保持不变。选择/目录/身份与纯主题事件另开通道，不往 revision 推断变更类型；沿用分阶段、拒绝重入和异常后完成必要通知的协议 |
| legacy 收敛 | 在第 3 步同批迁移 register_theme/family、activate/family、set_theme 等旧入口与仓库消费者，最终只有 scheme 目录和 selection 一条可写状态链路。旧双 palette DesignSystem 先作为组件解析快照适配，不作为新独立方案的作者模型 |

类型和入口形状（已按本批实现；具体错误类型名以公开头文件为准）：

```cpp
struct UnregisteredThemeSource {
    std::shared_ptr<const ThemeSchemeContent> content;
    ColorAppearance appearance;
};
using FixedThemeSource = std::variant<ThemeSchemeId, UnregisteredThemeSource>;
struct FixedThemeSelection {
    FixedThemeSource source;
};
struct AppearanceThemeSelection {
    ThemeSchemeId light;
    ThemeSchemeId dark;
    ThemePreference preference = ThemePreference::system;
};
using ThemeSelection = std::variant<FixedThemeSelection, AppearanceThemeSelection>;

auto ThemeSchemeId::parse(std::string_view) -> std::expected<ThemeSchemeId, ThemeSchemeIdError>;
auto ThemeSchemeId::text() const noexcept -> std::string_view;
auto ThemeManager::select(ThemeSelection) -> std::expected<void, ThemeSelectionError>;
auto ThemeManager::register_scheme(ThemeScheme) -> std::expected<void, ThemeCatalogError>;
auto ThemeManager::replace_scheme(ThemeScheme) -> std::expected<void, ThemeCatalogError>;
auto ThemeManager::remove_scheme(const ThemeSchemeId&) -> std::expected<void, ThemeCatalogError>;
```

以上 ID 文本按本批接受的公开兼容面实施。框架不代写应用设置文件；持久化只保存 schema、选择模式、
规范 ID 和必要偏好，不保存 manager revision；家族 ID 的文本可复用同一语法，但使用不同强类型。
同 ID 内容替换以显式不可变内容快照为依据，同快照重复提交幂等；框架不承诺深比较任意配方。
颜色合成/色域/真实像素读回保持第 4a 子项，第 3 步只完成结构与引用校验。

---

## 13. 第 3 步实施记录（P4.3 D2 方案目录与选择，2026-11-06，初稿待维护者复核）

本节标题保留历史锚点；第 3 步已完成复验并随 `995e6ae` 提交，以下为该批实施与复核记录。

按 §12 建议实施并完成测试迁移；legacy 写入口（register_theme/activate/register_family/
activate_family/register_theme_family/set_theme/apply）随本批从 ThemeManager 移除，仓库内
调用点（showcase、desktop probe、notebook）与既有测试经测试兼容层迁移到 scheme 目录/选择。

- **身份**：`ThemeSchemeId` / `ThemeFamilyId` 强类型，规范文本 `namespace:name`；namespace
  点分段、name 斜线分段，只接受规范小写 ASCII，拒绝空段/`.`/`..`/空白/Unicode，总长 255 字节
  上限；`parse` 返回带偏移的 typed error（空段指向空段起始分隔符，其他错误指向失效段起始），`text()` 无损返回。
- **内容与目录**：`ThemeSchemeContent`（单套 palette + tokens/typography/components）是唯一
  内容载体，`DesignSystem` 只是渲染器适配快照（同一 palette 放入两槽）；`ThemeSchemeCatalog`
  是值目录，register/replace/remove_scheme、register/remove_family，注册前校验 ID/家族/内容，
  移除受任一选择槽引用保护的方案。staged 拷贝提交：失败不改变目录、选择、生效快照或 revision。
- **选择**：`ThemeSelection = variant<FixedThemeSelection, AppearanceThemeSelection>`；fixed
  忽略系统外观，appearance 按 preference+系统外观选 light/dark 槽；槽位外观分类不匹配、
  未知 ID、空内容均返回 typed error。`UnregisteredThemeSource` 是固定模式显式匿名来源，
  无 scheme ID、不填充假注册名。
- **发布**：当前激活内容替换（即使 ID/外观不变）必须发布；同快照重复提交幂等；只改选择
  未生效项（亮色下改暗色槽位）只更新选择，不触发主题变化事件；系统外观变化经
  `set_system_preferences` 重新 resolve 并只在实际有效外观变化时发布。
- **文件导入**：`StyleDocument::apply` 暂存并验证所有文件主题与家族，再将目录、选择、style
  规则一次提交；无效/过长别名返回 expected 错误而非抛 bad_expected_access。主题 ID 为
  `styles:x<theme-hex>/light|dark`；家族 ID 为 `styles:x<family-hex>`；家族成员采用
  `styles:x<theme-hex>/families/x<family-hex>/light|dark`。共享文件主题的家族成员有独立 ID，
  共享不可变内容，不因激活变化改写其他家族归属。未激活家族同样校验并登记。
  文件别名不成为 manager 身份；重复导入提供新快照，只发布一次完整状态。可选字体注册在
  主题提交之后沿现有流程执行，不承诺字体失败回滚已经提交的主题与字体。
- **兼容查询（仅测试层）**：`theme_test::Manager`（tests/theme_test_support.hpp）把历史双
  调色板场景翻译成 scheme 选择，并复用 `require_idle` 通知守卫；active_name/active_family/
  preference 从目录与选择推断。
- 实现与验证记录见 [reference_draft](reference_draft.md#p43-d2-第-3-步方案目录与选择实施记录2026-11-06初稿待维护者复核)。

### 第 3 步复核补修边界（2026-11-06）

- 初稿的 6 个相关专项基线通过；追加直接使用生产 ThemeManager 的用例，避免只借测试兼容层
  宣称新 API 行为已经验证。兼容层继续限于历史场景，其查询修正与 ID offset=17 均保留。
- StyleDocument 原有逐项写入在长别名失败时部分提交、首次导入先发旧状态、只登记激活家族
  并抢占共享成员归属，三组回归均先失败后补修；现在主题目录/选择/样式只走一次提交。
- 替换方案与已选槽位的外观冲突返回 ThemeCatalogErrorKind::appearance_mismatch；所有失败
  保持原目录、选择、快照指针和 revision。目录/选择查询是借用视图，成功写入可使旧引用失效；
  长期保存方案须复制值或 shared content。ID 255 字节边界、目录复制后的独立身份、跨家族配对、
  固定/匿名来源、未生效槽位与内容替换、同内容换 ID、异常与通知中销毁均有生产 API 回归。
- Sidebar 在固定来源下没有可选另一槽位：用户切换后恢复实际 checked，不猜测另一方案或改写
  固定身份；appearance 模式仍显式提交原配对与新 preference。本地 playground 的旧选择入口
  也已迁移；其未跟踪源码不作为 CI 证据。

完整红测与最终矩阵见 reference_draft 的第 3 步复核记录。作者颜色入口与第 4/4a 步未启动。

---

## 14. 第 4 步 API 定形（2026-11-07）

本节把 §8 第 4 步拆成可实施的具体形状；实现记录见 §15。观察维度仍按 §5.1 区分，
发布协议沿用 §5.4 的分阶段提交（内部广播 → 投影写入 → effect 提交 → 外部订阅）与
§11/§13 的拒绝重入、异常隔离规则。

### 14.1 观察通道

| 通道 | 只读载荷 | 何时通知 | 何时不通知 |
| --- | --- | --- | --- |
| 呈现窄投影（§5.5，已交付） | `appearance` + 不透明 `revision` | 有效外观、有效内容、style 或动效发布 | 仅选择或仅目录变化 |
| 选择投影 `ThemeSelectionPresentation` | `ThemeSelectionState = ThemeSelection` | 模式、亮暗槽位或偏好改变（即使生效内容不变） | 仅生效变化、仅目录变化 |
| 目录投影 `ThemeCatalogPresentation` | `ThemeCatalogState {version, schemes, families}` | 注册/替换/移除方案或家族 | 仅选择变化、仅生效变化 |
| 变化事件 `ThemeChangeHub` | `ThemeChange {before, after}`，端点为 `{optional<ThemeSchemeId>, ColorAppearance, shared_ptr<const DesignSystem>}` | 生效身份、外观或有效快照变化 | 动效-only、非生效选择编辑、目录-only 变化 |

- **发布分类**：有效变更（内容/身份/外观/style/动效）走 revision 发布并保留既有内部广播；
  仅选择或仅目录变化走 staged-only 发布——不递增 `revision`、不向组件广播失效，只把
  已提交状态交给对应投影。目录事件以单调 `ThemeCatalogState::version`（只由目录项增删改
  推进，选择不改）为依据；选择事件以 `ThemeSelection` 值比较。等价 replace 不推进目录版本，
  相等定义为 ID、显示名、外观、家族相同且内容快照指针相同；不深比较任意配方。
- **所有权**：三者都是作用域绑定的只读投影（`ReactiveScope::own<T>`），随作用域注销观察者；
  manager 先销毁时 `valid()` 转 false 并保留最后载荷，任何一端先销毁都不悬垂访问。
  订阅返回 RAII `Subscription`；`ThemeChangeHub` 只公开 `subscribe`，不暴露可发布 Event。
- **便利入口**：`BuildContext::theme_selection()/theme_catalog()/theme_changes()` 每个页面
  作用域共享一个实例；`on_theme_change(handler)` 通过新增的 `ReactiveScope::keep` 把连接
  纳入作用域，作者无需手写注销。
- **交付顺序**：事件在四段通知的 notify 阶段交付；回调抛错不阻断后续投影/订阅者，首个
  异常在必要交付完成后重抛；订阅不伪造初始变化，初始状态一律通过读取获得。

### 14.2 统一作者入口（派生、结构校验、注册）

```cpp
auto base = catalog.find(id);                       // 选择基础方案
auto derived = theme::derive_scheme(*base, new_id, "Midnight");
auto content = std::make_shared<theme::ThemeSchemeContent>(theme::derive_content(*base->content));
content->tokens.radius.md = 10.0F;                  // 只改派生快照
derived.content = content;
if (auto ok = theme::validate_scheme(derived, catalog); !ok) { /* describe(ok.error()) */ }
themes.register_scheme(std::move(derived));         // 与文件导入共用目录/选择提交语义
```

- `derive_content(base)` 深拷贝内容；`derive_scheme(base, id, name)` 内容独立深拷贝并继承
  外观分类与家族归属。此后替换基础方案不会重写已注册派生方案（重新派生是显式动作）。
- `validate_scheme(scheme, catalog)` 做**结构校验**：ID 有效、显示名非空、内容非空、
  ID 未与目录冲突、家族存在。它不包含配色对比度/色域门槛（4a 子项），也不替代注册时
  目录与选择的校验。
- 注册入口仍是 `ThemeManager::register_scheme` / `replace_scheme`；`StyleDocument::apply`
  与 C++ 作者走同一暂存目录 + 选择提交，失败不留部分状态。

### 14.3 文件导入范围（styles.toml 子集）

已支持：`reference_palettes`、`themes`（参考色阶或直接语义色板）、`theme_families`、
`active_theme` / `active_family` / `appearance` 选择、遗留 `NanStyle` 规则、`fonts`。
尚未支持：Button/TextField 之外的配方覆盖、组件样式 reset、任意自定义 token、主题包版本
锁定与跨进程设置持久化。文件主题/家族编译为 `styles:x<hex>` 规范 ID，不成为 manager 身份；
完整 TOML schema 扩展另行分期。

### 14.4 不在第 4 步范围

4a（唯一合成器、对比度/色域诊断与注册门槛、真实后端像素读回）、内部广播按外观/内容/动效
维度收窄（本批保持保守广播，但选择/目录变化已不触发组件失效）、第三方配方扩展、设置
持久化、Windows/GPU/人工视觉验收。

---

## 15. 第 4 步实施记录（2026-11-07，初稿待维护者复核）

- **观察通道**：新增 `theme/theme_observation.{hpp,cpp}`：
  `ThemeSelectionPresentation`、`ThemeCatalogPresentation`、`ThemeChangeHub`（含
  `ThemeChange`/`ThemeChangeEndpoint`）。三者按 §14.1 分类接收通知。
- **发布分类**：`ThemeManager` 的 `publish_revision()` 拆为 revision 发布 + `publish_staged()`
  入口；`commit_catalog` 依据有效内容/身份/外观/style/动效与选择/目录差异选择通道。
  `ThemeSchemeCatalog` 新增单调 `version()`（仅目录项增删改推进）。
- **作者入口**：`theme_scheme.{hpp,cpp}` 新增 `derive_content`、`derive_scheme`、
  `validate_scheme`、`ThemeSchemeError` + `describe`；`ThemeSchemeCatalog::contains_family`。
- **作用域便利**：`ReactiveScope::keep(Subscription)`；`BuildContext` 新增
  `theme_selection()/theme_catalog()/theme_changes()/on_theme_change()`。
- **内置主题迁移**：`register_default_theme_families` 把 butter/fluent/material 编译为
  `nandina:<family>/light|dark` 规范 scheme 与 `nandina:<family>` 家族；目录级回归验证
  两个外观各自持有独立内容、规范 ID 可固定/按外观选择、重复注册幂等。
- **契约边界**：文件导入范围按 §14.3 写入 `design_tokens.md`；4a 与广播收窄不在本批。

故障注入（每项重新编译，恢复后全绿）：
1. 关闭 staged-only 发布 → 选择/目录变化不再通知（目录投影用例 2 处断言失败）。
2. 目录 `version` 不递增 → 目录投影无法感知注册（1 处断言失败）。
3. 变化事件去掉端点去重 → 动效-only 发布误报事件（3 处断言失败）。
4. `derive_scheme` 共享内容指针 → 派生修改污染基础（1 处断言失败）。
5. 结构校验不拒空内容 → `null_content` 用例失败（1 处断言失败）。

专项：`theme-scheme` 23 用例 / 333 断言；`theme-observation` 6 用例 / 124 断言；
`theme-presentation` 26 / 163；`theme-families` 9 / 69；`theme-design-system` 25 / 637；
`button` 33 / 221；`sidebar-showcase` 6 / 55。

最终验证矩阵见 [reference_draft](reference_draft.md) 第 4 步实施记录；本轮保持未提交。

### 第 4 步复核补修（2026-11-07，保持未提交）

Git 基线为 `995e6ae`（第 3 步已提交并推送）。先编译并通过三个专项，再新增回归，确认
等价目录替换仍递增 version 并触发订阅与 effect（16 断言中 8 失败）。现在等价替换幂等，
显示名/家族/外观/内容快照实际变化仍推进目录版本；目录投影先比较版本再复制载荷，避免
选择-only 或 motion-only 发布重复复制全部方案。

补充三个通道在投递中清理 scope、销毁 manager、断开后续订阅、拒绝重入与隔离异常的回归；
直接检查 SceneTree 内部通知计数，证明 staged-only 不广播、实际呈现变化只广播一次。
选择 readonly 与目录 computed 均验证依赖追踪；文件导入同时更新目录/选择/主题时，effect
与订阅读到同一完整状态；外层窗口 deferred effects 保持到 reactive 阶段才提交。

事件端点验证匿名来源、style 变更、motion-only 去重和 manager 销毁后的快照所有权。
页面 clear 后再次执行真实生效变更，两个 BuildContext 订阅均不再调用；派生则补充嵌套
recipe vector 隔离、家族继承与替换基础后已注册派生内容保持独立。新通道无 set/emit 的
编译期能力检查与结构校验 invalid_id 分支均已补齐。

故障注入与本轮完整矩阵见 reference_draft 的第 4 步复核记录。第 4a 步、作者颜色入口与
内部按维度收窄广播未并入本次补修；历史记录不作为桌面/GPU 或 Windows 验收。

---

## 16. 第 4a 步 CPU 侧实施记录（2026-11-07，初稿待维护者复核；后端读回未执行）

按 §6.5–§6.7 定形并落地 CPU 侧；真实后端像素读回（§6.8）与注册门槛接线仍待有 GPU 的
环境，本批不宣称配色门槛已通过。

- **foundation 唯一合成器**（`nandina/foundation/composition.hpp`）：
  `CompositeSpace`/`CompositeConfig` 显式声明首版策略——编码 sRGB 通道上的
  straight-alpha source-over，与当前 SDL 绘制路径源码核对一致；`nan_composite_over`
  要求不透明背景（误用抛 `invalid_argument`，非有限颜色同样拒绝），
  `nan_composite_background_stack` 按「由近及远」查找首个不透明终点并提前终止，
  `nan_composite_over_stack` 先合成背景栈再覆盖前景；无有限终点/非有限层/不支持空间
  返回 `CompositionError`，不猜测底色。
- **theme 诊断**（`nandina/theme/contrast_diagnostics.hpp`）：
  `ContrastRule` 显式声明前景、从近到远的背景栈、适用状态、所需阈值与 required 级别；
  `evaluate_contrast` 用 `resolve_theme_color` 解析角色，交给 foundation 合成器判定
  「有效前景/有效背景/对比度」，输出 `ContrastDiagnostic`（方案 id 或匿名来源、外观、
  规则名与状态、实测比值、所需阈值、每层解析后 OKLCH 与转换后 sRGB、角色路径、状态）。
  `ContrastStatus` 区分 passed/failed/insufficient_context：
  `has_required_contrast_failure` 只统计必需规则的实测失败，
  `has_insufficient_contrast_context` 单独报告上下文不足，不伪报为实测失败。
- **首批显式档案**：`default_contrast_profile()` 15 条与组件默认配方对应的规则
  （正文/卡片/浮层/次要文字/中性/强调/品牌/状态色/焦点环与信号色），状态色按 3:1、
  文本按 4.5:1；不做隐式整表断言、不做风格建议，严格模式与显式例外留待注册门槛批次。
- **局部 helper 收敛**：`theme_palette_contrast_tests` 与 `command_palette_tests` 的
  `composite_over` 改为消费 foundation 唯一实现，不再复制生产算法。
- **不在本批**：真实 RenderDevice 绘制 + framebuffer 读回（§6.8 步骤 1–5，需窗口/GPU）、
  由此确定色域策略与合成空间最终结论、注册/替换前的配色门槛接线、内部广播收窄、
  风格建议与可访问性认证。

故障注入（重新编译后恢复，均红转绿）：
1. 合成改在线性通道 → 编码空间手算用例 3 处断言失败。
2. 背景栈不提前终止（校验不透明终点之后的层）→ 提前终止用例 1 处断言失败。
3. 诊断把上下文不足记为 passed → 上下文不足用例 2 处断言失败。

专项：`composition` 6 用例 / 35 断言；`theme-contrast-diagnostics` 6 / 181；
`theme-palette-contrast` 4 / 129；`command-palette` 15 / 105。

首版维护者验证矩阵：buildDir 全量 121 OK / 1 FAIL（sdf-state-probe，无显示设备）/ 1 SKIP
（sdf-device 77）；ASan 100 OK / 1 skip；no-RTTI 102 OK；no-SQLite 97 OK / 1 skip；
tools 8 OK；release_check --ci 8/8（headers 197、docs 87、clang-format 23.1.1）。
本轮保持未提交。

### 第 4a 步 CPU 复核契约补充（2026-11-07，保持未提交）

场景使用 `ThemeColor::accent()/on_accent()`（含变换操作数）时，必须显式提供
`ContrastRule::tone`；缺少 tone 返回上下文不足，不沿用通用颜色解析器的 primary 回退。
诊断保留该 tone 与 `CompositeConfig`；尚不能解析的前景使用空的可选 sample，避免伪造颜色。
背景解析与采样同样在首个不透明终点停止，诊断只列出已检查的角色与已解析的颜色。
变换角色路径保留操作及操作数；临界比值和阈值的现行显示精度经相邻 float 回归确认，
判定仍直接使用未格式化值。

本批 CPU 数值沿用 `NanColor` 的规范化存储值、现有 OKLab → linear RGB 通道裁剪和浮点
sRGB 转换，不额外做 8-bit 量化；报告明确列出该策略。解析后 OKLCH 不是导入前作者原文，
原文追溯与真实绘制路径的量化/色域定形仍属后续验收。

本次复核先编译通过四个专项基线，再新增回归：缺少 tone 静默采用 primary（6 断言失败）、
变换角色丢失操作数（2 断言）、不透明终点后仍采样（2 断言）、报告缺少合成/色域/量化策略
（3 断言）。合成器本体未发现缺陷；补充未知空间、空栈、近不透明 alpha、非有限 alpha 与
错误下标覆盖。主题侧补充四种显式 tone 的两个外观、背景/变换 rhs 的缺失 tone、遮挡层、
非法阈值、相邻 float 边界、规范化 OKLCH 与裁剪后 sRGB 独立保留。

`default_contrast_profile()` 中状态色文字的 3:1 保留既有参考色阶家族的历史下限，
不代表普通小字满足 AA；默认主题仍由原有专项按 4.5:1 约束。正式注册档案及例外必须另行
审阅，本批不会提高或降低既有门槛，也不把诊断通过当作真实后端或整套 UI 验收。

专项：composition 7 用例 / 52 断言；theme-contrast-diagnostics 15 / 338；
theme-palette-contrast 4 / 129；command-palette 15 / 105。
本次最终矩阵：主套件 120 OK / 2 FAIL（sdf-device 与 sdf-state-probe 均无显示设备）/
1 SKIP；ASan 100 OK / 1 SKIP，no-RTTI 101 OK / 1 SKIP，no-SQLite 97 OK / 1 SKIP，
tools 8 OK；release_check --ci 8/8（headers 197、docs 87、clang-format 23.1.1）。
各套 SKIP 为 window-lifecycle，环境差异与日志见 reference_draft 第 4a 步 CPU 复核记录。
本批已随维护者统一提交 `fef829a` 推送；真实读回与配色注册门槛未完成。

---

## 17. 第 4b 步作者颜色入口：API 定形（2026-11-07）

§6.2 的两条作者路径（参考色阶派生、直接语义色声明）已存在于 `theme.hpp`，但 C++ 侧缺少
「在既有方案上显式覆盖单个语义角色」与「类型化读取 primitive 色阶」的入口；文件导入
（`[themes.x.palette]` 直接色值、`$palettes.<name>.<scale>.<shade>` 引用）已支持同类语义。
本节定形两侧收敛到同一提交语义的最小 API，不新增 red/blue 等语义 token。

### 17.1 primitive palette 访问（theme.hpp）

```cpp
enum class ReferenceColorFamily : std::size_t {
    primary, secondary, tertiary, neutral, success, warning, error,
};
[[nodiscard]] auto reference_scale(NanReferencePalette&, ReferenceColorFamily) noexcept -> NanColorScale&;
[[nodiscard]] auto reference_scale(const NanReferencePalette&, ReferenceColorFamily) noexcept -> const NanColorScale&;
[[nodiscard]] auto reference_color(const NanReferencePalette&, ReferenceColorFamily, ColorShade) noexcept -> const NanColor&;
[[nodiscard]] auto reference_family_name(ReferenceColorFamily) noexcept -> std::string_view;
[[nodiscard]] auto color_shade_name(ColorShade) noexcept -> std::string_view;
```

- 与 `$palettes.<name>.<scale>.<shade>` 的字符串寻址一一对应，C++ 作者不再手写字段名或下标。
- `ThemeSchemeContent` 增加**可选** `std::optional<NanReferencePalette> reference`：只记录
  「由哪个 primitive 色阶派生」的来源，`palette` 仍是唯一权威语义事实；从
  `from(DesignSystem, ...)` 或直接语义色构造的方案不伪造来源（保持 `nullopt`）。

### 17.2 显式 `NanColor` 语义覆盖（theme_scheme.hpp）

```cpp
struct SemanticColorOverride { ColorToken role; NanColor color; };
enum class ThemeColorErrorKind { non_finite_color };
struct ThemeColorError { ThemeColorErrorKind kind; ColorToken role; };
[[nodiscard]] auto describe(const ThemeColorError&) -> std::string_view;

[[nodiscard]] auto canonical_color_token(ColorToken) noexcept -> ColorToken;   // on_* / outline* 等别名 → 规范名
[[nodiscard]] auto semantic_color(const NanColorScheme&, ColorToken) noexcept -> const NanColor&;
[[nodiscard]] auto set_semantic_color(NanColorScheme&, ColorToken, NanColor) -> std::expected<void, ThemeColorError>;
[[nodiscard]] auto apply_semantic_colors(NanColorScheme&, std::span<const SemanticColorOverride>)
    -> std::expected<void, ThemeColorError>;

struct ThemeSchemeContent {
    ...
    [[nodiscard]] static auto from_reference(const NanReferencePalette&, ColorAppearance,
                                             PaletteVariantPolicy = {}) -> ThemeSchemeContent;
    [[nodiscard]] static auto from_semantic_colors(const SemanticColorSpec&) -> ThemeSchemeContent;
};
```

- 覆盖键是**既有语义角色**（`ColorToken`），别名映射到规范名并同步兼容字段，不引入
  `red`/`blue` 之类原始色 token；作者要固定色值就用 `NanColor`（OKLCH）或参考色阶档位。
- 校验针对 `NanColor` 规范化存储的 OKLCH 分量：只拒绝非有限值；构造已归一化
  light/hue/alpha、将负 chroma 归零，入口不重复裁剪。批量先验证全部再写入，任一项失败
  整体不修改（含兼容别名）。色域策略仍由诊断与后端读回阶段处理（§6.6）。
- 两条构造便利入口只影响 palette 与来源记录，tokens/typography/components 取
  `default_design_system()` 的模板；派生（`derive_content`）深拷贝并保留 `reference`，
  修改派生方案不污染基础方案。
- 文件导入与 C++ 必须产生同一语义：TOML 直接色值等价于 `set_semantic_color` 的逐角色写入，
  TOML `$palettes.*` 引用等价于 `reference_color` 取值；本步以相等性回归与一次提交接线证明，
  不新增第二套解析/归一化路径。

### 17.3 本步不做

Shell 多方案设置页与 showcase 视觉验收（§8 第 5 步）；内部广播按维度收窄；注册配色门槛与
真实后端像素读回（仍待 GPU 环境）；第三方配方注册（§7.3 后续能力）；语义角色的增删。

### 17.4 第 4b 步实施记录（2026-11-07，初稿待维护者复核）

按 §17 定形落地 C++ 作者颜色入口，并与既有文件导入路径做相等性回归。

- `theme.hpp`：`ReferenceColorFamily`、`reference_scale`（const/非 const）、
  `reference_color`、`reference_family_name`、`color_shade_name`；与
  `$palettes.<name>.<scale>.<shade>` 的字符串寻址一一对应。
- `theme_scheme.hpp/.cpp`：`SemanticColorOverride`/`ThemeColorError`；
  `canonical_color_token`（on_* / outline* / focus_ring 映射到 shadcn 规范名）、
  `semantic_color`（规范或别名读取）、`set_semantic_color`（单角色写入并同步兼容别名）、
  `apply_semantic_colors`（先验证全部再一次性提交）。作者入口只拒绝非有限 OKLCH 分量：
  `NanColor` 构造本身已归一化 light/hue/alpha 并归零负 chroma，入口不重复裁剪，也不把
  色域处理混进作者写入。
- `ThemeSchemeContent`：新增可选 `reference` 来源记录（palette 仍是唯一权威语义事实）；
  `from_reference` 从 primitive 色阶派生并记录来源，`from_semantic_colors` 从显式语义
  声明构造且不伪造来源；`derive_content` 深拷贝并保留 `reference`。
- 文件导入与 C++ 收敛：TOML 直接色值等价于逐角色 `set_semantic_color`，
  `$palettes.*` 引用等价于 `reference_color`；回归按全部规范角色逐一比较，并验证
  `StyleDocument::apply` 注册出的规范方案 palette 与 C++ 构造一致。

故障注入（重新编译后恢复，均红转绿）：
1. `set_semantic_color` 不同步兼容别名 → 别名同值用例 1 断言失败。
2. 批量覆盖边验证边写入 → 原子性用例 1 断言失败（失败后 palette 已被部分修改）。
3. `reference_scale` 的 neutral 错映到 primary → 类型化访问用例 1 断言失败。
4. `derive_content` 丢弃 primitive 来源 → 派生保留来源用例 1 断言失败。
5. `on_muted` 不做别名规范化 → 别名写读用例 2 断言失败。

专项：theme-color-entry 7 用例 / 432 断言；既有 theme-palette-contrast 4/129、
theme-contrast-diagnostics 15/338、theme-scheme 25/357 保持通过。
首版维护者矩阵（复核前，Linux 无显示/GPU 沙箱；每套先编译再测）：buildDir 全量
122 OK / 1 FAIL（sdf-state-probe，无显示设备）/ 1 SKIP（sdf-device 77）；ASan 101 OK /
1 SKIP；no-RTTI 103 OK / 0 SKIP；no-SQLite 98 OK / 1 SKIP；tools 8 OK；
release_check --ci 8/8（headers 197、docs 87、clang-format 23.1.1）。
本批保持未提交，待维护者复核。


### 17.5 第 4b 步复核补充（2026-11-07，保持未提交）

文件 palette 覆盖与 C++ 覆盖复用 `apply_semantic_colors`：先解析为角色/颜色列表，再统一
验证、同步别名并提交。非有限的直接色值或已引用的 primitive 值在解析阶段返回错误；
未被引用的 primitive 来源不新增配色注册门槛。TOML 同时提供规范名和兼容别名时仍以
规范名为准；C++ span 则按声明顺序应用，重复角色（含归一化别名）最后一项生效。
primitive/语义查询 API 延续既有色阶查询的前提：调用方传入枚举中已声明的值。


复核先通过四个专项基线，再新增 C++/TOML 非有限输入相等性回归，原实现 5 断言失败
（light=NaN、chroma=inf、hue=inf、alpha=NaN，以及 `$palettes` 引用的非有限值）。
`StyleDocument::parse_palette` 现复用批量覆盖，不再直接修改颜色字段或另做别名同步。
独立故障注入去掉文件入口的错误传播，重新编译后同一用例 5 断言失败（exit 42）；
恢复后六个受影响专项全绿。

补充 const 访问与全部 7 族 × 11 档的字段/字符串对应、34 规范角色通过既有解析器消费、
14 别名双向写读与 TOML 规范名优先级、批量重复角色的声明顺序、失败不触碰兼容字段、
空批次与 primitive 来源的独立复制/不反向覆盖语义色。规范文档删除不可达的
`alpha_out_of_range` 示例，并修正重复的 §17.2 实施记录编号。

专项：theme-color-entry 14 用例 / 1243 断言；button 33 / 221，theme-scheme 25 / 357，
theme-design-system 25 / 637，theme-contrast-diagnostics 15 / 338，theme-palette-contrast
4 / 129 保持通过。

最终矩阵：主套件 121 OK / 2 FAIL（sdf-device 与 sdf-state-probe 均无显示设备）/ 1 SKIP；
ASan 101 OK / 1 SKIP，no-RTTI 102 OK / 1 SKIP，no-SQLite 98 OK / 1 SKIP，tools 8 OK；
release_check --ci 8/8（headers 197、docs 87、clang-format 23.1.1）。各套 SKIP 均为
window-lifecycle，SDK 双次打包与 vendored 消费者通过。实际日志与首版环境差异见
reference_draft 第 4b 步复核记录。本轮保持未提交，待维护者复验。

---

## 18. 第 4c 步 foundation 命名颜色字面量：API 定形（2026-11-08）

主题作者需要一个「固定色值」的便捷写法，但不应为此把 red/blue 等原始色升成语义 token。
本步在 foundation 提供**命名颜色字面量**：它只是显式 `NanColor` 的一种写法，不是新的
运行时覆盖顺序，也不改变组件继续只消费语义角色的默认路径。

### 18.1 范围与规范快照

- **148 个 CSS 命名颜色**：CSS Color 4 命名颜色表 = SVG 1.1 的 147 个 + `rebeccapurple`。
  固定快照为 **CSS Color 4 候选推荐草案 2026-11-03 §6.1**
  （<https://www.w3.org/TR/2026/CRD-css-color-4-20261103/#named-colors>），逐条采用规范
  固定的 sRGB 整数值（例如 `green = #008000`、`lime = #00FF00`、`rebeccapurple = #663399`）。
- **排除 `transparent` 与 `currentColor`**：前者是透明黑、后者依赖元素上下文，均不是
  固定不透明色值，不适合纯查表入口。

### 18.2 公开 API（foundation/named_color.hpp）

```cpp
enum class NanNamedColor { aliceblue, ..., yellowgreen };   // 148 项，小写、字母序
[[nodiscard]] auto nan_named_color(NanNamedColor named) noexcept -> NanColor;
[[nodiscard]] auto nan_named_color(std::string_view name) -> std::optional<NanColor>;
[[nodiscard]] auto nan_named_color_name(NanNamedColor named) noexcept -> std::string_view;
```

- **双入口共用一张表**：公开头只放枚举与三个函数声明；唯一 `constexpr` 数据表
  （`{整数 RGB, 规范名称}`）放 `.cpp` 内部，不暴露表结构，也不给 `NanColor` 堆成员。
- **枚举与持久化**：148 个名称各有独立枚举成员与表项，RGB 相同的别名（`aqua`/`cyan`、
  `fuchsia`/`magenta`、`gray`/`grey` 系列等）**不去重**；枚举序号**不承诺持久化稳定**，
  需要保存时使用 `nan_named_color_name` 的规范名称。
- **转换**：整数 RGB 经既有 `NanColor::from_hex` 规范化为 OKLCH，与作者写 hex 完全同一路径。

### 18.3 查询规则

- ASCII 大小写不敏感：只折叠 `A–Z`，不依赖 locale；`"SteelBlue"`、`"steelblue"`、
  `"STEELBLUE"` 同值。
- 拒绝：空串、首尾或内部空白、显式长度的嵌入 NUL、非 ASCII 输入、未知名称（返回
  `nullopt`）。不扩展为完整 CSS 颜色语法（不支持 `#`、`rgb()`、`color()` 等）。
- `nan_named_color_name` 返回规范小写拼写；`enum → 名称 → 字符串查询 → 同一颜色`
  往返成立。
- 枚举序号越界的防御值不构成公开契约（良好调用方只传声明值）。

### 18.4 推荐使用方式与非目标

- 命名颜色是**显式 `NanColor` 的写法**：主题作者可以合法地把 `steelblue` 赋给
  `primary`，组件仍通过语义角色消费；默认配方不得直接硬编码颜色。
- 不新增语义 token、不改组件默认配方、不改变既有样式优先级。
- 本步不做 TOML 字符串简写（与 `$palette.*` / `$palettes.*` 的分派另行定形）；
  不覆盖 `transparent`/`currentColor`/完整 CSS 语法。

### 18.5 第 4c 步实施记录（2026-11-08，初稿待维护者复核）

- 新增 `foundation/named_color.{hpp,cpp}`：148 项枚举 + 类型化/字符串/名称三个入口；
  私有 `constexpr` 表按名称字母序，RGB 为规范快照整数原值，别名各保留身份。
- 测试（`tests/named_color_tests.cpp`，unit）：测试内置**独立**的 148 条「名称 → 期望
  hex」清单逐条比对（不拿生产表遍历自证）；覆盖全部名称/值、`green`/`lime` 等易混值、
  同值别名身份、ASCII 大小写、空白/NUL/非 ASCII/未知/`transparent`/`currentColor` 拒绝、
  往返一致与表项数恰为 148。
- 头文件按 experimental 分类；TOML 简写与参考色阶规划保持独立。

故障注入（重新编译后恢复，均红转绿）：`green` 误写成 `lime` 值（3 断言失败）、
表缺 `rebeccapurple`（计数与往返多处失败）、查询放宽为 trim 空白（2 断言失败）。

专项：named-color 5 用例 / 1095 断言；composition 7/52、theme-color-entry 14/1243、
theme-scheme 25/357、theme-contrast-diagnostics 15/338、theme-palette-contrast 4/129
保持通过。首版维护者矩阵（复核前，Linux 无显示/GPU 沙箱；每套先编译再测）：buildDir 全量
123 OK / 1 FAIL（sdf-state-probe，无显示设备）/ 1 SKIP（sdf-device 77）；ASan 102 OK /
1 SKIP；no-RTTI 104 OK / 0 SKIP；no-SQLite 99 OK / 1 SKIP；tools 8 OK；
release_check --ci 8/8（headers 198、docs 87、clang-format 23.1.1）。
本批保持未提交，待维护者复核。


### 18.6 第 4c 步复核记录（2026-11-08，保持未提交）

Git 基线 `3e2542a` 已包含 4b；先编译通过 named-color 与 theme-color-entry 基线。
以独立离线参考清单对照，私有表与测试期望清单的 148 个名称及 RGB 原值一致；运行查询
与转换实现未发现缺陷，未修改 `named_color.cpp`。该轮原文在线复核未完成，后续已通过
本地 SVG 原文与 CSS Color 4 固定快照补验（§18.7），来源证据缺口已关闭。

发现测试盲点：原类型化用例通过整数强转枚举，未实际绑定枚举成员。临时交换头文件
`green`/`grey` 声明后，旧测试仍 5 用例 / 1095 断言全绿。现将独立期望清单逐项绑定
`NanNamedColor::<name>`，直接验证成员的名称、颜色与往返；同样交换会触发 6 断言失败。
删除自查 `contains`，实际验证 148 个名称与枚举身份唯一、名称严格递增；9 组同值别名
直接使用独立枚举成员并核对各自名字，避免只比较测试字符串。

补充全表转换回 RGBA 字节与不透明性、148 名称的全大写/交替大小写查询、非 NUL 结尾的
有界 `string_view`、每个 0–255 字节在名称首/尾/中间的拒绝、长未知名和大小写排除词。
头文件澄清 transparent 是透明黑、currentColor 才依赖上下文，以及枚举入口只接收声明值；
未将非法枚举防御返回值升级为公开契约。

独立故障注入（逐项重编译，恢复后通过）：枚举 green/grey 互换 6 断言失败、green 改为
lime RGB 3 断言失败、rebeccapurple 表名失配 1 断言失败、放宽 trim 空白 2 断言失败，
均为 Catch2 exit 42。专项：named-color 6 用例 / 2916 断言；composition 7/52、
theme-color-entry 14/1243、theme-scheme 25/357、theme-contrast-diagnostics 15/338、
theme-palette-contrast 4/129 保持通过。

最终矩阵：主套件 122 OK / 2 FAIL / 1 SKIP（两个失败均无显示设备）；ASan
102 OK / 1 SKIP，no-RTTI 103 OK / 1 SKIP，no-SQLite
99 OK / 1 SKIP，tools 8 OK；release_check --ci 8/8
（headers 198、docs 87、clang-format 23.1.1）。各套 SKIP 均为 window-lifecycle；
SDK 打包与 vendored 消费者通过。日志与原文补验见 reference_draft 第 4c 步复核记录；
当时保持未提交，随后已随 `f383201` 统一提交推送。
TOML 简写、语义 token、默认配方均未改动；真实像素读回与配色注册门槛仍未完成。


### 18.7 第 4c 步规范原文补验（2026-11-08，保持未提交）

维护者保存于 `/tmp/vscode` 的原文为 SVG 1.1 Second Edition §4.4，直接从原文表格
提取 147 项名称/十进制 RGB，与生产表、测试清单全部一致。随后环境恢复，通过 HTTPS
实际取得 §18.1 约定的 CSS Color 4 固定快照（HTTP 200），确认文档日期为 2026-11-03、
§6.1 Named Colors；原文表格的 148 个十六进制与十进制 RGB 逐项相符。

CSS 表与 SVG 表的 147 个共有项完全一致，仅增加 `rebeccapurple = #663399`；生产表与
独立测试清单均与 CSS 原文 148 项键值完全一致，枚举成员名/数量/字母序亦相符。原文哈希、
提取脚本与结构化结果见 reference_draft 第 4c 步规范原文补验记录。

此前规范来源原文核对的证据缺口已关闭；无需修改运行代码。本次只更新证据和文档，不将
前轮 C++ 矩阵冒充为本轮重跑结果。代码复核、自动验证和规范来源核对均已完成；整批仍
当时保持未提交，随后已随 `f383201` 统一提交推送。GPU、Windows 与视觉验收仍属独立边界。

---

## 19. 第 4d 步 TOML 命名颜色字面量：API 定形（2026-11-08）

4c 已把 CSS Color 4 命名颜色做成 foundation 字面量；本步只把同一写法接到文件导入侧，
不新增第二套颜色模型，也不扩展引用语法。

### 19.1 契约：增加一种字面量写法，保留既有引用能力

- **命名颜色是字面量写法**：当前接受颜色数组 `[light, chroma, hue(, alpha)]` 的字段，
  同时接受 `"SteelBlue"` 这类命名字符串；解析统一调用 `nan_named_color()`，因此大小写、
  空白、嵌入 NUL、非 ASCII 与未知名称规则与 C++ 入口完全一致。
- **引用语义不变**：`$palette.*` 继续是语义 token 引用（配方颜色字段），`$palettes.*`
  继续是参考色阶取值（palette 覆盖）。引用匹配失败立即报错，**不回退成色名**。
- **不扩展引用形式**：palette 覆盖与 Button/TextField 配方的既有引用范围不同，本批只加
  命名颜色字面量，不顺带让每个颜色字段接受所有引用形式。
- **优先级不变**：palette 覆盖中 TOML 同时给规范名与兼容别名时仍以规范名为准；C++ 批量
  覆盖仍按声明顺序。
- **错误与原子性**：错误信息带字段路径（如 `themes.author.palette.primary`、
  `palettes.brand.primary.50`）；任一颜色/引用失败使 `parse_style_document` 返回错误，
  不产生可 `apply` 的文档，因此不会修改活动目录、选择或样式。

### 19.2 解析入口对照

| 字段位置 | 颜色数组 | 命名颜色 | 既有引用 |
| --- | --- | --- | --- |
| `[palettes.<name>.<scale>] <shade>` | 接受 | 接受（`"White"`） | 不接受（保持） |
| `[themes.<name>.palette] <role>` | 接受 | 接受 | `$palettes.<p>.<scale>.<shade>` |
| Button / TextField 规则颜色字段 | 接受 | 接受 | `$palette.<role>` |

### 19.3 本步不做

TOML 侧 `transparent`/`currentColor`、完整 CSS 颜色语法（`#hex`/`rgb()`）、新增引用形式、
注册配色门槛与真实后端像素读回（另行排期）；Shell 多方案设置随后单独推进。

### 19.4 第 4d 步实施记录（2026-11-08，复核完成并随本批提交）

- `style_document.cpp` 增加 `literal_color()`：颜色字面量字段接受命名颜色字符串
  （经 `foundation::nan_named_color`），数组路径保持不变；`$` 前缀不作为字面量。
- palette 覆盖：`$palettes.*` 仍由 `palette_reference` 解析并立即报错（不回退成色名）；
  其他字符串走命名颜色；错误信息带字段路径（`themes.<name>.palette.<role>`、
  `palettes.<name>.<scale>.<shade>`）。配方颜色字段：`$palette.*` 保持语义 token，
  其他字符串先查命名颜色；不扩展其他引用形式。规范名优先规则不变。
- 引用 scale 的每个档位也可用命名颜色字面量，错误带完整档位路径。
- 测试（`tests/style_document_named_color_tests.cpp`，unit）：文件 palette/参考色阶/配方
  字段的命名颜色、`$palettes.*` 与原引用语义、`$palette.*` token 保持、未知名称/未知
  palette/畸形引用错误、字段路径断言、失败导入不修改活动 manager、规范名优先。
- 故障注入（重新编译后恢复）：palette 覆盖拒绝命名颜色（3 断言失败）、配方字段拒绝
  命名颜色（1）、错误丢失字段路径（2）、别名优先破坏规范名优先（1）。
- 专项：style-document-named-color 5 用例 / 50 断言；named-color 6/2916、
  theme-color-entry 14/1243 等既有专项保持通过。首版维护者矩阵（复核前，Linux 无显示/GPU
  沙箱；每套先编译再测）：buildDir 全量 124 OK / 1 FAIL（sdf-state-probe，无显示设备）/
  1 SKIP（sdf-device 77）；ASan 103 OK / 1 SKIP；no-RTTI 105 OK / 0 SKIP；
  no-SQLite 100 OK / 1 SKIP；tools 8 OK；release_check --ci 8/8（headers 198、
  docs 87、clang-format 23.1.1）。本批保持未提交，待维护者复核。


### 19.5 第 4d 步复核契约补充（2026-11-08，随本批提交）

配方颜色错误使用含 0 起始数组下标的路径（如 `styles.button[1].foreground`、
`styles.text_field[1].placeholder`）；palette 统一校验的非有限错误同样保留完整输入路径，
包括作者使用的别名拼写，而不只输出 role 名。此项只完善错误定位，不改变引用支持范围、
颜色规范化或校验策略。


复核基线为 `f383201`，四专项先编译通过。新增回归先发现两类路径缺口（5 断言失败）：
配方错误只输出字段名、不带规则位置；palette 非有限值通过统一校验返回时丢失主题路径。
现分别补齐 `styles.<kind>[index].<field>` 和 `themes.<name>.palette.<input-role>`，
别名路径保留作者拼写，运行引用范围/颜色规范化/提交顺序不变。

补充全部 148 个色名的交替大小写在 palette、primitive 档位、Button/TextField 字面量中的
等价性；10 个配方颜色槽位分别验证命名值/颜色数组/token 三种输入，token 在换 palette 后
仍延迟解析；四种位置的空白、TOML 转义 NUL/非 ASCII、未知名/保留字/错误类型拒绝；
跨位置引用与错误 scale/shade/额外分段的拒绝。删除只在失败 parse 后创建空 manager 的
自证检查，改用实际导入入口：先应用具名色文档，再经历 parse 失败、未知主题与缺失家族
成员的 apply 失败，验证目录内容/版本、选择、revision、样式与生效快照身份全部不变。

独立故障注入每次重编译：去掉 Button 前景路径 1 断言失败、去掉 palette 校验路径前缀
3 断言失败、把 token 提前解析为字面量 1 断言失败，均 Catch2 exit 42；恢复后六个专项
通过。style-document-named-color 12 用例 / 1895 断言；named-color 6/2916、
theme-color-entry 14/1243、button 33/221、theme-scheme 25/357、theme-contrast-diagnostics
15/338 保持通过。

本轮最终矩阵：主套件 126 OK（无失败/跳过），ASan 103 OK / 1 SKIP，
no-RTTI 105 OK，no-SQLite 100 OK / 1 SKIP，tools 8 OK；
release_check --ci 8/8（headers 198、docs 87、clang-format 23.1.1）。现有 SDF 两项
设备/状态检查在 Vulkan/RADV 环境真实运行并通过；该结果不替代 4a 专项合成读回、
Windows 或人工视觉验收。SDK 打包和消费者通过。实际日志与首版环境差异见
reference_draft 第 4d 步复核记录；本批复核完成并统一提交。

后续顺序：4d 维护者复验与统一提交 → Shell 多方案使用闭环（页面正文由维护者负责）→
4a 软件 RenderDevice 读回补验；软件结果不代替 GPU 实测，不与注册配色门槛接线绑定。
