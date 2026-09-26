# 001 · 框架忘记了自己做过的决定

> 2026 年 9 月。起因是补两个组件，结果挖到了 `app` 层 Page/Router 合约的分岔。
> 相关提交：`3a99e0f`（ContextMenu / Combobox）、`b2fec1b`（showcase 构建隔离）。
> 状态：调查与目标合约已定；typed Routes、Navigation 和单当前页切换的第一阶段已实现，旧 keep-alive 路径仍在迁移中。

## 一、本来是件小事

阶段 4 的菜单族还差两个组件。`ContextMenu` 的代码其实早就写好了 —— 两百多行，用 `friend class`
挂进 `DropdownMenu`，右键和 `Menu` 键都在。它唯一的问题是**从来没被编译过**：没有登记进
`nandina/meson.build`，没有 `ComponentTraits`，没有测试。

我当时的动作很直接：先定位它为什么编不过（缺一个 `#include "../scene/input_event.hpp"`，我用
`-include` 强制补上后零错误，证明源码本身是完整的），然后补齐交付清单、写测试、更新文档。

顺手还修了一个我自己上一轮引入的问题：`subdir('showcase')` 写在了 `if not meson.is_subproject()`
之外，导致下游消费者执行 `subproject('nandina')` 时会连带编译这个演示程序。我建了个最小 consumer
实测过，消费者默认构建里确实有 3 个 `nandina_showcase` 编译步骤。

这两件事都是确定的、可验证的工作。`ContextMenu` 12 个用例、`Combobox` 25 个用例，
73/73 测试通过。

## 二、然后撞墙了

你（项目作者）在 showcase 里想做一个最简单的应用：**第一个页面点按钮跳到第二个页面，第二个
页面点按钮跳回去**。

写不出来。

这不是"麻烦"，是"写不出来"。追下去发现：

```cpp
// nan_page.hpp:285
[[nodiscard]] auto build(PageContext& context)
    -> std::shared_ptr<scene::NanNode2D> final {   // ← final，子类无法覆写
    auto ui = context.ui();                        // ← PageContext 在这里被丢掉
    return build(ui);
}
```

面向开发者的 `app::Page<>` 基类把 `PageContext` 降级成 `BuildContext` 再交给页面，
而 `BuildContext` 里**没有 router**（grep 过，零命中）；`Page::build(PageContext&)` 又是 `final`，
子类改不回去；`NanRouter` 在 `nandina/widget/` 与 `nandina/scene/` 里同样零引用——没有任何
祖先查找的逃生通道。

**结论：用 `app::Page<>` 写的页面，永远拿不到 router。** 页1 没法 push，页2 没法 pop。

更荒诞的是能力倒挂：`app::run(config, [](PageContext& ctx){...})` 这条 lambda 路径**能**导航，
而看起来更方便的 `app::run<PageT>()` **不能**。便利基类在这里不是能力超集，是真子集。

还有一个埋得更深的坑：`nan_router.cpp:231` 里 `PageContext context { ... }` 是 `push_page` 的
**栈局部变量**，只活在 `build()` 这一次调用期间。所以这样写必悬垂：

```cpp
build(PageContext& ctx) {
    return ui.make<Button>("返回").on_click([&ctx]{ ctx.router().pop(); });  // ctx 已析构
}
```

导航天然是"以后"才发生的事（按钮点击），但 API 只在构建期间给你一个引用。

而这一切**没有任何文档**——因为 Getting Started 的第 4、5、6 章（布局、信号、页面与导航）
全都是空壳，分别只有 23、23、25 行，里面只有"本章目标"和"建议覆盖的知识点"。
前三章是写完的（93、214、165 行）。学习路径恰好断在难点上。

## 三、我先提了一个错的方案

我当时的建议是："修 showcase 的构建泄漏，然后把 ContextMenu 接通。"

你的回复大意是：**这是虚假的工作**。不要单独修 showcase，那只会让使用者难受。

这个批评是对的，而且指出了我思维里的惰性：我在**已经能看到的问题**里挑最容易动手的，
而不是去找**真正挡住人的那一层**。showcase 编不过是个症状；症状下面埋着的是
"这个框架的多页面应用根本没有可用的写法"。

## 四、去翻归档

你提醒我：项目里的 page/router 参考了 Angular，很多内容没同步到当前仓库，而是留在
`/workspace/Cpp/nandina_experiment`（已归档）。

