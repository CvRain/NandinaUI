# Page / Router 目标合约

> 状态：typed Routes、Navigation 和单当前页切换已落地；旧页面栈与转场 API 已移除。迁移范围与验收条件见第 9 节。

本合约源于归档分支 `archive-0.0.2` 的 `docs/page-contract.md` 和 `archive-0.0.3` 的 `docs/development/page-and-router.md`：保留注册表、当前页面和每次进入重建的决定；结合当前 C++26 主线，把页面参数改为强类型，把元数据放到路由表。

## 1. 使用入口与所有权

| 入口 | 用途 | 路由能力 |
| --- | --- | --- |
| `app::run(config, [](widget::BuildContext& ui) { ... })` | 快速创建单窗口界面 | 无 |
| `app::run<PageT>(config)` | 以一个页面类型启动单页应用 | 无 |
| 继承 `app::NanWindow`，在 `on_setup()` 中配置 Routes 与 Shell | 多页面应用 | 有 |

前两条是单页入口。它们内部通过一个私有的单路由表启动 `Page`，但不向应用暴露 Router；根视图工厂只接收 `BuildContext&`，不具备页面导航和 Store 访问能力。需要导航时使用 `NanWindow`。`NanApplication` 仍可由应用显式创建，以安装 Store、资源和主题。

多页面窗口中的所有权关系固定为：

```text
NanApplication（Graph、Store）
└── NanWindow（Shell 作用域、Router、OverlayHost）
    └── Shell 根节点（常驻）
        ├── 应用自选的导航控件
        └── RouterOutlet（当前页面根节点与页面作用域）
```

Shell 是窗口内容，不是注册在 Routes 里的普通 Page。Router 只替换一个 Outlet 中的页面内容，不替换 Shell。第一版每个路由窗口恰好有一个 Outlet；未安装或重复安装 Outlet 在窗口启动时明确报错。窗口关闭时先销毁页面和 Shell，再销毁 Router；Graph 与 Store 由 Application 持有，寿命更长。

## 2. 路由表

Routes 是页面类型与导航元数据的唯一声明点。下面是目标 API 形态，具体 C++ 名称可在实现时调整，语义不变：

```cpp
const app::Routes routes {
    app::route<HomePage>({.title = "首页", .icon = "home"}),
    app::route<SettingsPage>({.title = "设置", .icon = "settings"}),
    app::route<OptionPage>({.title = "详情", .show_in_nav = false}),
};

void MainWindow::on_setup() {
    auto& router = use_router(routes);
    router.start<HomePage>();
    set_shell([](app::ShellContext& ctx) -> widget::View {
        auto ui = ctx.ui();
        return ui.row().children(
            build_sidebar(ctx),
            widget::authoring::make<widget::Expanded>().child(ctx.outlet())
        ).build();
    });
}
```

- 初始页面必须显式指定并已注册；注册顺序不决定初始页面。
- 页面类型用无 RTTI 的 `app::page_key<PageT>()` 识别，身份就是类型本身。类型令牌带类别标签（`PageKey` / `ParamsKey` / `StoreKey`）：把参数键或 Store 键交给路由查询会在**编译期**失败，不再像裸 `const void*` 那样只能靠"运行时匹配不上"发现问题。旧的 `nan_type_key<PageT>()` 等价于 `page_key<PageT>()`，`NanTypeKey` 保留为 `PageKey` 的兼容别名。
- 注册表校验在**声明期**一次性完成：`Routes::validate()` 与 `NanRouter::configure()` 返回 `std::expected<void, RoutesError>`。`RoutesError` 带 `kind` 与冲突条目下标，`describe()` 给出可直接用作日志/异常信息。它拒绝空表、无效条目、重复的**页面类型**、重复的**非空 `address`**，以及"已有当前页面时换表"。未注册页面的 `navigate<PageT>()` 不改变当前页面并返回失败，`start<PageT>()` 在表里没有该类型时同样失败。
- 只读查询与遍历：`find()`（`noexcept`，未命中返回 `nullptr`）、`at()`（未命中抛 `std::out_of_range`）、`contains()`、`index_of()`、`begin()/end()`。`find()` 刻意不返回 `optional`/`expected`：唯一失败原因是"未注册"，套一层只会引入第二种"空"状态；需要说明失败原因的场合是 `navigate_to()`。
- `nav_entries()` 返回"点一下就能进去"的只读视图（`show_in_nav` 且 `activate != nullptr`）。导航 UI 直接枚举它即可，不必各自重复这两条筛选规则，也就不会因为漏掉判空而留下点了没反应的死条目。Router 侧的 `current_entry()` / `is_current()` 是标题、图标与高亮的唯一依据。
- 路由表在启动后不可变。`show_in_nav = false` 只影响导航 UI 的默认筛选，不禁止程序跳转。
- `title`、`icon` 属于路由表，Page 不再提供 `route_key()`、`title()`、`icon()`。应用可以自选导航布局和排序，但同一页面的标题与图标应从路由表读取。
- 可选的 `address` 只是给应用显示的静态地址文字（窗口标题、面包屑末段之类），**不是路由身份**：留空时回退到 `title`，两者都空也合法，所以 `app::route<PageT>()` 这种最简写法可用。它刻意不叫 `key`，避免与 `RouteEntry::page_key` 混淆。`NanRouter::current_address()` 读它，`current_page_key()` 读身份，两者成对。
- 第一版不提供可解析的 URL/path，不能把 `"/items/:id"` 误当参数匹配规则。深链接、序列化和路径解析留待另行设计。

