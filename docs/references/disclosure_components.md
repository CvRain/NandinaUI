# 折叠内容组件（P4.2c Collapsible）

> 2026-11-05：精确契约与实现已随 `095336b` 统一提交推送；**2026-11-13 Showcase 视觉验收
> 完成**（收起/展开/嵌套/Signal 同步/禁用共 6 个实例，双平台 CPU+SDF 8 条 golden 批准；
> 页面测试 `[collapsible][showcase]` 断言语义计数与 1200×800、640×800 布局收敛）。
> Accordion 后续消费本批折叠内容基础；本批不实现分组互斥、方向键漫游或过渡动画。

## 1. API 与归属

- `widget::Collapsible(text, theme)` / `Collapsible::create(text, theme)`，默认收起。
  `ui.make<Collapsible>(text)` 与 `ui.make<Collapsible>(Signal<bool>&, text)`；
  builder `.child(...)`、`.open(bool)`、`.on_change(handler)` 和 `.gap(float)`。
- `set_child` / `child` / `clear_child` 管理单个内容节点；必须非空、未挂载、无父级循环、
  无锚点约束。同内容重复设置无效。替换失败保留原内容，替换通过既有树变更通道执行。
- `set_open(bool)` 是静默程序同步；`open()` 查询状态。`toggle()` 表达用户操作，
  disabled 时拒绝，真实变更发 `set_on_change(function<void(bool)>)` 与 `open_changed()`。
  回调和事件载荷均为本次动作结果，回调重入不会修改同一次事件的载荷。
- `set_text` / `text` 修改非空触发器标签；`set_disabled` / `disabled` 仅限制触发器与 toggle，
  已展开的内容仍按自己的 disabled 状态交互；程序化 set_open 不受限制。
- `set_gap` / `gap` 默认零，有限非负。`trigger()` 返回只读 Button，供查询尺寸/语义身份；
  `set_trigger_override(ButtonRecipeRule)` 与 container/label visual part 沿用 Button 覆盖。
- 组合属于 widget：Collapsible 是 NanControl，内部 Button 处理真实输入与焦点提示，
  内容宿主隔离 visibility。semantics 只增加平台无关 `State::expanded` 可选值，
  merge_descendants 沿用 checked/selected 的可选状态合并规则（父显式值优先）；
  不新增 scene 对 widget 的依赖、不新增折叠组件颜色/配方或专用平台通路。

## 2. 布局与内容生命周期

- 有限宽度时触发器和内容使用相同的紧宽度、内容高度无界，先触发器再内容；
  展开自然高度 = 触发器高 + gap + 内容自然高，收起仅占触发器高。
  无界宽度时自然宽度取两者最大值，再按该宽度测量高度，measure/arrange 保持一致。
- 父级给紧高度时保持两部分自然高度，默认矩形 clip；间距仅在有可见且已展开内容时加入。
  自然尺寸和间距之和须可表示为有限 float，超限在 measure/layout 阶段抛 invalid_argument。
- 收起通过内部宿主隐藏子树，不删除内容、不改写内容自己设置的 visible、不退出树，
  不清理内容的绑定/编辑状态；隐藏内容不测量，不参与命中、Tab 顺序或语义树。
- setter 只标布局/绘制/语义脏，不自行触发 loose relayout；相同视口应收敛。
  退出树仍按各内容节点现有约定清理；重入树不重建子内容。

## 3. 输入、焦点、语义与作用域

- 触发器复用 Button：左键、Enter、Space 和语义 activate 均汇聚到 toggle；
  Enter/Space 的 repeat/release 不重复切换。容器自身不可聚焦，焦点在真实触发器或内容控件。
- 收起时，若实际焦点在内容子树内，先恢复到本树中可用的触发器（restore 原因）；
  触发器 disabled 或不可见时清空焦点。内容子树的 pointer capture 同时释放。
  替换/清空仍聚焦的内容时也恢复触发器；焦点回调若重入替换/清空，较新的内容代次
  优先、旧操作不继续提交。外部焦点不受影响。展开不主动抢焦点。
- 容器暴露 generic 语义与标签，触发器为 button、状态 expanded 与当前 open 同源；
  closed 隐藏内容语义，open 恢复原子树。disabled/label/focused/actions 保留 Button 动态语义。
- open 状态唯一存储在内部共享状态，触发器仅弱引用查询；原生触发回调也有生命期守卫。
  外部保留触发器节点并销毁容器后，不得调用已销毁的 owner。
- Signal 同步沿 BuildContext::bind/connect 的既有页作用域路径，.on_change 由 NodeBuilder
  守卫；页作用域清理后，保留节点仍能本地切换，但不得再写旧 Signal 或调用页面回调。

## 4. 验收与边界

