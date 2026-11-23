# Dialog 迁移契约

Dialog 是统一浮层设施迁移的最后一个阶段 2 组件，用于验证**模态**场景：遮罩阻断、Escape 与点击外部关闭、焦点限制，以及关闭后的焦点恢复。迁移目标是替换内部的呈现与焦点实现，不改变现有应用层构造方式。

## 公共 API

以下接口保持兼容：

- `Dialog::create(theme)` 与 `ComponentTraits<Dialog>::make(ui, title, content)`；
- `set_title()` / `title()`；
- `set_content()`；
- `open()` / `close()` / `is_open()`；
- `set_dismissible()` / `dismissible()`；
- `set_on_close()`；
- 主题覆盖、文本管线与语义接口。

新增命名槽位（[组件公共契约](component_contract.md) 第 4 节）：

- `set_header(std::shared_ptr<NanControl>)`、`set_footer(std::shared_ptr<NanControl>)` 与已有的 `set_content()` 构成三个固定语义区域；
- `set_title(std::string)` 保留为 header 槽位的文本便捷入口：标题文本与 `set_header()` 互斥，后设置者生效；
- 槽位缺省表现为空，不占用高度；header 与 content 之间、content 与 footer 之间由 recipe 的 `gap` 分隔。

Dialog 仍然是组件而非页面：页面作者不需要直接创建 `OverlayHost`，组件从所属窗口的 `BuildContext` 获取窗口级浮层服务。

## 内部职责

Dialog 自身退回为「面板内容 + 淡入淡出状态机」，把以下职责交给浮层设施：

```text
OverlayHost / overlay layer
└── DismissLayer            模态阻断 + 点击外部 / Escape → on_close
    └── FocusScope          初始焦点 + Tab 循环 + 卸载时焦点恢复
        └── DialogPanel     header / content / footer
```

| 现有实现 | 迁移后 |
| --- | --- |
| `z_index_hint()` 提升绘制序 | 浮层承载时用 overlay layer 的层级顺序；树内回退保留 |
| `contains_point()` 恒真吞掉遮罩命中 | `OverlayOptions::block_below` |
| `global_bounds()` / `on_measure()` 撑满父容器 | 浮层承载时交给 overlay surface；树内回退保留 |
| `trap_focus()` 自研 Tab 循环 | `FocusScope` |
| Escape / 面板外点击判定 | `DismissLayer` |
| `open()` 中的 `request_focus()` | `FocusScope` 的初始焦点 |
| 关闭后的焦点恢复（缺失） | `FocusScope::on_exit_tree()` |

因此 `contains_point()`、`is_focusable()`、`global_bounds()` 三个 override 以及 `trap_focus()` 会被移除；它们不是应用层接口，对应断言改为对浮层行为的断言。`on_measure()` 与 `z_index_hint()` 是虚函数而非应用层接口，保留下来服务于树内回退。

### 焦点落点与 Escape 的可达性

`DismissLayer` 与 `FocusScope` 都通过 `on_input_capture` 生效，而按键的派发路径是「从当前焦点节点沿祖先链向上冒泡」。因此**只有焦点位于 FocusScope 内部时，Escape 才可能到达 DismissLayer**。

现状有一个必须补上的缺口：`NanSceneTree::focus_first_within()` 在作用域内没有可聚焦控件时返回 `false` 并且不改动焦点，焦点会留在浮层之外，Escape 与 Tab 都进不了浮层。典型场景是只有标题和一行说明、没有任何控件的纯提示对话框；现有实现靠 `is_focusable() == active()` 让 Dialog 自己成为焦点来规避它。

因此 `FocusScope` 需要保证焦点一定落在自己内部：

- 优先聚焦内部第一个可聚焦控件；
- 内部没有可聚焦控件时，回退为把焦点落在作用域自身，使 Escape 与 Tab 仍然沿浮层路径派发。

注意兜底不能简单地把 `FocusScope::is_focusable()` 改成恒真：`_collect_focusable_nodes()` 会把作用域自身排在子节点之前，那样 Tab 会停在不可见的容器上而不是第一个控件。兜底应由 `on_ready()` 在 `focus_first_within()` 找不到控件时显式执行，具体形态在实现时确定。

这属于共享设施的能力补齐，在 Dialog 迁移内一并完成。

### 浮层层级