路由表只保存页面类型与元数据，不预先创建页面实例；每次进入时才由导航请求创建该页面及其根节点。参数类型由 `Page<Params>` 决定，未带参数的页面使用 `Page<>`。

## 3. 唯一导航动作

```cpp
auto nav = ctx.navigation();
nav.navigate<OptionPage>(SlotParams{.slot_id = "dimm1"});
nav.navigate<OverviewPage>();
```

Router 只维护一个当前路由，公开动作只有 `navigate<PageT>(params)`；没有 `push`、`pop`、`back`、`replace`、`pop_to` 或隐式历史。导航到当前页面也视为一次新进入，会重建页面。向导从三级页回概览页，直接 `navigate<OverviewPage>()`。

`navigate<PageT>(params)` 的页面类型在编译期确定，失败只可能是"忘了注册"这类编程错误，所以返回 `bool`。当页面键来自运行时枚举（侧边栏、命令面板遍历 `nav_entries()` 的结果）时改用 `Navigation::navigate_to(page_key)`：它返回 `std::expected<bool, NavigationError>`，把"句柄已失效"（`Unavailable`）、"空键"（`InvalidKey`）、"没有注册"（`UnknownRoute`）、"这一页需要参数"（`RequiresParams`）区分开，`describe(error)` 给出可读文字。消费方因此不必重写 `if (key == …) navigate<PageT>()` 链，也不会把失败静默成"点了没反应"。

`Navigation` 是可复制的弱句柄，由 `PageContext::navigation()` 和 `ShellContext::navigation()` 提供。回调按值捕获它；不得按引用捕获构建期的 Context。句柄不延长 Window 或 Router 寿命。窗口关闭后调用返回 `false`，不访问悬垂对象。未注册目标或已关闭窗口也返回失败；返回 `true` 只表示请求已被接受，不表示页面已经切换。

所有被接受的导航请求在 UI 线程任务阶段提交，控件事件期间不直接修改场景树。一个任务阶段内多次请求取最后一次，跳过中间页面的构建；构建过程中发起的请求排到下一次任务阶段，防止递归换页。后台线程须经现有 `UiDispatcher` 回到 UI 线程。页面销毁时，其 `AsyncScope` 会取消工作；页面异步任务不应在取消后再发起导航。

## 4. 页面构建与切换事务

应用页面只实现一个构建方法：

```cpp
class OptionPage final: public app::Page<SlotParams> {
public:
    auto build(app::PageContext& ctx) -> widget::View override {
        auto ui = ctx.ui();
        auto slot_id = params().slot_id;
        auto nav = ctx.navigation();
        // 使用 ui 构建控件；回调按值捕获 slot_id 和 nav。
    }
};
```

`PageContext` 提供构建作用域、Store 和导航句柄；`BuildContext` 负责声明式控件构建，不认识 `app::NanRouter`。删去旧的 `build(BuildContext&)` 便利重载，避免 PageContext 被降级后丢失服务。参数存于新创建的 Page 实例，`params()` 在该实例寿命内有效；异步回调需要的值应自行复制。