我在那里找到了两份东西：

- `archive-0.0.2:docs/page-contract.md` —— 头部写着 **状态：已决定（2026-05）**；
- `archive-0.0.3:docs/development/page-and-router.md` —— 同一份合约的改写版，
  注明"吸收 Angular 的 page/router 思路"。

它们描述的是：`Page` 是**页面描述对象**（提供 `route_key()`、`title()`、`icon_type()`、
`build()`），`Router` 是**注册表 + 当前路由**（提供 `navigate_to()`，**不是历史栈**），
`PageHost` 负责延迟换页。而且明确写着：

> showcase、sidebar、registry 或未来文档导航，**不应再各自维护一份重复元数据**。

最刺眼的是它的 Don't 清单。对照当前 `main`：

| 旧合约明确说"Don't" | `main` 的实际做法 |
| --- | --- |
| 在没有文档化语义之前先加 `push/pop/replace` | 已实现 push / replace / pop / pop_to + 转场 |
| 让 sidebar / registry 再复制一份页面元数据 | 没有页面元数据 → playground 手写 `PageDescriptor` |
| 默认假定页面会缓存 | `NanRouter` 是 keep-alive 栈 |
| 把参数建立在字符串字典上 | ✅ 这条做对了（`NanPageT<Params>`） |

**框架把旧合约明令禁止的几件事都做了，而它指定的“元数据 + 注册表 + 单当前页”模型没有保留下来。**
当前 Router 虽有自己的 host，但它服务的是 keep-alive 栈，不是旧合约的单页替换宿主。

这就解释了两件事：为什么"点按钮跳转"这么难（能力挂在了应用代码够不到的地方），
为什么侧边栏要手写一堆样板（元数据不存在，只能自己造）。

还有一个旁证：playground 的写法本身就承认了这一点——它启动时只 `use_router().push<>()` **一次**，
之后应用内导航全走 `Signal<string> active_route` + keyed 响应式区域，自检甚至断言
"应用内导航时 router 栈深度必须保持 1"。仓库里最复杂的官方示例，**主动放弃了 router**。

## 五、讨论：把走向定下来

你把原则说得很清楚：**前端是这个项目最好的老师**；使用要简单，实现可以优雅；
使用者"知道一个确定的方法可以做一个确定的事情"——所以**不能出现多种方法做同一件事**。

于是我们逐条收敛：

**三条入口各有定位。** 你提醒我项目其实有三种创建方式：`app::run(配置, 工厂)`、
继承页面 + `app::run<Page>()`、继承 `NanWindow`。前两者是**快速单页应用**的通道，
只有 `NanWindow` 是多页面的正路。我按代码核对过——这个判断是准确的。

**不留状态。** 你的理由很实在：层级一多，全都保留状态很耗性能，不如直接"还原"页面。
所以默认 rebuild-on-enter，没有隐式缓存。

**元数据放路由表。** 旧合约说放 Page，Angular 说放路由表。我们选了后者——省掉"为了取标题而
实例化页面"，也让 sidebar 不必再抄一份。

**导航原语取类型而不是路径。** 项目已经定了 typed params、不要字符串字典，
所以用 `navigate<PageT>(params)`。第一版不做 URL/path 解析，也不在路由表里放一个看起来能匹配参数、
实际却只是展示文字的 `"/items/:id"`。

## 六、那个真正棘手的问题

然后是这轮最有价值的一问。你问：

> 首页 → 页面 A，A 里选了条目带参跳 B，B 改参数跳 C，C 改完直接回 A。
> 如果数据在服务端（http / 数据库）很简单；**但如果数据在 A 上、A 最终确认才写入，怎么办**？
> 比如组装电脑的软件：进内存、主板页修改，也能进三级页微调，最后回概览页提交。OA 系统里很常见。

这直接顶到了"不保留状态"的墙上：A 都不存在了，B/C 的修改怎么回到 A？

答案是：**草稿不属于页面，属于 Store**。而且框架自己早就写了这句话——`nan_page.hpp` 的开头：

> Route params are downward data. Shared app state lives in a developer-defined `NanStore` …
> so deep pages can update the store and ancestor pages react through Signal/Effect
> **without reverse route plumbing**.

"without reverse route plumbing" —— 不需要把数据反向传回上级页面。B/C 改 Store，A 重新构建时
读 Store。**重建的是页面，不是数据。**

