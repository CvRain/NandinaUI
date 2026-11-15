# 数据组件：选择模型与虚拟化依据（P4.2 阶段 5 第一批）

> 状态：**2026-11-03 初稿已实施；2026-11-04 维护者复核完成（7 项修复 + 测试缺口补齐），并统一提交推送于 `5d91e01`。**
> **2026-11-14：§9 升级为 v3 定稿；2026-11-15 v3.1 勘误定稿（维护者评审）：数据视图采用非模板
> 组件 + `IRowModel` 行模型 + `DataTableColumn` 列定义 + `IItemDelegate` + 显式选择模型；
> v3.1 只修正接口矛盾（命名冲突/事件可见性/ItemBinding/typed 签名/哈希/动态列/`apply` 语义），
> 不改变总体架构。**§9 的批 1A（TrackSolver + 静态 Table 接入）已完成并提交；DataTable/ListView
> 仍未实现，按 §9.13 继续。** §8 静态 Table 与 §3/§4 基础契约不变。**
> 本文件是阶段 5 数据组件的契约入口：已完成批次见 §1–§8，后续数据视图见 §9（v3.1）。
> 静态 `Table`、`DataTable`、`Accordion` 等阶段 5 组件清单仍按
> [组件开发路线图](component_roadmap.md#阶段-5信息组织与数据展示) 分批推进，不在本批。

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

- P4.2a 已提交（`5d91e01`）。静态 `Table` 不承担行选择；交互数据视图见 §9（v3.1 定稿；批 1A 已提交）：
  非模板 `DataTable` + `IRowModel`/`DataTableColumn`/`IItemDelegate`/`TableSelectionModel`，
  以及新虚拟化 `ListView` 的迁移计划（旧 `ListView` 退役、`ForEach` 保留）。
- `VirtualListView` 作者 DSL 入口与视觉验收随维护者编写的 Showcase 组合另行接入；
  其迁移/退役在新 `ListView` 稳定后单独立项（§9.11）。


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

## 9. P4.2d 数据视图契约：DataTable 与 ListView（v3.1 定稿）

> 状态：**2026-11-14 v3 定稿；2026-11-15 v3.1 勘误定稿（维护者评审）；批 1A（TrackSolver +
> 静态 Table 接入）已完成并提交。DataTable/ListView 尚未实现，按 §9.13 继续施工。**
> 数据视图采用**非模板组件 + 行模型 + 列定义 + delegate + 显式选择模型**；v3.1 只修正接口矛盾，
> 不改变总体架构。§3/§4/§8 已交付契约不变；旧 `ListView` 只作 `ForEach` 薄包装的状态随 §9.11 回收。
>
> 决策依据（参考实现阅读）：GTK4 `GtkColumnView`（模型 + 列 factory + `GtkSelectionModel` +
> `GtkSorter`）、Qt `QAbstractItemModel`/`QStyledItemDelegate`/`QItemSelectionModel`、
> TanStack headless table、shadcn data-table 默认样式；本仓库 `playground/reference/` 保留了
> GTK/Slint/Godot/shadcn 源码副本。关键结论：**数据/列/渲染/选择/排序五个事实来源分离**；
> 视觉不双写；内部共享用组合而不是继承树。

### 9.1 设计原则

1. **非模板组件**：`ui.make<DataTable>(columns)` / `ui.make<ListView>()`；数据经 `IRowModel` 注入，
   行对象类型不进入公共类模板。作者按模型类型写 typed delegate（§9.5），框架负责转换与校验。
2. **列事实唯一**：行模型只负责行；列只由 `DataTableColumn`（含现有 `TableColumn` 轨道）定义。
3. **稳定身份**：`RowId`/`ColumnId`/`CellId` 是排序、选择、增量更新的唯一身份；**行索引不是默认身份**。
4. **不拷贝数据**：本地排序只重排显示索引；外部排序由应用重查后 `notify_reset()`。
5. **视觉归属二选一**：本批选择「delegate 承担全部单元格视觉，DataTable 不新增绘制路径」——因此
   **不新增 `DataTableRecipe`**。若将来要 DataTable 自绘表头/分隔/选中底，必须同时引入配方并撤回
   「无绘制路径」声明，不能两者兼称（§9.10）。
6. **组合优先**：内部提取 `UniformRowWindow`/`TrackSolver`/`ItemHost`/滚动命中工具，由
   DataTable/ListView 组合；不建 `UnitViewBase` 继承树。
7. **TreeView 只保留方向**：为未来树形视图保留行模型/delegate/选择扩展点，但不承诺兼容或本批实现。

### 9.2 稳定身份与哈希

```cpp
namespace nandina::widget {
struct RowId { std::uint64_t value = 0; };
struct ColumnId { std::string value; };
struct CellId {
    RowId row;
    ColumnId column;
    friend bool operator==(const CellId&, const CellId&) = default;
};
} // namespace nandina::widget

// 框架在同一头文件提供 std::hash<RowId> / std::hash<ColumnId> / std::hash<CellId>
// 特化：用户可直接把它们用于 unordered_map/unordered_set；实现无 RTTI 依赖。
```

- 模型负责 `row_id(row)` 在插入/排序/过滤后保持稳定且唯一；**不提供「行索引即身份」的默认实现**。
  ↑ 这是**稳定身份要求**，不提供兜底；简单模型可用自增计数或主键。
- `ColumnId` 用字符串：跨列插删、持久化、排序状态都按 id 而不是索引。
- 选择、当前单元、锚点、SortSpec 全部按 id 存储。

### 9.3 数据层：`IRowModel` 与 `TypedRowModelBase`

```cpp
struct RowRange { std::size_t first = 0; std::size_t last_exclusive = 0; };

// 类型令牌：无 RTTI 构建下也可用的静态地址令牌（与 framework 既有 type-token 模式一致）。
// 必须先于使用它的类声明。
template<typename T>
struct ModelTypeToken { static inline const int value = 0; };
template<typename T>
[[nodiscard]] auto model_type_token_of() noexcept -> const void* {
    return &ModelTypeToken<T>::value;
}

class IRowModel {
public:
    virtual ~IRowModel() = default;
    [[nodiscard]] virtual auto row_count() const noexcept -> std::size_t = 0;
    [[nodiscard]] virtual auto row_id(std::size_t row) const -> RowId = 0;
    // 视图/typed delegate 配对校验用；直接实现 IRowModel 时作者自己返回令牌
    [[nodiscard]] virtual auto model_type_token() const noexcept -> const void* = 0;
    // 变更事件：视图订阅（Event::subscribe 返回 move-only Subscription）
    [[nodiscard]] virtual auto reset_event() const noexcept -> const reactive::Event<>& = 0;
    [[nodiscard]] virtual auto data_changed_event() const noexcept
        -> const reactive::Event<RowRange>& = 0;
};

class RowModelBase: public IRowModel {
public:
    // public overrides：事件查询对视图可见
    [[nodiscard]] auto reset_event() const noexcept -> const reactive::Event<>& override {
        return reset_event_;
    }
    [[nodiscard]] auto data_changed_event() const noexcept
        -> const reactive::Event<RowRange>& override {
        return data_changed_event_;
    }

protected:
    void notify_reset();                              // 行集合/行数/顺序变化
    void notify_data_changed(RowRange range);         // 行内容变化（与列无关）

private:
    reactive::Event<> reset_event_;
    reactive::Event<RowRange> data_changed_event_;
};

// 非模板 RowModelBase 无法自动知道最终派生类型；作者通常继承这个：
template<typename Derived>
class TypedRowModelBase: public RowModelBase {
public:
    [[nodiscard]] auto model_type_token() const noexcept -> const void* override {
        return model_type_token_of<Derived>();
    }
};
```

**通知契约（同步事件）**：

- 只能在 **UI 线程**发出；事件同步回调，不承诺 `RowModelBase` 自身具有「帧」概念。
- `notify_data_changed({first,last})` 要求 `first <= last <= row_count()`；空范围不发事件。
  `data_changed` 不得改变行数、行顺序或 `RowId`；这些结构变化必须用 `notify_reset()`。
- detached/idle 状态下，视图收到事件后**立即更新内部账本**（行列数、显示序索引、当前/滚动夹取）。
- 遍历期（input/process/animation/layout/post_layout/paint 等 `defers_tree_mutation` 相位）只记录
  **pending reset / range**，在安全相位合并应用（同窗口内多次通知合并成一次；树变更仍走延迟通道）。
- 视图在退出树/析构时释放订阅；模型不得在通知回调中改视图树。
- 退出树只断开订阅，不丢弃 `shared_ptr<IRowModel>`；重新入树时自动重连并按
  `reset` 语义完整对账，使脱树期间的模型变化不会漏掉。替换模型或析构则永久断开旧订阅。
- 第一批只实现 `reset` + `data_changed`；插入/删除/移动通知随虚拟化的窗口复用一起加（接口不破坏）。
- 所有权 `std::shared_ptr<IRowModel>`；DataTable/ListView 都只消费 `IRowModel`（ListView 等价于一列）。
- 取数不走模型虚函数，而走列/条目取值器（§9.4/§9.5）。

### 9.4 列定义：`DataTableColumn`（二维结构唯一来源）

现有静态 Table 的 `TableColumn`（[table.hpp:16](../../nandina/widget/table.hpp)）**保持不变**，
作为纯轨道定义（`std::optional<float> width; float flex`）。新类型包含它，避免同名冲突：

```cpp
enum class SortDirection: std::uint8_t { ascending, descending };
enum class SortStrategy: std::uint8_t { none, local, external };
struct SortSpec { ColumnId column; SortDirection direction; };

// 列取值器：框架内部擦除存储。model_token 为空表示该列没有取值器（内容完全由 delegate 提供）。
struct ColumnValue {
    const void* model_token = nullptr;   // = model_type_token_of<ModelT>()
    std::function<std::string(const IRowModel&, std::size_t model_row)> text;
};

// 本地排序比较器也必须携带独立令牌；不能借用 value/delegate 的令牌，
// 因为隐藏排序列可以只有 comparator。
struct ColumnComparator {
    const void* model_token = nullptr;
    std::function<std::weak_ordering(const IRowModel&, std::size_t lhs_row,
                                     std::size_t rhs_row)> compare;
};

struct DataTableColumn {
    ColumnId id;
    std::string title;
    TableColumn track;                        // 固定/权重轨道，复用静态 Table 定义
    bool visible = true;                      // false：可排序、不参与布局/命中/选择/导航 universe
    SortStrategy sort = SortStrategy::none;
    ColumnComparator comparator;              // local 使用；external/none 为空
    std::shared_ptr<IItemDelegate> delegate;  // 空 = 表级默认 delegate（列级覆盖）
    ColumnValue value;                        // 默认 delegate 的文本来源（构造糖填充）
};
```

类型化构造糖（框架负责擦除与令牌，作者不写 `static_cast`；文本按值返回避免悬垂 view）：

```cpp
template<typename ModelT>
auto text_column(ColumnId id, std::string title,
                 std::function<std::string(const ModelT&, std::size_t model_row)> value,
                 TableColumn track = {}, bool visible = true) -> DataTableColumn;

template<typename ModelT>
void set_local_sort(DataTableColumn& column,
                    std::function<std::weak_ordering(const ModelT&, std::size_t lhs_row,
                                                     std::size_t rhs_row)> compare);

template<typename ModelT>
auto custom_column(ColumnId id, std::string title, TableColumn track,
                   bool visible = true) -> DataTableColumn;
```

- **令牌校验协议**：`DataTable::set_model` / `set_columns` 时校验每个非空
  `ColumnValue::model_token`、`ColumnComparator::model_token` 与
  `delegate->accepted_model_type_token()` 是否等于模型令牌；`nullptr` 表示
  “通用/不绑定具体模型”（通用 delegate 或缺少该能力）。不一致在**所有构建模式**
  （含 release、no-RTTI）抛 `std::invalid_argument`，绝不进入未定义行为。擦除后的取值器只在
  令牌校验通过后由框架调用。
- **`set_columns()` 规则**（事务性）：
  1. 空列表、空 `ColumnId::value` 或重复 `ColumnId` → `std::invalid_argument`，原状态不变；
  2. 允许在数据存在时改列；**被移除的 `ColumnId`** 从排序状态、选择集合、current、anchor 中裁剪；
  3. **隐藏列**仍可按 `ColumnId` 程序化排序（「隐藏排序键列」实现依据），但不进入布局、命中、
     选择和导航 universe；把列变为隐藏时裁剪该列上的选择/current/anchor。
- **排序配置不静默降级**：`SortStrategy::local` 必须有非空 comparator；
  `none`/`external` 必须不携带 comparator，否则 `set_columns()` 事务性拒绝。
  `set_local_sort<ModelT>()` 同时写入 `sort=local`、比较函数和模型令牌。
- 列宽：`track` 的 fixed/flex 沿用 §8.2 的有限宽度分配；无界宽度下 flex 列用可配置 flex basis
  （默认 160 DIP）合成测量宽，文档声明「非内容自适应」——内容自适应需要全量物化，与虚拟化冲突。
- 行高：统一 `row_height`（有限 > 0，默认 32）；变高行后续。

### 9.5 渲染层：`IItemDelegate` / `ItemBinding` / typed delegate

```cpp
struct ModelIndex { std::size_t row = 0; std::size_t column = 0; };  // 显示序坐标
struct ItemState {
    bool selected = false;
    bool current = false;      // 当前单元（键盘/程序化）
    bool hovered = false;
    bool enabled = true;
    std::uint64_t theme_revision = 0;
};

struct ItemBinding {                 // 同步借用，delegate 不得保存引用/指针
    const IRowModel& model;
    std::size_t model_row = 0;       // 模型行坐标
    ModelIndex display_index;        // 显示序坐标（排序/过滤后；两者可能不同）
    RowId row;                       // 稳定身份 = model.row_id(model_row)
    const ColumnId* column = nullptr; // DataTable 借用列 id；ListView 为 nullptr
    ItemState state;
};

class IItemDelegate {
public:
    virtual ~IItemDelegate() = default;
    // 绑定的模型类型令牌：nullptr = 通用 delegate（接受任意 IRowModel），
    // TypedItemDelegate<ModelT> 覆写为 model_type_token_of<ModelT>()。
    [[nodiscard]] virtual auto accepted_model_type_token() const noexcept -> const void* {
        return nullptr;
    }
    // create 才拿 item-scoped BuildContext（with_scope），用于建控件/effect/subscription
    virtual auto create_item(const widget::BuildContext& ui, const ItemBinding& binding)
        -> std::shared_ptr<scene::NanControl> = 0;
    // update 会被反复调用：只更新控件，不得安装 effect/subscription（避免每次更新重复订阅）
    virtual void update_item(scene::NanControl& control, const ItemBinding& binding) = 0;
};

template<typename ModelT>
    requires std::derived_from<ModelT, IRowModel>
class TypedItemDelegate: public IItemDelegate {
public:
    // 令牌校验通过后，框架只在这里做一次 static_cast
    [[nodiscard]] auto accepted_model_type_token() const noexcept -> const void* final {
        return model_type_token_of<ModelT>();
    }

    auto create_item(const widget::BuildContext& ui, const ItemBinding& binding)
        -> std::shared_ptr<scene::NanControl> final {
        return create_typed(ui, checked_model(binding.model), binding);
    }
    void update_item(scene::NanControl& control, const ItemBinding& binding) final {
        update_typed(control, checked_model(binding.model), binding);
    }

protected:
    virtual auto create_typed(const widget::BuildContext&, const ModelT&, const ItemBinding&)
        -> std::shared_ptr<scene::NanControl> = 0;
    virtual void update_typed(scene::NanControl&, const ModelT&, const ItemBinding&) = 0;

private:
    [[nodiscard]] static auto checked_model(const IRowModel& model) -> const ModelT& {
        if (model.model_type_token() != model_type_token_of<ModelT>()) {
            throw std::invalid_argument("Item delegate model type does not match");
        }
        return static_cast<const ModelT&>(model);
    }
};
```

- **约定**：`IItemDelegate` 名称面向 list/table；表格列通过 `DataTableColumn::delegate` 持有同一接口。
- **生命周期**：create → bind/update → 复用（滚动/数据替换）→ 销毁；由内部 ItemHost 做 keyed 复用
  （与 `ForEach` 同族）。首批 create/update 两阶段，窗口复用在虚拟化批次接入，公共接口不变。
- **自定义绘制**：`create_item` 返回任意 `NanControl`（组合或自绘 `on_draw` 的轻量节点）；文本请走
  文本管线/组合控件。
- **默认 delegate**：`TextItemDelegate`（单元格）+ `TextHeaderDelegate`（表头），只用语义 token /
  typography role；默认文本**单行、省略号、`max_lines=1`**（匹配统一行高）。作者可整体替换或按列覆盖。
- **类型转换责任在框架**：作者不写 `static_cast`；`TypedItemDelegate<ModelT>` / 列构造糖 / typed
  取值器封装转换与令牌校验（无 RTTI 构建同样生效，release 也检查）。通用 delegate（默认 delegate、
  不读具体模型的展示 delegate）返回 `accepted_model_type_token() == nullptr`，可配任意模型。
- `BuildContext` 与 `ItemBinding` 都是同步借用；`ItemBinding::column` 也只在当次调用
  期间有效。delegate 不得把它们或其引用/指针保存到控件之后使用。

### 9.6 选择层：`TableSelectionModel`

复用现有选择模式枚举（[selection_model.hpp:30](../../nandina/widget/selection_model.hpp) 的
`widget::SelectionMode`），不再声明第二个；二维行为另用 `SelectionBehavior`：

```cpp
enum class SelectionBehavior: std::uint8_t { cells, rows, columns };
enum class SelectionOp: std::uint8_t { replace, toggle, extend };

class ITableSelectionModel {
public:
    virtual ~ITableSelectionModel() = default;
    [[nodiscard]] virtual auto behavior() const noexcept -> SelectionBehavior = 0;
    [[nodiscard]] virtual auto mode() const noexcept -> SelectionMode = 0;
    [[nodiscard]] virtual auto current() const -> std::optional<CellId> = 0;
    [[nodiscard]] virtual auto anchor() const -> std::optional<CellId> = 0;
    [[nodiscard]] virtual auto is_selected(const CellId&) const -> bool = 0;
    [[nodiscard]] virtual auto selected() const -> const std::vector<CellId>& = 0;
    [[nodiscard]] virtual auto changed() const -> const reactive::Event<>& = 0;
    // 清空选中集合和 anchor，保留 current（键盘游标）。
    virtual void clear_selection() = 0;
    // 只移动当前单元，不改选中集合或 anchor；空值清除 current。
    virtual void set_current(std::optional<CellId> current) = 0;
    // reset/列变更后裁剪 selected/current/anchor；keep 只在调用期间借用。
    virtual void retain(const std::function<bool(const CellId&)>& keep) = 0;
    // extent 由视图按显示序展开（矩形/整行/整列）；调用者保证身份已验证
    virtual void apply(SelectionOp op, CellId target, std::span<const CellId> extent) = 0;
};

class TableSelectionModel final: public ITableSelectionModel { ... };
```

`apply()` 语义（定稿）：

- 身份存在性由 **DataTable 程序化入口**负责校验；`apply()` 只处理已验证身份。
- `extent` 为空 → 视为 `{target}`；**非空 extent 不含 target → `std::invalid_argument`**（调用违约）。
- `replace`：清空 → 选中 extent；current=target；anchor=target。
- `toggle`：target 已选则移除，否则加入；current=target；anchor=target（Ctrl 单击）。
- `extend`：选中集合替换为 extent（含两端）；current=target；**anchor 保持不变**（无 anchor 时取 target）。
- `SelectionMode::single`：忽略 extent，只保留 target。
- `clear_selection()` 清空 selected + anchor，保留 current；需要清除键盘游标时显式调
  `set_current(std::nullopt)`，避免“取消选中”意外丢失导航位置。
- `changed()` 在**选择集合、current、anchor 任一可观察状态变化**时触发，即使选中集合没变。
- `set_current()` 只服务键盘/程序化导航；`retain()` 只遍历已存储的选中/current/anchor，
  不要求视图物化或构造整张表的 `CellId` universe。两者都按实际变化幂等发事件。
- 行为：首批只实现 `cells`（单元格 → 行 → 列按后续批次）；跨排序/过滤按身份稳定；隐藏列不在 universe。

### 9.7 键盘导航：`GridNavigation`

```cpp
enum class GridDirection: std::uint8_t { up, down, left, right };
class GridNavigation {
public:
    void set_shape(std::size_t rows, std::size_t columns) noexcept;  // 显示态；0 行/0 列 → 无 current
    void set_page_rows(std::size_t rows) noexcept;                   // 视口可见行数
    void set_current(ModelIndex index) noexcept;   // 非空网格夹取；空网格清空 current
    [[nodiscard]] auto current() const noexcept -> std::optional<ModelIndex>;
    auto move(GridDirection) -> bool;        // 空网格恒 false；上下保持列，左右保持行
    auto home() -> bool; auto end() -> bool; // 行首/行尾；空网格恒 false
    auto page_up() -> bool; auto page_down() -> bool;  // 保持列；空网格恒 false
    [[nodiscard]] auto scroll_to_index() const -> std::optional<std::size_t>; // 空网格 nullopt
};
```

- **空网格**：行数或可见列数任一为 0 时不存在合法坐标，不产生虚假 `(0,0)`：`current()` /
  `scroll_to_index()` 返回 `std::nullopt`，所有移动/Home/End/Page 返回 false；`set_shape` 从非空
  变空时清空 current。
- **同步归属**：GridNavigation 只持有显示序 `ModelIndex`；`TableSelectionModel` 只持有身份
  `CellId`。视图负责双向映射：移动时把 current 写入选择模型；reset/排序后按身份重算显示坐标
  （身份消失则夹取）。两者都不持有对方的副本作为事实来源。
- 表头键盘排序在批 3 实现：表头是独立 delegate host + 可聚焦单元，不与网格导航混用。

### 9.8 排序：本地 / 外部 / 不可排

```cpp
enum class SortRequestReason: std::uint8_t { user_header_click, programmatic };
struct SortRequest {
    std::optional<SortSpec> sort; // nullopt = 清除外部排序
    SortRequestReason reason;     // 区分表头点击与程序化操作
};
```

- **状态**：DataTable 持有 `std::optional<SortSpec>`（按 ColumnId），并维护**显示序索引表**
  （显示行 → 模型行）。排序只重排显示序，不改模型、不拷贝行数据。
- **本地**（`local`）：列提供比较器，`std::stable_sort` + 方向；相等项保持模型顺序；选择身份不变。
- **外部**（`external`）：DataTable 不重排；**`reactive::Event<SortRequest>` 是事实来源**，同时提供
  `set_on_sort_requested()` 便捷入口（按现有 callback scope 生命周期守护）；应用重新查询后
  `notify_reset()`；DataTable 保留排序状态用于表头指示，显示顺序以模型为准。
- **不可排**（`none`）：表头不响应点击；程序化 `set_sort` 返回 false。
- 自定义业务排序（创建时间/修改时间/id/业务规则/自然排序/本地化）由 local 比较器闭包覆盖,
  比较器签名 `std::weak_ordering(const ModelT&, std::size_t lhs_row, std::size_t rhs_row)`；非显示字段
  用 `visible=false` 的列承载；多列排序不在本批（SortSpec 预留可扩展）。
- 表头点击循环：`none → ascending → descending → none`（仅 sortable 且 visible 列）；
  外部排序的第三步发出 `SortRequest{.sort = std::nullopt}`，使应用能清除数据源排序。
  `sort_changed()` 无载荷；排序后视图优先保持 current（否则首个选中）单元可见，
  否则夹取 scroll offset。当存在 local sort 时，任何非空 `data_changed` 都重新应用
  `stable_sort`（通知不携列信息，不能安全假定排序键未变）。

### 9.9 视图与内部共享工具（组合，不建基类）

| 工具 | 职责 | 落地批次 |
| --- | --- | --- |
| `internal::TrackSolver`（`track_solver.hpp/.cpp`） | 纯轨道数学：固定/权重分配、无界 intrinsic、gap、有限性校验；**不测量节点** | **1A（已完成 2026-11-15）** |
| `internal::UniformRowWindow` | 统一行高窗口：内容高度、offset 夹取、半开可见区间、滚动显现数学、行命中 | 1B |
| `internal::ItemHost` | delegate 控件 host：create/update/销毁/keyed 复用、每项 scope | 2B（与真实消费者一起设计） |
| 滚动/列命中 | 行命中并入 `UniformRowWindow`；列命中等 DataTable 出现后再提取 | 1B/2B |

- `TrackSolver` 无状态，实现为内部纯函数（`validate_tracks` + `solve_tracks`），不建类/成员；
  `Table` 的 `columns_` 仍是唯一事实来源，每次验证/求解做轻量 `TrackSpec` 转换，不缓存第二份。
- 输入：`std::span<const TrackSpec>`、`std::optional<float> available_width`、`std::span<const float>
  intrinsic_widths`（仅无界使用）、`gap`；输出 widths + total_width。
- 约束：有界宽度下 intrinsic 不使用；无界时 intrinsic 数量必须等于轨道数；非有限 intrinsic 与旧
  实现一致，在总宽检查处报 `Table content extent must fit finite layout coordinates`；负 intrinsic
  照旧夹取为 0；`solve_tracks()` 自身执行验证，不依赖调用者先验证；`Table::set_columns()` 仍先
  验证候选值保证失败不修改状态；固定列不测 intrinsic，flex 列保持「列外层、行内层」测量顺序；
  保留现有 Table 对外异常类型与消息；`layout_extent()` 继续留在 `table.cpp` 处理行高与坐标。
- 输入：复用场景树 capture/bubble（[scene_tree.cpp:1335](../../nandina/scene/scene_tree.cpp)）——
  delegate 内控件先消费事件；DataTable 只在冒泡到手时做命中→选择→回调。回调建议：
  `on_cell_clicked/on_cell_double_clicked/on_cell_activated/on_row_activated/on_header_clicked`。

### 9.10 默认视觉归属（本批：delegate-only，不新增配方）

**决定**：所有单元格视觉（背景、hover、选中、当前环、行分隔线）由默认/作者 delegate 依据
`ItemState` 表达；DataTable 只做布局、滚动、命中、状态跟踪与子控件托管，**不新增绘制路径**，
因此**不新增 `DataTableRecipe`**。作者用 `Surface`/`Card` 组合外框（与静态 Table 一致）。

默认 `TextItemDelegate`/`TextHeaderDelegate` 映射（shadcn 基准，只用语义 token）：

| 部位 | shadcn | 默认实现 |
| --- | --- | --- |
| 单元格文本 | `text-sm` + `p-2` | `TypographyRole::label` + `spacing_sm` 内边距；单行省略号 |
| 表头 | `text-muted-foreground font-medium h-10 border-b` | `muted_foreground` + `label` 角色 + 500 字重 + `spacing_sm`；底边 `border`/`border_thin` |
| 行分隔 | `border-b` | 每格底边 `border`/`border_thin`（连续成行线） |
| 行 hover | `bg-muted/50` | `muted` + `opacity_hover_overlay` |
| 行选中 | `bg-muted` | `muted` 填充 |
| 单元格选中 | — | `ColorToken::selection` 填充（主题可覆盖） |
| 当前单元 | focus ring | 键盘模态下 `ring` token；复用 P3.2a 输入模态策略 |
| 排序指示 | 图标箭头 | 默认 header delegate 用**线段/三角图元绘制**（参考 Select/Menu 箭头），不用 `▲/▼` 文本 |
| 空表 | "No results." | DataTable 组合默认 `EmptyState` 子控件（组件组合，不是自绘）；作者可替换 |

- 主题可覆盖路径：语义 token / 替换 delegate / 按列覆盖 delegate；若将来需要更细粒度控制，再引入
  `DataTableRecipe`（非破坏性增补，同时更新本节声明）。
- 不硬编码颜色；默认 delegate 解析 token 时跟随 `theme_revision`。

### 9.11 ListView 与 VirtualListView 迁移

| 设施 | 去向 |
| --- | --- |
| `ForEach` | **保留**：声明式全量键控区域（不是数据视图） |
| 旧 `ListView`（`ForEach` 薄包装，[list_view.hpp:23](../../nandina/widget/list_view.hpp)） | **退役**；`BuildContext::for_each()`（[build_context.hpp:383](../../nandina/widget/build_context.hpp)）改为直接使用 `ForEach` |
| 新 `ListView` | 基于 `IRowModel` + `IItemDelegate` + 虚拟窗口的 **1D 虚拟化数据视图**；行选择用 `SelectionModel<RowId>`；第一版统一行高；不再新增非虚拟化版本（该职责归 `ForEach`） |
| `VirtualListView` | 暂时兼容；新 `ListView` 稳定后评估迁移或退役（其模板 API/913 行测试单独批次） |

- 新 `ListView` 与 DataTable 共享 `IRowModel`、`IItemDelegate`、`UniformRowWindow`/`ItemHost`/
  滚动命中工具，但**不要求**共用选择模型类（1D 用 `SelectionModel<RowId>`）。
- notebook 的 NoteList/GroupList 在迁移批改为新 ListView（delegate 产出 Button）；数据层测试不变。
- 未来 TreeView：只保留 `IRowModel` 扩展方向（父子查询/展开状态），本契约不承诺兼容。

### 9.12 非目标

列拖拽/重排/合并、单元格内编辑、变高行、水平滚动、粘性列、多列排序、分组/树形、分页组件
（只提供外部排序请求事件）、选中视觉配方、TreeView 实现/兼容承诺、ListView 与 DataTable
共用同一选择模型类（只共享身份与 delegate 词汇）。

### 9.13 实施批次与验收

| 批 | 内容 | 闸门 |
| --- | --- | --- |
| **1A ✅ 已完成（2026-11-15，`refactor(table): extract shared track solver`）** | `internal::track_solver.hpp/.cpp` 纯函数 + 静态 Table 接入（删除原 validate_tracks 与列宽分配代码） + 直接单测 | 主套件 134/134、ASan 109+1 skip、no-RTTI 111/111；CPU table golden 与 HEAD 逐字节一致（A/B）；`release_check --ci` 8/8 |
| **1B** | 从 VirtualListView 提取 `UniformRowWindow`（内容高度/offset 夹取/半开可见区间/滚动显现/行命中）；pending 调度与 ForEach 复用留在 VirtualListView | VirtualListView 913 行测试全绿；行为不变 |
| **2A** | 身份 + `std::hash`、`RowRange`、`IRowModel`/`RowModelBase`/`TypedRowModelBase`、类型令牌、`TableSelectionModel`、`GridNavigation`（纯协议，不建 DataTable 节点） | 红先单测：身份/通知合并/令牌不匹配/矩形选择/导航 |
| **2B** | `DataTableColumn`、delegate/`ItemBinding`/`TypedItemDelegate`、`ItemHost`、全量物化、本地/外部排序；公共头标 `internal` 不导出 | 红先单测：列事务、排序状态迁移、delegate 生命周期、外部排序事件 |
| **3** | DataTable 虚拟化 + 表头键盘排序 + 公共导出（`header_surface.toml` 转 experimental）+ 组件文档/索引 | 窗口物化计数、复用、滚动边界、键盘/语义红先；双平台 Showcase 页 + golden 由维护者交付/审批 |
| **4** | `BuildContext::for_each()` 改 `ForEach` 直接实现；相关测试/文档同步 | for_each 行为不变；authoring 测试全绿 |
| **5** | 新虚拟化 `ListView` + notebook 迁移 + 旧 `ListView` 退役（sdk_surface 记录破坏性变更） | ListView 红先（窗口/行选择/delegate 复用）；notebook 构建；全矩阵 |
| **6（另议）** | `VirtualListView` 迁移/退役、TreeView | 单独立项 |

**公共 API 闸门**：2B 的 DataTable 头只作 internal 构建输入，不进入 `sdk_surface` 公共面；
批 3 虚拟化验收通过后才转为 experimental 并在 `sdk_surface.md` 记录。

**施工纪律（1A 起）**：每批先红先测试再实现；工作树保持未提交供维护者复核；本契约只随已证实的
实现进度更新状态，不把计划写成已实现。

### 9.14 验证预案

- 单测：TrackSolver 轨道数学；identity/hash；模型通知合并；令牌校验（含 release/no-RTTI）；
  delegate 生命周期/复用；矩形选择（replace/toggle/extend、单选/多选、跨排序身份稳定）；
  GridNavigation（上下左右/Home/End/PageUp/Down 保持列）；本地排序稳定性、外部排序请求事件；
  虚拟化窗口物化与滚动边界、表头键盘排序语义。
- 矩阵：主套件 + ASan/UBSan + no-RTTI + no-SQLite + 纯工具 + `release_check --ci`（每批）。
- 视觉：默认 delegate 的 token 解析与主题切换单测；Showcase 页面正文与双平台 golden 由维护者
  手写/审批（组件先在 showcase 验收）。
- 性能：虚拟化窗口物化计数断言（不依赖墙钟）；页面加载成本按 P4.8b 口径可选报告。

### 9.15 已定项（v3.1，原 §9.15 开放项关闭）

1. **`TableSelectionModel::apply(SelectionOp, CellId target, std::span<const CellId> extent)`**：
   矩形展开由视图负责；非空 extent 不含 target 抛 `invalid_argument`；身份有效性由 DataTable
   程序化入口校验（§9.6）。
2. **默认文本 delegate**：单行、省略号、`max_lines=1`，适配统一行高。
3. **ListView 第一版**：统一行高。
4. **外部排序**：`reactive::Event<SortRequest>` 为事实来源，同时提供 `set_on_sort_requested()`
   便捷入口（按 callback scope 生命周期守护）。