切换按事务处理：先验证目标并在独立页面作用域中构建新页面；成功后提交新根节点，再使旧页面回调失效、取消旧页面异步任务、关闭它拥有的浮层、解除焦点和响应式订阅，最后拆除旧根节点。Shell 与 Shell 作用域始终保留。若新页面构建失败，保留旧页面及当前路由，清理未提交的新作用域，并将错误交给窗口的错误处理路径；不可留下空白 Outlet 或静默吞掉异常。

失败路径的实现：`NanRouter` 捕获构建期异常，回滚未提交的记账与作用域，然后调用已安装的错误处理器。`NanWindow` 在 `use_router()` 时把处理器接到 `on_error()`（默认实现记录 error 日志，覆写可接入自己的错误界面）；不继承窗口的场景与测试用 `NanRouter::set_page_error_handler()` 接管。因此 `navigate()` **不抛异常**：失败返回 `false`，错误另行上报——带 dispatcher 时调用点在 `UiDispatcher::drain()` 的任务里，异常抛出去会穿出主循环并终止进程。

完成切换后，键盘焦点按新页面的默认焦点规则进入 Outlet；不能恢复到已经销毁的旧页面节点。页面生命周期是“每次进入构建、离开销毁”，旧 `on_activate()` / `on_deactivate()` 的 keep-alive 语义一并移除。

退役顺序的实现要点：`ReactiveScope::clear()` 本身已经按"先失效 generation（`guard_callbacks` 的凭据）、再断开外部事件、最后释放 signal/computed/effect"执行，所以 Router 侧只需在拆根**之前**依次清页面作用域与 `AsyncScope`。反序（先 `remove_child`）会让 `on_exit_tree` 在页面订阅仍然生效时运行，回调可能去操作正在拆除的子树。

浮层的关闭方式与上面几步不同：浮层挂在 OverlayHost 的 overlay layer 上，与页面节点之间**没有可查询的从属边**，因此 Router 不做遍历。页面拥有的浮层由其内部控件的 `OverlayHandle` 在页面析构时关闭——这就是 Dialog / Tooltip / Select 持有 portal handle 的原因。只要页面不把 handle 交给比它活得更久的所有者（例如 Store），切页后就不会留下无归属的面板。

## 5. 状态归属

| 状态 | 所有者 | 例子 |
| --- | --- | --- |
| 本次导航输入 | 强类型 Params | 要编辑的槽位 ID、只读筛选条件 |
| 页面瞬态 | 页面作用域 | 当前焦点、局部展开状态、滚动位置 |
| 跨页面草稿与业务状态 | 应用级 `NanStore` | 尚未提交的整机配置 |
| 常驻导航 UI 状态 | Shell 作用域 | 侧边栏是否折叠 |

Params 是一次导航的不可变输入，可以携带标识或适合按值传递的只读条件；需要在多个页面间修改、保留或提交的数据放进 Store。不要把可变业务对象的旧快照当作跨页面共享状态。深层页面修改 Store，概览页重新构建时从 Store 读取最新草稿。应用如需“取消”与“提交”，可以在 Store 中区分 draft 与 committed；框架不代做事务。

`NanApplication::use_store<StoreT>()` 安装 Store，页面通过 `ctx.store<StoreT>()` 访问。应用需保证 Store 在窗口和页面销毁之后才销毁；Store 中的 Signal 仍受 Graph 生命周期约束。页面与组件作用域离开后必须清理订阅和回调，即使应用意外保留了旧控件的 `shared_ptr`。

## 6. Shell、Outlet 与导航控件

Shell 的构建上下文由 `app` 层提供。它持有窗口级作用域、`BuildContext`、`Navigation` 和 Outlet 创建入口。Outlet 是常驻 Shell 内的挂载槽；当前页面由 Router 负责创建和销毁，Outlet 负责布局、焦点和节点替换。Outlet 的宿主实现必须保证页面在下一次绘制前完成切换，不在输入回调中修改树。

Outlet 的换页是**原子**的：旧页面先摘、新页面后挂，任何时刻最多一个页面子节点。Router 侧的退役（清作用域、取消异步、解除焦点）与换页是两件事——退役不拆根，拆根由 Outlet 在替换时一并完成，否则"旧页面已退役"与"新页面已挂载"之间会存在两个子节点共存的窗口。