Select 与 Tooltip 使用 `present()` 的默认层级，模态内容必须位于它们之上。组件不应直接写 CanvasLayer 的 order 数值（[浮层架构](overlay_architecture.md) 的叠放规则），因此 `OverlayOptions` 的 `order` 改为语义层级：

```cpp
enum class OverlayLevel : int {
    popup = 0,   // 提示、下拉、菜单
    modal = 100, // 模态对话框：高于 popup，并阻断其下所有输入
};
```

Dialog 以 `{.level = OverlayLevel::modal, .block_below = true}` 呈现。Select 与 Tooltip 取默认值，行为不变。

### 关闭动画与 portal 生命周期

现有 `close()` 是「启动淡出 → 动画结束后隐藏并触发 `on_close`」，`is_open()` 在淡出期间即为 `false`。迁移后 `OverlayHandle` 必须晚于淡出释放，否则面板会在半透明状态下消失：

- `close()`：`is_open()` 变 `false`，启动淡出，**保持挂载**；
- 淡出完成：释放 `OverlayHandle` 并触发 `on_close`；
- `open()`：在已有挂载时幂等；淡出未完成时再次 `open()` 复用同一份挂载并反向淡入；
- `on_exit_tree()` 与析构同样释放 `OverlayHandle`。

### dismissible 语义

`DismissLayer` 的回调统一走 `RequestClose` 判定：

- `dismissible() == true`：关闭，与现有行为一致；
- `dismissible() == false`：**不关闭，但吞掉该次 Escape / 点击**。模态对话框不应把按键泄漏给下层，这比现状（`return false` 放行 Escape）更严格，属于有意变更。

## 验收条件

- Dialog 位于 `ScrollView`、`Card` 等裁剪容器内时，遮罩与面板覆盖整个视口而非父容器；这是当前实现无法满足的核心缺陷；
- `block_below` 阻断其下所有输入：打开期间下层按钮、Select、Tooltip 触发器都不响应；
- Dialog 内再开 Select（浮层套浮层）时，弹层压过模态遮罩且选项可点，Tab 仍在 Dialog 的 `FocusScope` 内循环；
- Escape 与点击面板外经 `DismissLayer` 关闭；`dismissible == false` 时两者都不关闭；
- Tab / Shift+Tab 在 Dialog 内的控件间循环，首尾相接；打开期间焦点不会落到浮层之外；
- 只含标题与说明（内部无可聚焦控件）的 Dialog 打开后，焦点仍位于浮层内部，Escape 可以关闭它；
- 关闭后焦点回到打开前的控件；打开前无焦点时回到 `nullptr`，不残留悬空焦点；
- 淡出期间遮罩、面板与内容持续可见，`is_open()` 为 `false`；淡出完成后才卸载并触发 `on_close`；
- 打开动画、`set_override()` 的配方覆盖与亮暗切换保持现有测试语义；dialog role 与 label 仍然存在，但改由面板报告（见「实现状态」）；
- 注入的窗口 `OverlayHost` 先于 Dialog 销毁时，弱服务引用安全失效；
- **无窗口、无 OverlayHost 的 detached 上下文仍以树内模态方式工作**：现有 `tests/dialog_tests.cpp` 的 10 个用例不依赖浮层即可通过。

## 实现状态

迁移已完成，实现落在以下结构上：

```text
浮层承载（有 OverlayHost）
OverlayHost / overlay layer
└── DismissLayer          全屏遮罩与阻断、点击外部 / Escape → close()
    └── FocusScope        初始焦点、Tab 循环、卸载时焦点恢复
        └── DialogPanel   header / content / footer

树内回退（无 OverlayHost）
Dialog（页面锚点，打开时铺满父容器）
└── 同一套 DismissLayer → FocusScope → DialogPanel
```

与契约的差异与补充：