本批要求布局/wrap/收敛、点击和键盘、语义 activate、焦点恢复与 pointer capture、disabled、
绑定/回调作用域、隐藏不退出树、内容替换/销毁、主题与覆盖优先级的独立回归和故障注入。

不包含 Accordion、任意自定义触发器、图标/箭头新绘制、过渡动画、销毁型折叠策略、
通用控件间 ARIA controls/labelledby 关系（语义协议尚无该字段）。Showcase 页面和组件手册
正文由维护者手写；本批只提供实验性 API 核对提纲，不宣称视觉验收完成。

## 5. 初稿验证记录（2026-11-05，Linux）

- 专项 `collapsible`：30 用例 / 165 断言。覆盖自然尺寸/wrap/ScrollView、内容保留与
  隐藏不测量、命中/Tab/语义隔离、左键/Enter/Space/语义 activate、静默 setter 与事件、
  Signal/页面回调作用域、disabled、焦点与 capture 恢复、内容替换/延迟替换/回调重入、
  主题与 typed/visual 覆盖、owner 销毁与保留 trigger、布局收敛和数值边界。
- 复核补充（红测两处，均已先失败后通过）：
  a. `ui.make<Collapsible>(Signal, text)` 的初始值：初稿测试只用初始 false 的 Signal，
     未覆盖 Signal 初始为 true 的即时同步；新增用例在移除 traits 的 `ui.bind` 后
     3 处断言失败，恢复后通过（初始值即时应用、toggle 写回、set 同步、防回环）。
  b. 程序化 `set_open` 不受 disabled 限制（契约 §1 明示）：新增用例在给 `set_open`
     加 disabled 守卫后 2 处断言失败，恢复后通过。
- 故障注入（各项重新编译后运行，均 exit 42，恢复后全绿）：
  1. body visibility 强制为 true → 隐藏子树命中/visibility 断言失败。
  2. 删除 `open_changed_.emit(next)` → 用户事件断言失败。
  3. 触发器 expanded 恒为 false → 展开语义断言失败。
  4. `release_content_input` 直接返回 → 焦点恢复/capture 释放失败。
  5. 强制 gap 为零 → 有限宽度自然高度断言失败。
  6. 关闭 finite_extent 校验 → 溢出拒绝断言失败。
- 新增 State::expanded 时，合并语义路径最初未迁移：merge_descendants 下两处断言失败，
  补齐 `merge_state` 后通过，另验证父显式 false 优先于后代 true。
- 首版 set_child 恢复焦点触发应用回调，重入替换后旧调用继续挂载内容，3 处断言失败；
  用 content_generation_ 守卫后通过，clear_child 同样由较新内容写入优先。
- 最终代码树验证矩阵（每个配置先编译再测试）：

  | 范围 | 结果 |
  | --- | --- |
  | buildDir 全量 | 117 OK / 0 FAIL，含 sdf-device、sdf-state-probe 与 SDK 子项目打包验证 |
  | buildDir-asan unit | 94 OK / 1 skip；启用 ASan/UBSan，`LSAN_OPTIONS=detect_leaks=0` |
  | buildDir-no-rtti unit | 96 OK |
  | buildDir-nosqlite unit | 91 OK / 1 skip |
  | buildDir-tools | 8 OK |
  | release_check --ci | 8/8，无 skip；clang-format 固定 23.1.1 |

  ASan 与 no-SQLite 配置的跳过项均为 window-lifecycle（禁用原生窗口测试）。
  Windows、Showcase 页面和视觉验收未在本轮执行；不由自动测试结果代替。
- P4.2c 已随 `095336b` 统一提交推送；Showcase 视觉验收已于 2026-11-13 完成并批准双平台
  8 条页面基线（详见 [视觉回归](visual_regression.md)）。

## 6. 2026-11-13 展开内容高度回归

Collapsible 自身没有布局缺陷，但 showcase `收起与展开` / `Signal 与禁用` 暴露了父级线性
容器在 measure 阶段不给 flex 子项分配主轴尺寸的问题：`set_open(true)` 的换行内容按整行
宽度测量，最终半行宽度换行后高度少算一行，内容画到 example Card 之外；运行时用户展开
则被截断，看起来像「禁用实例被误展开」（实际是溢出内容覆盖了下一行）。修复在布局层
（`layout.cpp` 的 `measure_linear_container()`，见
[布局收敛与文本测量](layout_convergence_and_text_measurement.md) §10）。

本组件的契约不变：`set_open()` 静默、用户激活才发 Signal、禁用实例忽略激活并保留空内容槽。
新增页面级回归 `[collapsible][showcase][layout]`：900/1200 宽度下展开内容底边必须落在
Collapsible 底边之内；运行时 `toggle()` 后复检；禁用实例 `toggle()` 不改变状态且不请求
布局。Linux `collapsible` CPU/SDF 基线重采（候选），待维护者审阅后批准。
