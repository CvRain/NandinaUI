# 数据组件：选择模型与虚拟化依据（P4.2 阶段 5 第一批）

> 状态：**2026-11-03 初稿已实施；2026-11-04 维护者复核完成（7 项修复 + 测试缺口补齐），并统一提交推送于 `5d91e01`。**
> **2026-11-14：新增 §9 `DataTable` 交互数据表契约草案（未实现、未开工，待维护者审阅并关闭
> §9.7 的开放问题）；§8 静态 Table 与 §3/§4 基础契约不变。**
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
- P4.2a 已于 `5d91e01` 提交；归档验证为主套件 115/115，ASan 0 失败 + 1 预期跳过、no-RTTI 94/94、no-SQLite 89 + 1 跳过、纯工具与发布门禁各 8/8。
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
- 本批已于 `5d91e01` 统一提交推送；阶段 5 后续组件可接续推进。Windows/真实 IME 等平台验收不属本批。

## 7. 后续

- P4.2a 已提交（`5d91e01`）。静态 `Table` 不承担行选择；行选择/排序/虚拟化属于
  后续 `DataTable`。`Accordion`/`Collapsible` 与 `Resizable`/`AspectRatio` 依阶段 5
  组件路线分批。
- `VirtualListView` 作者 DSL 入口与视觉验收随维护者编写的 Showcase 组合另行接入。


## 8. P4.2b 静态 Table：布局契约

> 2026-11-04 实现；2026-11-05 维护者与最终复核完成，代码批次收口。**2026-11-13 Showcase
> 视觉验收完成**：新增 `showcase/pages/table_page.cpp` 覆盖宽/窄列轨道、空表与自然行高四组
> 场景；双平台 CPU+SDF 共 8 条 golden 由维护者批准，页面测试 `[table][showcase]` 断言四表
> 语义计数（table/column_header/cell）与 1200×800、640×800 下的布局收敛与有界滚动根。

本子批交付非交互静态表格。行选择属于后续 DataTable 的
职责：静态 Table 只协调有限数量的控件、共享列轨道和行高，不另存选中状态。数据源绑定、
SelectionModel、RovingFocus、虚拟化与排序留给后续交互数据表；本批不关闭整个 P4.2。

### 8.1 API 与归属

- `widget::Table` 是 NanControl，`ui.make<Table>(columns)` / `Table::create(columns)`
  创建（默认两列）。`TableColumn` 含可选逻辑宽度 `width` 与正数权重 `flex`。
- `set_columns`、`set_header(Cells)`、`clear_header()`、`add_row(Cells)`、
  `set_rows(vector<Cells>)`、`clear_rows()`；Cells 是普通 NanControl shared_ptr 的向量。
  每行必须与列数相同；单元格必须非空、未挂载、在本次操作中唯一且无父级循环。
  有内容时不能改变列数，须先清空表头和正文。非法操作不替换原内容。
- `set_column_gap` / `set_row_gap` 默认零；列宽与 gap 有限非负、flex 有限正数。
  gap 与轨道数的乘积/总和须在 float 内有限（`set_row_gap(FLT_MAX)` 后再加行、或
  带大行距的表加表头/行会抛 `invalid_argument`，失败不改变现有内容）。
  只标布局脏，不在 setter / on_ready 中按 loose 约束抢先布局。
- Table 为布局/语义协调器，不新增配方、颜色或绘制路径；字体与颜色由内容节点的
  StyleContext / Label token 处理，背景、边框和内边距用现有 Surface / Padding 组合。
  ComponentTraits 仅导出普通构造，不要求 Graph 或平台服务。

### 8.2 测量与排列

- 有限宽度：固定列保留逻辑宽度，减掉固定列和列间距后的剩余空间按 flex 权重分给
  非固定列（不足时为零）。固定列超出表格盒子时保持列宽，表格矩形裁剪负责限制外溢。
- 无界宽度：非固定列取表头与所有正文在该列的自然宽度最大值；固定列仍按指定宽度测量。
  有限与无界两种路径均全量测量，不宣称虚拟化。