- `z_index_hint()` **保留**：浮层承载时层级由 `OverlayLevel` 决定，但树内回退仍要靠 z 序压过后续兄弟，因此只在非浮层模式且打开时返回 1。`contains_point()`、`is_focusable()` 与 `trap_focus()` 按契约移除；
- 槽位由 `DialogPanel` 直接持有，**不存在节点搬移**：`Dialog` 节点从不把槽位挂到自己名下，`DismissLayer` 子树在浮层模式下被 `present()`、在回退模式下作为 `Dialog` 的子节点，承载方式在首次打开时确定。这样避免了在 deferred 阶段对活动子树做 `remove_child`；
- dialog 语义从 Dialog 节点移到 `DialogPanel`，并且 Dialog 节点在浮层承载时保持不可见。原因是节点一旦可见，父级 `Column` 就会为这个零高子节点多算一个 gap，把后面的内容整体推下去；而语义系统会跳过不可见节点。面板在两种承载方式下都可见，边界就是面板边界，比零尺寸的锚点更准确。代价是 `dialog open toggles z-order, focusability and semantics` 的断言改为从 `semantics_tree()` 里按面板的 `semantics_id()` 查找；
- 新增 `OverlayLevel::nested_popup` 与 `OverlayHost::hosts_node()`：模态内部再展开的提示与下拉必须压过模态遮罩。迁移前 Dialog 在树内绘制，Select 弹层天然在它之上；改为浮层承载后若不抬高层级，弹层会被遮罩埋掉且点不到；
- 面板居中改为 `DismissLayer::set_content_centered(true)`，由遮罩层在自己的布局里把内容居中。契约原先设想由 `Dialog` 计算位置，那样依赖 `viewport_size()` 在打开时已经有效，窗口首帧前打开会把面板留在原点；
- `FocusScope` 的兜底形态：`is_focusable()` 返回「内容子树里没有可聚焦控件」。`_collect_focusable_nodes()` 会把作用域自身排在子节点之前，恒真的 `is_focusable()` 会让 Tab 停在不可见的容器上；
- 淡入淡出移到了 `DismissLayer`：它是遮罩与内容的共同祖先，`local_opacity()` 一次作用于整个模态子树，遮罩与面板不会各自动画。

实现顺序与落点：

1. `scene`：`OverlayOptions::order` → `level`（`OverlayLevel { popup, modal }`），Select / Tooltip 取默认值；
2. `widget::internal`：`FocusScope` 补上焦点兜底；
3. `widget::internal`：`DismissLayer` 增加遮罩、淡入淡出与内容居中；新增 `DialogPanel`；
4. `widget::Dialog`：改为 portal 托管，保留 detached 回退；
5. `ComponentTraits<Dialog>` 注入浮层服务，`create()` 回退到最近的祖先 `OverlayHost`；
6. 测试：现有 10 个用例保持语义，新增浮层托管、裁剪容器逃逸、模态阻断、遮罩 / Escape 关闭、`dismissible` 吞输入、焦点恢复与重复开关用例；
7. `docs/components/dialog.md` 记录应用层用法。

## Sheet（贴边模态抽屉，v1）

> 状态：2026-11-22 实现 + 验证。**复用统一模态浮层设施与 `DialogRecipe` / `DialogRecipeRule`**；
> 支持 `SheetSide { top, right, bottom, left }`（默认 `right`），在桌面端统一覆盖侧边/顶底
> Sheet 与 Drawer 场景，不另拆重复的 `Drawer` 组件。

### 1. API 与归属

- 公开头：`<nandina/widget/sheet.hpp>`（`experimental`），实现：`nandina/widget/sheet.cpp`（内部面板 `internal::SheetPanel` 封装于 `.cpp`，不新增 `internal/` 头）。
- 方向与主轴厚度：
  - `enum class SheetSide { top, right, bottom, left }`（默认 `SheetSide::right`）；`set_side(SheetSide)` / `side()` 对非法枚举值抛 `std::invalid_argument`。
  - `set_extent(float)` / `clear_extent()` / `has_explicit_extent()` / `extent()`：控制抽屉主轴厚度（`left`/`right` 为面板宽度，`top`/`bottom` 为显式面板高度）；非有限或 `<= 0.0F` 抛 `std::invalid_argument`。未显式设置时，`left`/`right` 默认取 `resolved_style().metrics.panel_width`（`360.0F` DIP），`top`/`bottom` 默认取三槽位自然总高（不低于 `metrics.min_height`）。
    `extent()` 返回显式值；未显式设置时返回 recipe 的 `metrics.panel_width` 配置回退值。`top`/`bottom` 清除显式值后的实际自然高度属于布局结果，不承诺等于 `extent()`。