配套的纪律是：**参数描述这一次导航，Store 保存需要跨页面修改的数据**。
`navigate<OptionPage>({.slot_id = "dimm1"})` 是典型用法；只读筛选条件也可以按值传参。
但把 `memory_gb` 的旧快照当作后续页面共用的草稿会出错——B 改了、C 又改，快照早就过期了。

这里我把"不保留状态"的边界说清楚了：它指的是**页面瞬态**（滚动位置、展开哪个折叠区），
不是"任何状态"。跨页面存活的东西必须进 Store。

## 七、最后只剩一个动作

讨论历史语义时，你说：项目既然没有栈式跳转机制，那**甚至不需要 back，直接 navigate 就完了**。

这句话把设计彻底简化了。于是：

- Router = 路由表 + 当前路由，**只有一个动作** `navigate<PageT>(params)`；
- 没有 push / pop / back / replace / pop_to；
- 向导里"三级页改完回概览页"就是 `navigate<OverviewPage>()`——没有历史，也就没有
  "回到哪一层"的歧义；
- 旧的 keep-alive 栈仍被测试和 playground 使用，但它不符合新合约；在 alpha 阶段一次性迁移，
  不留两套并行的导航 API。

同时我提的"自动生成侧边栏"被你否掉了：导航是产品设计的一部分，框架不该替开发者决定。
改为提供一套可组合的 Sidebar 组件族（参照 shadcn 的形状），开发者自己组装；
由 `app` 层的 route menu 适配器从路由表取标题和图标，再把值与回调传给 `widget` 层的控件，
所以仍然不重复元数据，也不让 `widget` 反向依赖 `app`。
按你的说法，这叫**让用户做判断题，而不是选择题**。

## 八、记下来的几件事

1. **症状不等于病灶。** showcase 编不过是症状；"多页面应用没有可用写法"才是病灶。
   只修症状就是虚假的工作。
2. **归档不等于作废。** 旧合约里的 Don't 清单，比很多新文档都更有约束力——它记录的是
   当时**已经想明白的事**。当前实现违反了它，是债，不是演进。
3. **文档的空洞会变成设计的空洞。** Getting Started 恰好断在"页面与导航"这一章，
   而这个能力也就恰好没有收敛。
4. **"简单"是需要设计的。** 从一个 `final` 关键字、一句注释的位置（`PageContext` 是栈局部），
   就能让一个框架的核心能力对使用者完全不可见。反过来，把这些理顺之后，
   多页面跳转就是一行 `navigate<PageT>()`。

## 九、补上的边界

复查时又遇到一个问题：如果 Shell 也是普通 Page，而 Router 每次只保留一个 Page，
导航到内容页时 Shell 和 Sidebar 都会一起消失。现在把所有权说清楚：`NanWindow` 持有常驻 Shell，
Shell 中只有一个 Outlet，Router 只替换 Outlet 内的页面。Shell 的折叠等 UI 状态随窗口存活，
页面状态每次进入重建，业务草稿留在 Store。

回调也不能抓构建期的 `PageContext&`。目标 API 用可复制的弱 Navigation 句柄，
回调按值捕获；Window 关闭后调用安全失败。导航请求在 UI 任务阶段提交，
页面构建失败时保留旧页。这个取舍也来自以前的 `PageHost`：它把换页延后，
避免事件处理期间直接修改控件树。

还有一个分层约束：`BuildContext` 属于 `widget`，不能直接认识 `app::NanRouter`。
因此原先设想的 `ui.router()` 改成 `PageContext::navigation()` / `ShellContext::navigation()`；
路由感知的菜单适配器放在 `app`，Sidebar 本身仍是通用控件。

旧 API 在当前测试里确实被使用。之所以不留兼容层，是因为项目仍处 alpha 且尚无外部使用者，
这次的目标是让应用只有一套可解释的多页写法。删除旧栈、转场和生命周期钩子时，
测试、playground、showcase、教程都要一起迁移。将来的页面转场可以由 Outlet 短暂保留两棵根树完成，
不需要重新引入无限期的 keep-alive 栈。

下一步按 [`docs/references/page_and_router.md`](../docs/references/page_and_router.md) 的验收清单实施：
先打通 Routes、Shell、Outlet 和 Navigation 的闭环，再用真正的 A → B → A 场景检查 Store、
作用域、焦点与浮层的生命周期，最后回到 showcase。