- 确定共享列宽后，以相同的紧宽度、无界高度测量各单元格；每行自然高度取该行最大值。
  行高不因父级紧高度平均分摊；长文字自然长高。所有单元格安排为整列宽/整行高，
  内容可用自身盒内对齐。排列使用实际指派宽度重新计算列轨道，保证 resize 后表头与正文对齐。
- 单元格与表格默认矩形 clip；表格不滚动。需要滚动时使用既有 ScrollView。
  重复相同视口布局应收敛，子内容变更沿现有场景布局失效链重新测量。
- 自然列宽总和、内容行高与行间距总和必须可表示为有限 float；用 double 累计，
  测量与排列在发布 geometry 前统一检查，超限抛 `invalid_argument`，不将 inf 传播给父布局。
  此检查发生在测量/排列阶段；内容与间距 setter 的即时参数校验仍按 §8.1 执行。

### 8.3 语义与边界

- 添加平台无关 `Role::table / row / column_header / cell`，形成为
  Table → row → column_header 或 cell → 内容节点的语义树；内容按钮等保留自身 action。
  Table / row / cell 本身不可聚焦、不抢键盘、不添加选择/激活动作。
- 行和单元格包装属于 widget 内部实现；semantics 只新增角色值与 role_name，
  scene/render/theme 不认识 Table。行槽位变更使用已有延迟树变更通道。
- 不含排序、选择、数据模型、虚拟化、合并单元格、粘性表头、列拖拽、按内容自适应后再
  限宽的轨道算法。Showcase 页面与使用手册正文由维护者手写，初稿只提供 API 核对表。

### 8.4 验证记录（2026-11-05，Linux）

- `table`：18 用例 / 136 断言。覆盖固定/加权列、有限宽度换行与自然行高、无界自然宽度、
  overflow 裁剪、轨道/单元格验证、失败事务性、锚点拒绝、表头替换、遍历期延迟替换、
  Table/row/header/cell 语义树与单元格 action、收敛、滚动组合、行距溢出校验、
  Builder 槽位入口与 callback 作用域。
- 红测一：自然宽度测试确认测量得到 20/80 列宽后，`on_layout` 按有限宽度重算成 50/50，
  自然宽内容被撑宽且行高变化；保留 `intrinsic_width_` 的 measure/arrange 模式后通过。
- 红测二（本次复核补充）：`set_row_gap(FLT_MAX)` 之前不校验间距总和，三行把测量高度
  推成 inf；新用例在 `require_row_gap_total` 缺失时 6 处断言失败，补齐
  `set_row_gap` / `add_row` / `set_rows` / `set_header` 的有限和校验后通过。
- 红测三（最终复核）：间距独立校验合法，但自然列宽、内容行高或“内容＋间距”之和
  仍可能溢出。新增用例在修复前 4 处断言失败（exit 42），统一以 double 累计并在
  转成 float 前经 `layout_extent` 校验后，18 用例 / 136 断言通过。
- 最终矩阵（先 compile 后 test；Linux，2026-11-05）：

  | 配置 | 结果 |
  | --- | --- |
  | `buildDir` 全量 | 116/116，0 失败、0 跳过；本次 `sdf-device` 与 `sdf-state-probe` 均通过 |
  | `buildDir-asan` unit | 93 通过 + 1 个预期 `window-lifecycle` 跳过；`LSAN_OPTIONS=detect_leaks=0` |
  | `buildDir-no-rtti` unit | 95/95 |
  | `buildDir-nosqlite` unit | 90 通过 + 1 个配置预期跳过 |
  | `buildDir-tools` | 8/8 |
  | `release_check --ci` | 8/8，0 跳过；clang-format 23.1.1（`buildDir/fmt-venv`） |

- 维护者复核时 `sdf-state-probe` 曾报 `SDL_CreateGPUDevice(vulkan): unsupported`；
  最终复核环境中原测试通过，未修改探针、测试断言或跳过策略。