- 标题、说明与三槽位：
  - `set_title(string)` / `title()`、`set_description(string)` / `description()` 为内置 header 区域的便捷入口（标题单行省略，说明最多 4 行自动换行，二者间距 `4.0F`）。
  - `set_header(control)` / `header()`、`set_content(control)` / `content()`、`set_footer(control)` / `footer()`：自定义 `set_header` 与内置 `set_title`/`set_description` 互斥，后设置者生效；槽位校验非空、未挂载在其他父节点下，且拒绝自引用或包含所属 `Sheet` 的祖先环。
- 生命周期与事件：
  - `open()` / `close()` / `set_open(bool)` / `is_open()`、`set_dismissible(bool)` / `dismissible()`（默认 `true`）。
  - `set_on_close(callback)` 与 `closed() -> const reactive::Event<>&` 共用同一事件源，在退场淡出结束、浮层卸载并恢复焦点后触发；触发期间以 `weak_from_this().lock()` 保活，允许回调释放最后一个外部 `shared_ptr<Sheet>`。
- 作者入口：`ComponentTraits<Sheet>` 支持 `ui.make<Sheet>(title, content)` 与 `ui.make<Sheet>(side, title, content)`，自动注入主题与窗口 `OverlayHost`，并支持 builder `.open(bool)`；收录入 `showcase/component_catalog.hpp`（公开组件数 44）。

### 2. 浮层承载、几何布局与配方复用

- 承载链路与 `Dialog` 同构：`DismissLayer → FocusScope → SheetPanel`（`OverlayLevel::modal`，`block_below = true`；无 `OverlayHost` 时回退为树内模态）。`DismissLayer` 新增 `set_origin_resolver(...)` 按 `SheetSide` 与面板测量尺寸计算贴边原点：
  - `right`：`(max(0, W - panel_w), 0, panel_w, H)`
  - `left`：`(0, 0, panel_w, H)`
  - `top`：`(0, 0, W, panel_h)`
  - `bottom`：`(0, max(0, H - panel_h), W, panel_h)`
- 面板内部纵向布局：`header` 置于顶部内边距处；`footer` 贴齐面板底部内边距（`max(after_content_y, panel_h - pad_y - footer_h)`）；`content` 获得 `header` 与 `footer` 之间的剩余可用高度（子项设 `height(fill)` 或 `ScrollView` 时自动填满中段）。
- 主题与配方：复用 `DialogRecipe` / `DialogRecipeRule`；`Sheet::resolved_style()` 默认将 `panel.radius` 置为 `0.0F`（贴边直角），若作者通过 `set_override(DialogRecipeRule)` 显式提供 `panel_radius` 等字段则按规则覆盖。
- 无障碍：浮层模式下页面内 `Sheet` 锚点保持不可见且不占父布局 `gap`；可见面板节点暴露 `semantics::Role::dialog` 与 `.label = title()`。抽屉内打开的 `Select` 等弹层自动走 `OverlayLevel::nested_popup` 浮于遮罩之上。

### 3. 验证记录（2026-11-22）

- `sheet` 单测（`tests/sheet_tests.cpp`，注册于 `unit` 套件）：11 用例 / 118 断言，覆盖 `DialogRecipe` 复用与直角默认/覆盖、`set_extent`/`clear_extent`、四边停靠几何（`right`/`left`/`top`/`bottom`）、`footer` 贴底与 `content` 剩余高度分配、`set_header` 与 `set_title`/`set_description` 互斥切换、裁剪容器逃逸与下层输入阻断、遮罩点击/Escape 关闭与 `dismissible=false` 吞输入、Tab/Shift+Tab 焦点陷阱与关闭恢复、`closed()` / `set_on_close` 回调内释放最后一个 owner、面板 `Role::dialog` 语义与父容器零额外间距、抽屉内嵌套 `Select` 弹层、非法 `side`/`extent`/空槽位/已挂载节点/祖先环、跨槽复用事务性拒绝，以及关闭后退出树不再恢复陈旧焦点。
- 红先：将 `Sheet::install_origin_resolver` 中 `SheetSide::right` 的 x 坐标固定为 `0.0F` 或移除 `Sheet::resolved_style()` 的 `style.panel.radius = 0.0F` 默认直角设置，均使 `sheet_tests` 对应断言失败（退出 42），恢复后全矩阵通过。
- 下游 SDK：`tests/fixtures/sdk_app/main.cpp` 新增 `sheet-api=1` 探针。