换页还必须对**场景树遍历阶段**安全。`process` / `layout` / `post_layout` / `paint` 阶段不能直接改 `children_`（`NanNode::remove_child` 会直接抛），此时 Outlet 把目标页面记下来并只排**一次**延迟换页；同一阶段内多次换页在 flush 时合并成最后一次。这里必须合并：若每次换页各排一个替换，第二个替换会用排队时捕获的陈旧 current 去摘节点，结果 flush 后 Outlet 里留下一个没有任何引用的僵尸页面。

`widget` 层不能依赖上层 `app`。因此通用 Sidebar、Button 等控件只接收标题、图标、选中状态和点击回调。把 `PageT` 映射为元数据、`Navigation` 与选中状态的适配器放在 `app` 层，例如 `app::route_menu_button<PageT>(ctx)`；不能在 `widget::BuildContext` 或 `widget::SidebarMenuButton` 内直接引用 `NanRouter`。开发者决定导航 UI 的位置、分组与样式，框架不从 Routes 自动生成整个 Sidebar。

## 7. 范围与后续

第一版只支持单窗口内一级路由和一个 Outlet。多窗口各自拥有 Router 与 Shell，可以共享 Application Store。嵌套路由、深链接、guards、resolve、页面缓存和自动生成面包屑均暂缓；现有 `Breadcrumb` 控件仍可由应用手动组合。

页面重建并不排除转场。未来可由 Outlet 在有限时段内同时持有新旧根节点完成动画，结束后立即清理旧页面；转场不应重新引入无限期的 keep-alive 页面栈。当前旧 Router 的转场 API 随栈模型一起移除。

## 8. 模块与 API 边界

- `app` 拥有 Routes、Page、Shell、Navigation、Router 和 Outlet 的编排逻辑；`scene` 只提供节点与生命周期机制；`widget` 只提供可组合的视觉控件。
- `PageContext` / `ShellContext` 属于 `app`，可包含一个 `BuildContext` 值；`BuildContext` 不包含上层类型，保持[模块依赖规则](module_dependency.md)中的单向依赖。
- 公开给控件回调的是弱 `Navigation` 值，不是 `PageContext&`、`NanRouter&` 或裸指针。路由表查询与菜单适配器由 `app` 提供。

## 9. 与当前实现的差异和验收

当前 `main` 已提供 typed `Routes`、`Navigation` 和单当前页切换；`NanApplication::run_page` 与 `app::run<PageT>` 已经通过私有单路由表启动 `Page`。页面每次进入重建，离开时销毁；Router 不再提供历史栈、keep-alive 生命周期或转场栈 API。playground 仍保留 `active_route` 作为 Shell 标题和导航高亮的 UI 状态，不承担页面生命周期。

路由身份与查询层已加固：类型令牌按 `PageKey` / `ParamsKey` / `StoreKey` 分类（传错类别是编译错误），`Routes::validate()` / `configure()` 返回带原因的 `RoutesError`，`Routes` 提供 `contains` / `index_of` / `at` / `begin` / `end` / `nav_entries` 等只读辅助，`Navigation::navigate_to()` 提供类型擦除导航，Router 提供 `current_entry()` / `is_current()`。`tests/router_tests.cpp` 已覆盖这些行为。

迁移完成的最低标准：

1. 快速单页入口与多页 `NanWindow` 各有可编译示例；单页入口不暴露 Router。
2. A → B → A 后，A 的 `build()` 再执行一次，A 的旧根节点、Effect、事件订阅、AsyncScope 和浮层均已清理；Store 草稿仍在。
3. Shell 和 Sidebar 的节点身份保持不变，Outlet 只保留当前页面；同页导航也重建。
4. 回调按值捕获 Navigation 后可安全请求跳转；窗口关闭后旧句柄安全失败；事件期请求到任务阶段才切换。
5. 未注册目标、重复注册、缺失初始路由、重复 Outlet、页面构建失败均有明确可测试的行为；失败不得破坏当前页面。
6. 更新 `tests/router_tests.cpp`、playground、showcase 与 Getting Started 第 6 章；旧栈及转场 API 已删除，剩余工作集中在示例与教程表达统一。
7. 根据窗口真实运行验证焦点、Overlay 清理、异常路径和布局；无窗口单测覆盖路由模型与作用域释放。

当前事实仍以源码为准。实施过程中若 API 形状调整，应先同步此合约和相关测试，再修改面向使用者的教程。Getting Started 由项目作者写正文，AI 只核对符号与可运行示例，遵守[文档分工](../components/coverage.md)。