- 本轮不声明 Windows 验证；Showcase 页面、正式视觉验收和组件使用文档正文仍由维护者手写。

## 9. P4.2d DataTable：交互数据表契约（草案，待维护者审阅）

> **状态：2026-11-14 草案。未实现、未开工。** 本节只定形状：API 归属、选择/排序/虚拟化的
> 分工、非目标与开放问题。开工前必须先关闭 §9.7 的 Q1–Q8；与静态 Table 相同，先契约后代码。
> 已交付的基础不在本草案内改动：[§3 选择模型](#3-selectionmodel-规则)、[§4 虚拟列表](#4-virtualistview-规则)、
> [§8 静态 Table](#8-p42b-静态-table布局契约)。

### 9.1 职责与不变量

`DataTable` = 列轨道 + 固定表头 + 行选择 + 排序 + 统一行高虚拟化滚动 + 键盘漫游。
它合成静态 Table 的布局规则与 VirtualListView 的窗口/选择规则。

不变量（与既有契约一致）：

- **静态 Table 不改**：§8 仍是非交互协调器；行选择/排序/虚拟化只属于 DataTable。
- **选择事实唯一**：选中状态由 `SelectionModel<Key>` 拥有；DataTable 不另存第二份。
- **行身份即 Key**：行控件按 Key 键控复用；数据替换经 `retain` 裁剪（§3）。
- **自身是滚动视口**：与 VirtualListView 相同，视口大小由父布局决定；表头钉在视口顶部，
  行窗口在表头以下。行高统一，变高行不在本批。
- **Item 按值快照**：`set_source` 与 VirtualListView 同形（拷贝进表格内部有序数组）；
  tracked 源经 `set_model` 绑定，绑定随退出树/销毁解除。
- **无新增绘制路径**：DataTable 不新增 theme 配方；表头/行外观由内容节点 + 既有
  Surface/Padding 组合表达。唯一例外是选中行视觉，见 Q8。

### 9.2 提议的 API 形状

```cpp
template<typename Item, std::equality_comparable Key>
struct DataTableColumn {
    std::string header;              // 表头文本；或改用作者 cell 工厂，见 Q2
    std::optional<float> width;      // 固定逻辑宽；空则按 flex 分权重
    float flex = 1.0F;
    std::function<std::shared_ptr<scene::NanControl>(reactive::ReactiveScope&,
                                                     const Item&)> cell;
    std::function<std::weak_ordering(const Item&, const Item&)> compare; // 空 = 不可排序
};

enum class SortDirection : std::uint8_t { ascending, descending };
struct SortSpec {
    std::size_t column = 0;
    SortDirection direction = SortDirection::ascending;
};

template<typename Item, std::equality_comparable Key>
class DataTable final: public scene::NanControl {
public:
    static auto create(reactive::Graph& graph,
                       std::vector<DataTableColumn<Item, Key>> columns,
                       std::function<Key(const Item&)> key,
                       float row_height = 32.0F) -> std::shared_ptr<DataTable>;

    // 列轨道（规则同 §8.1/§8.2；有内容时改列数须先清空，见 Q2）
    void set_columns(std::vector<DataTableColumn<Item, Key>> columns);

    // 数据源（同 VirtualListView §4.1/4.3）
    void set_source(const std::vector<Item>& items);
    void set_model(Source& source);

    // 选择（同 VirtualListView §4.2）
    void set_selection_mode(SelectionMode mode);
    auto select(const Key& key) -> bool;
    auto select_range(const Key& from, const Key& to) -> bool;
    auto toggle(const Key& key) -> bool;
    void clear_selection();
    [[nodiscard]] auto is_selected(const Key& key) const -> bool;
    [[nodiscard]] auto selected_keys() const -> const std::vector<Key>&;
    [[nodiscard]] auto selection_changed() const -> const reactive::Event<>&;

    // 排序（DataTable 拥有；触发与视图策略见 §9.4 与 Q2/Q5）
    auto set_sort(std::size_t column, SortDirection direction) -> bool;
    void clear_sort();
    [[nodiscard]] auto sort_spec() const -> std::optional<SortSpec>;
    [[nodiscard]] auto sort_changed() const -> const reactive::Event<>&;

    // 滚动/程序化（同 VirtualListView §4.1 的首布局前挂起规则）
    void scroll_to_key(const Key& key);
    void scroll_to_row(std::size_t index);

    // 行访问：测试与作者视觉钩子（Q8）
    [[nodiscard]] auto row_node(const Key& key) const -> scene::NanControl*;
    [[nodiscard]] auto item_count() const noexcept -> std::size_t;
};
```

- `create` 需要 `reactive::Graph`（选择事件 + `RovingFocus`），且必须以 `shared_ptr` 持有：
  行窗口延迟物化用 `shared_from_this()` 挂 `post_layout`（与 VirtualListView/ScrollView 同约定），
  栈上实例首布局触发 `bad_weak_ptr`。
- 作者入口：`ui.make<DataTable<Item, Key>>(…)` 是否作为唯一 Builder 入口，或另加
  `NodeBuilder` 槽位，属 Q1。
- 列头/单元格内容默认都是普通 `NanControl`；DataTable 只协调位置、共享列宽与行高。

### 9.3 列轨道、行高与测量

- **固定/权重规则复用 §8.2**：固定列保留逻辑宽度，剩余空间按 flex 权重分；不足为零。
  建议抽一个 widget 内部 track solver 供静态 Table/DataTable 共用；静态 Table 行为与
  页面 golden 必须保持 0 差异（共享实现属内部重构，见 Q1 的准备项）。
- **有限宽度**：按上面规则分配；resize 后表头与正文用同一套轨道重算（§8.2 语义）。
- **无界宽度**：**不按内容自动测**——那会强制把每一行的单元格都建出来，直接破坏虚拟化。
  flex 列按可配置的 `flex basis`（默认 160 DIP）合成测量宽度，并在组件文档写明这不是
  内容自适应；Q4 决定默认值与是否要求作者显式给宽度。
- **行高**：统一 `row_height`（有限且 > 0，默认 32 DIP）；表头高度按表头内容自然测量，
  不参与行窗口；正文高度 = 行数 × 行高 + 行间距。
- **有限性**：总宽/总高用 double 累计、发布前统一校验（沿用 §8.2 的 `layout_extent` 模式），
  不把 inf/NaN 传给父布局；参数 setter 的即时校验与布局期校验分开（§8.1/§8.2 已定的分工）。

### 9.4 排序

- **归属**：`DataTable` 持有内部有序快照；`std::stable_sort` 保证相等项维持数据源顺序；
  `compare` 由列提供，没有 `compare` 的列不可排序。
- **触发**：表头指针点击（循环规则见 Q2）；程序化 `set_sort`/`clear_sort`；事件
  `sort_changed()` 无载荷，订阅方重新读 `sort_spec()`（与 `selection_changed()` 同形）。
- **数据变化**：`set_source` 后重新应用当前 SortSpec；更改列集合（列数或列身份）时清空
  排序并在文档写明（列索引不再有稳定含义）。
- **选择不受排序影响**：选中集合按 Key 保留；排序后视图如何跟随见 Q5。
- **排序是 DataTable 的职责**：静态 Table 与数据源都不排序（§8.3 不变）。

### 9.5 选择、键盘与语义

- **指针**：左键按下选择；plain 独占、ctrl 切换、shift 以锚点为起点的范围（同 §4.2）；
  点击同时把焦点交给表格容器。
- **键盘**：容器可聚焦；方向键/Home/End/PageUp/PageDown 走 `RovingFocus`；`Space` 切换
  选中、`Enter` 激活（`activated(key)`）；按住重复只移动活动行、不重复提交（同 §4.2）。
  typeahead 是否进第一批见 Q3。
- **语义**：复用 `Role::table / row / column_header / cell`（静态 Table 已引入，§8.3）；
  行 `selected` 状态从 SelectionModel 读取；行/单元格不可聚焦、焦点留在表格容器。
- **虚拟化语义边界**：语义树只包含物化（可见）行，读屏无法一次浏览全表；Q7 决定是否
  接受并在文档声明，或补一个只读的"总行数"信息。

### 9.6 非目标（本批不做）

变高行；按内容自动列宽（需要全量物化）；水平滚动；列拖拽/重排/合并；单元格内编辑；
树形或分组行；粘性多表头/列固定；分页；行内交互控件（行默认只展示，交互由表格统一处理）；
`ScrollView` 嵌套组合形态；DataTable 专属视觉配方（除非 Q8 选择 recipe 路线）。

### 9.7 待维护者决定（开工前必须关闭）

| 编号 | 问题 | 提议 / 取舍 |
| --- | --- | --- |
| Q1 | 模型形状：模板 `DataTable<Item, Key>`，还是类型擦除 `IDataTableModel` + 非模板组件？Builder 入口是什么？ | 提议模板（与 VirtualListView/ForEach 同族），但 `ui.make` 的模板实例化与文档成本要先确认；track solver 抽取属内部准备 |
| Q2 | 表头：文本自动生成 Label（样式继承）还是作者 cell 工厂？点击循环 `none→asc→desc→none` 还是 `asc↔desc` 往返？ | 提议 header 文本 + 内部 Label 继承组件文本样式，循环 `none→asc→desc→none`；有内容时禁止改列数（同 Table §8.1） |
| Q3 | typeahead / 行 label 函数是否进第一批？ | 提议进第一批，否则键盘可达性与 VirtualListView 不一致 |
| Q4 | 无界宽度下的 flex basis：默认 160 DIP？还是要求作者显式宽度、无界时拒绝测量？ | 提议显式默认值 + 文档声明"非内容自适应"；拒绝会让嵌套布局更难用 |
| Q5 | 排序后视图：保持选中首项可见、保持 scroll offset 行、还是保持活动行可见？ | 提议优先保持选中/活动行可见，否则夹取 offset |
| Q6 | 语义：是否同批给 `semantics::State` 增加 `sort` 字段？表头键盘排序同批还是第二批？ | 提议先加 `sort` 字段（只增不改），表头键盘排序放第二批并如实声明缺口 |
| Q7 | 虚拟化语义只含可见行是否可接受？ | 提议接受并在组件文档声明，同时给表格容器一个只读总行数 |
| Q8 | 选中行视觉归属：行 update 回调带 `selected` 布尔由作者绘制 / 新增 SelectedRow 配方 / 作者用 `row_node` + `selection_changed` 自更新？ | 提议回调带 `selected`（不新增绘制路径，符合"无配方也能满足契约"）；若维护者要默认选中背景，则走配方四步同步 |

### 9.8 分批与验证预案

- **Step 1 模型与选择**：公共 API、列轨道、表头、全量物化（小数据集）、选择、语义、
  指针/键盘；先红后绿。
- **Step 2 排序**：stable sort、表头触发、事件与语义（按 Q6 结论）。
- **Step 3 虚拟化**：把内部全量物化替换为窗口行（公共契约不变），沿用 §4.1 的窗口一致性、
  同窗口数据替换、首布局前滚动规则；用创建/复用计数断言窗口物化边界。
- 每步验证：新增 `data-table` 测试（`suite: 'unit'`，注册进 `tests/meson.build`）＋选择/
  排序/窗口边界红测；ASan/no-RTTI/no-SQLite/纯工具矩阵；`release_check --ci`；若新增
  showcase 页与 golden，按双平台采集→维护者审批流程。
- 新增文件按 [新增一个组件](adding_a_component.md)：`nandina/widget/data_table.hpp/.cpp`、
  `nandina/meson.build` 登记、`builtin_component_traits.hpp`（若提供 `ui.make`）、
  `tests/data_table_tests.cpp` + 注册、`docs/components/data_table.md` + 索引、
  `component_roadmap.md` 阶段 5 进度。
- 预计不新增 theme 配方（同静态 Table）；若 Q8 选 recipe 路线，走配方四步同步。
