# 数据组件：选择模型与虚拟化依据（P4.2 阶段 5 第一批）

> 状态：**2026-11-03 初稿已实施；2026-11-04 维护者复核完成（7 项修复 + 测试缺口补齐），并统一提交推送于 `e6d67e1`。**
> 本文件是阶段 5 数据组件的「选择 / 虚拟化依据」契约：先定两块可复用的基础，
> 再让复合数据组件在它们之上成长。静态 `Table`、`DataTable`、`Accordion` 等
> 阶段 5 组件清单仍按 [组件开发路线图](component_roadmap.md#阶段-5信息组织与数据展示)
> 分批推进，不在本批。

## 1. 问题与目标

阶段 4 菜单族把「选项模型 + 键盘漫游 + typeahead」收敛成了 `MenuItem` /
`MenuSelection` / `RovingFocus`，但数据展示与菜单的形态不同：

- 菜单条目是**对象**（含 kind/disabled/checked），数据行是**记录**，稳定身份是行键；
- 数据列表可能成千上万行，不能把全部行控件挂进场景树；
- 行需要点击 / shift 范围 / ctrl 多选与键盘漫游，但选中状态不应散落在各组件里。

本批交付两块依据与一个合成组件：

1. **`widget::SelectionModel`**：以稳定 Key 为身份的选中状态（单选/多选、范围、
   锚点、数据替换保留），纯逻辑、不依赖窗口，可脱离场景树单测；
2. **`widget::VirtualListView`**：统一行高的虚拟化数据列表，只物化视口内可见行，
   滚动窗口移动时按 key 复用行控件，直接消费 `SelectionModel` 与 `RovingFocus`；
3. 一套针对虚拟化窗口、滚动、点击/键盘选择、绑定与数据替换的回归测试。

## 2. 与既有设施的分工

| 设施 | 本批的关系 |
| --- | --- |
| `ForEach` | 键控对账内核。`VirtualListView` 内部持有一个 `ForEach` 窗口区域，只把**可见窗口**交给 `region->set_items()`；全量数据保留在列表自己的 `source_items_` 中 |
| `ListView` | 仍是 ForEach 的全量 typed 包装；不虚拟化。`VirtualListView` 复用其 `ListDataModelSource` 绑定概念 |
| `ScrollView` | `VirtualListView` 自己是滚动视口（垂直单列），不套 ScrollView：父布局给的盒子就是视口，偏移驱动窗口重算 |
| `RovingFocus` | 键盘漫游：方向键/Home/End/PageUp/PageDown/typeahead 全部复用；`VirtualListView` 只落地 Intent |
| `MenuSelection` | 菜单专用（checkbox/radio kind、disabled 规则、`MenuItem::checked` 唯一事实来源）。`SelectionModel` 面向数据行：身份是键、承担范围选择与 retain |
| `SelectionModel` | 本批新增，纯逻辑；任何阶段 5 数据组件（Table 行选择等）都可直接消费 |

## 3. SelectionModel 规则

`template<std::equality_comparable Key> class SelectionModel`（[selection_model.hpp](../../nandina/widget/selection_model.hpp)）。

- **身份即键**：`selected()` 返回首次选中顺序的 `std::vector<Key>`；范围选择按
  `ordered` 参数中的行序展开。
- **模式**：`single` 任何 select/范围都只保留一个成员；`multiple` 成员独立累加，
  范围选择**替换**为跨度内全部成员（普通点击 = 独占选中的语义由此获得）。
- **幂等与事件**：状态无变化的操作返回 false 且不触发 `changed()`；`changed()` 无载荷，
  订阅方重新读取 `selected()` / `contains()`。幂等重复的范围选择是个例外：选中集合不变、
  不触发事件，但锚点仍更新为本次落点（to）。
- **范围与锚点**：`select_range(ordered, from, to)` 含两端，未知键不修改任何状态；
  `anchor()` 记录最近一次且仍被选中的落点，供 shift 扩展使用。`ordered` 的键必须唯一
  （`VirtualListView::set_source` 已保证）；违约时 `select_range` 防御性去重，保持
  「`selected()` 是无重复的身份集合」不变量。
- **数据替换**：`retain(universe)` 裁剪已消失的键并清空失效锚点，触发一次事件。
- **模式切换**：multiple → single 只保留第一个成员，触发一次事件；锚点若指向被裁剪的
  成员则重指保留的首成员（保持「锚点 = 已选中成员」不变量）。
- **键类型**：`Key` 需满足 `std::equality_comparable` 且**可哈希**（`std::hash<Key>` 可用）：
  范围去重与内部索引使用 `unordered_set`/`unordered_map`。

## 4. VirtualListView 规则

`template<typename Item, std::equality_comparable Key, typename NodeT = scene::NanControl>
class VirtualListView`（[virtual_list_view.hpp](../../nandina/widget/virtual_list_view.hpp)）。

### 4.1 虚拟化

- **统一行高**：`set_row_height()`（有限且 > 0）。行控件按 `(row_width × row_height)`
  tight 摆放，行内自然高度不改变行高。**变高行不在本批**：窗口计算与滚动全按等高。
- **窗口物化**：`set_source(items)`（或 `set_model(Source&)` 绑定 tracked 源）只把
  可见窗口 `[first, last]` 交给窗口区域 `set_items`；滚出视口的行销毁、滚回的按 key
  重建（键控对账复用仍在窗口内重叠的行控件）。
- **视口语义**：本列表**是**滚动视口。**视口大小由外部布局决定，列表自身不限制高度**：
  `on_measure` 向父级报告内容高度（行数 × 行高），最终视口 = 父级给列表的盒子
  （显式 `set_height`、`Expanded`、或父级 tight 约束，如 `Expanded` 组合测试所示）。
  Column/Flex 等默认按子级自然高度布局，**不会**替列表决定视口：放在未限高 Column
  里的列表会量到内容高并全量物化。父级给无界高度时整个列表可见，此时物化全部行是
  正确语义（而非错误）。
- **窗口一致性**：窗口变化若发生在遍历期（process/animation/layout/post_layout/paint
  等 `defers_tree_mutation` 相位），物化推迟到 `post_layout`，由第二次布局落位；
  相位空闲（idle）时立即对账。注意单位测试直接派发输入时树处于 idle，走立即路径；
  真实窗口在 input 相位派发输入（见 [nan_window.cpp](../../nandina/app/nan_window.cpp)
  的帧顺序），会走延迟路径。滚动偏移立即更新已有行的位置，新窗口行在下一布局出现。
- **同窗口数据替换**：窗口区间不变但数据代次变化时仍必须推送 `set_items`
  （`source_generation_`/`synced_generation_` 对账）：同数量同 key 换内容走
  `update_` 刷新，同数量换 key 销毁旧行重建新行。
- **遍历期收缩窗口**：窗口在布局/post_layout 相位收缩（如 Expanded 组合下二次布局
  改变视口）会触发 `set_items` 移除行；`ForEach::set_items` 的清理循环用
  `remove_and_delete`（而非 `remove_child`）以避开遍历期抛 `logic_error`。
- **首布局前滚动**：`scroll_to_index`/`select`/`select_range` 在 `height() <= 0`
  （尚未布局）时挂起目标索引（`pending_scroll_index_`），`on_layout` 拿到高度后落位，
  避免按零高度视口把偏移跳到行底、布局后目标行反而滚出视口。

### 4.2 选择与键盘

- **点击**：左键按下选中。ctrl（多选模式）= 切换；shift = 以锚点为起点的范围；
  普通点击 = 独占选中（两种模式都清掉其它行）。点击同时把焦点交给列表。
- **键盘**：列表容器可聚焦；方向键/Home/End/PageUp/PageDown 走 `RovingFocus`
  （single 模式选中跟随活动行，multiple 模式只移动活动行）；方向键按住自动重复，
  `Space`/`Enter` 只在**初次按下**触发（切换选中 / 激活），repeat 不重复提交
  （与 DropdownMenu / Pagination 约定一致）；typeahead 经 `set_label_function()` 接入，
  缓冲在**数据替换**（`set_source`）与**失焦**（FocusLeaveEvent）时清空，避免残留
  过期前缀命中旧标签。标签回调返回 `std::string`（按值拷贝），列表快照进自身缓存
  供 typeahead 匹配——回调写 `return item.label;` 是安全形态，不会产生指向临时
  字符串的悬垂 view（ASan stack-use-after-return，见 §6 红测 10）。
- **程序化**：`select(key)` / `select_range(from, to)` **无条件**滚动到目标行
  （与点击/键盘落点一致；重复同跨度、无选中变化也滚动）。首布局前滚动挂起、
  布局后落位（见 §4.1）。`selection_changed()` / `activated()` 事件与 `set_on_*`
  便捷入口并存。

### 4.3 生命周期

- `set_model` 绑定在退出树或控件销毁时解绑；**退出树后绑定不自动恢复**（与
  `ForEach::bind` 一致）：重新入树需再次 `set_model`。行 effect scope 由窗口区域按行管理。
- 数据替换时选中经 `retain` 裁剪，活动索引由 `RovingFocus::sync` 夹回合法位置。
- **共享所有权**：列表必须以 `shared_ptr` 持有（`create()` 构造）——布局期延迟物化
  用 `shared_from_this()` 挂 post_layout 动作，栈上实例首布局触发 `bad_weak_ptr`
  （与 ScrollView 相同的约定）。
- **键类型**：`Key` 必须可哈希（`std::hash<Key>` 可用）：VirtualListView 内部
  `unordered_map` 按键对账，SelectionModel 的 `unordered_set` 用于范围去重（见 §3）。

## 5. 已知边界（本批不做）

- 变高行、水平滚动、行内交互控件（行默认只展示，点击/键盘选择由列表统一处理）；
- 滚动窗口的节点池回收（当前销毁重建，键控对账只复用窗口内重叠行）；
- 无障碍语义、行级焦点（焦点留在列表容器上）、`ScrollView` 组合形态；
- `Flex::add` 对自身做 loose relayout，因此虚拟列表**不能直接继承 ForEach/Flex**
  （会把自尺寸列表撑到内容高并全量物化）；窗口区域作为子控件吸收这份 relayout。
- 阶段 5 组件（静态 Table、DataTable、Accordion 等）与 DataTable 的排序/选择/虚拟化
  成熟条件，仍按 [组件开发路线图](component_roadmap.md#阶段-5信息组织与数据展示) 分批。

## 6. 验证（2026-11-04，Linux）

- `selection-model`：16 用例 / 82 断言；`virtual-list`：28 用例 / 146 断言。
  覆盖窗口物化、滚动窗口移动与行复用、wheel（双向与上下边界夹取）、点击
  （独占/shift/ctrl、底边命中、release 与右键不选择）、键盘（single 跟随 /
  multiple 切换、PageUp/PageDown 边跳、Enter/Space 只在初次按下、方向键重复）、
  typeahead（跨标签前缀/回落、数据替换与失焦清缓冲）、程序化滚动选择（含重复
  跨度仍滚动、首布局前挂起）、数据替换（同窗口刷新/换键、裁剪选中与活动项）、
  重复键拒绝、绑定源、脱离树构建、Expanded 组合视口、空数据惰性、退出树重入
  绑定不恢复。
- 全量 unit 与门禁见维护者复核后的统一记录（本批仍未提交，等待关闭审阅循环）。
- 红测证据（修复前失败、修复后通过；均为新测试先行红测）：
  1. 初版继承 ForEach：`Flex::add` 的 loose relayout 把列表撑到内容高并全量物化，
     布局期 `remove_child` 抛 traversal 异常。改为包含窗口区域设计后
     "materializes only the visible window" 通过。
  2. 重写后丢失空窗口守卫：`window_last_` 哨兵使行摆放循环越界（vector out-of-range）。
     补守卫后通过。
  3. Space 原为激活；改为切换选中后 "keyboard: selection follows in single, toggles
     in multiple" 的 `is_selected(97)` 断言先红后绿。
  4. 同窗口数据替换：窗口区间相同而 `sync_or_schedule` 早退，`update_` 不生效、换键
     不重建。加 `source_generation_` 代次对账后 "same-window source replacement
     refreshes reused rows / swaps row keys" 由红转绿。
  5. 首布局前 `select(500)`：零高度视口把偏移跳到行底、布局后目标行滚出视口。加
     `pending_scroll_index_` 挂起后 "pre-layout programmatic select" 由红转绿。
  6. 底边点击：`hit_row` 按未物化下一行命中。夹回 `window_last_` 后 "click on the
     bottom edge" 由红转绿。
  7. 重复跨度 `select_range` 不再滚动（与 `select` 不一致）：改为无条件滚动后
     "select_range scrolls even when the span is already selected" 由红转绿。
  8. typeahead 缓冲残留：`set_source` / 失焦未清空，旧前缀命中旧标签。加
     `focus_.reset_typeahead()` 后 "set_source / focus leave resets the typeahead
     buffer" 由红转绿。
  9. 遍历期收缩窗口：Expanded 组合下二次布局把窗口 1000→10，`ForEach::set_items`
     清理循环的 `remove_child` 抛 `logic_error`，析构期 entry scope 空指针 SIGSEGV。
     `ForEach` 改用 `remove_and_delete` 后 "inside Expanded virtualizes" 由红（SIGSEGV）
     转绿。
  10. typeahead 标签悬垂：`set_label_function` 收 `std::function<std::string_view>`，
     自然写法 `return item.label;` 把临时字符串转成 view——ASan 在既有 typeahead
     测试抓到 stack-use-after-return（roving_focus starts_with_ci）。回调改为按值
     返回 `std::string` 并快照进列表缓存后，buildDir-asan 下 28 用例全绿。
- 本批已于 `e6d67e1` 统一提交推送；阶段 5 后续组件可接续推进。Windows/真实 IME 等平台验收不属本批。

## 7. 后续

- 阶段 5 第一批验收关闭后：静态 `Table`（行选择直接消费 `SelectionModel`）、
  `Accordion`/`Collapsible`、`Resizable`/`AspectRatio`，再按成熟度评估 `DataTable`。
- 作者 DSL 入口（`ui.list(...)` 类）与 showcase 视觉验收在复合数据组件批次接入。
