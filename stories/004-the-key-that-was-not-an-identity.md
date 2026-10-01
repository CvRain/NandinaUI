# 004 · 那个不承担身份的 key

> 2026 年 10 月。起因是写 showcase 时"路由用起来不舒服"，最后收敛成一次对 `app` 层
> 路由 API 的加固：类型键分标签、失败带原因、补上消费侧要用的辅助函数。
> 状态：已落地，`buildDir` 与 `buildDir-no-rtti` 均通过，70/70 单元测试绿。

## 一、起因是三句话

项目作者的原话大概是：用 `Routes` 的时候体验不好；`NanTypeKey` 原型是 `void*`，很容
易什么都放进去、很难核验传的到底是什么；返回 `RouteEntry*` 是不是该换成
`optional`/`expected`；另外这个类功能太少。

我先只给了建议，没有动手。三条里两条我判断错了，后面会写到。

## 二、`void*` 的问题不在"放得进东西"

先说一个反直觉的判断：`nan_type_key<T>()` 取的是函数内 static 变量的地址，它是**令牌**
而不是数据指针。传一个不是它产生的 `void*` 进去，只会匹配不上，不会造成内存问题。
"什么都放得进去"这个担心，实际上是"什么都**编译得过**"——

```cpp
find(NanTypeKey key)      // 页面键
RouteEntry::params_key    // 参数键
store_key_                // Store 键
```

三者类型完全相同。把 Store 键交给路由查询、把页面键交给 `set_store()`，全部静默通过。
所以真正该修的不是"能不能放进去"，而是**编译期丢掉了类别**。

改法很便宜：

```cpp
struct PageTag{}; struct ParamsTag{}; struct StoreTag{};

template<typename Tag> struct TypeKey {
    const void* token = nullptr;
    [[nodiscard]] constexpr auto valid() const noexcept -> bool { return token != nullptr; }
    friend constexpr auto operator==(TypeKey, TypeKey) noexcept -> bool = default;
};
```

`Tag` 参与函数签名，于是 `page_key<Foo>()` 和 `params_key<Foo>()` 拿到的是**不同地址**：
既编译期不可互换，运行期也不会误判。零运行时开销，仍然不用 RTTI（`buildDir-no-rtti` 是
专门为此存在的，这次也验证过）。

## 三、`optional<RouteEntry*>` 是更差的方案

这条是作者提的，我否决了：`optional<RouteEntry*>` 有两种"空"——`nullopt`，和 engaged
但值为 `nullptr`。裸指针的 `nullptr` 本来就已经表达了"没有"，套一层只让调用点多一次
`has_value()`，不带来任何信息。

`std::expected` 则要分场合。`Routes::find` 的失败原因只有一条："未注册"。为单一枚举值
定义 `Err`，是仪式感。但 `configure()` 不一样——它原来返回 `bool`，而实现里压掉了**七种**
失败：

```cpp
// 旧实现
if (route_mode_ || current_.has_value() || routes.entries().empty()) { return false; }
if (entry.page_key == nullptr || entry.params_key == nullptr) { return false; }
if (earlier.page_key == entry.page_key) { return false; }        // 重复页面
if (... options.key == entry.options.key) { return false; }      // 重复显示 key
```

调用方（`NanWindow::use_router`）拿到 `false` 只能抛一句
`"invalid or duplicate routes"`，既不知道哪里错，也不知道该改哪一行。这里是真正需要
`expected` 的地方：

```cpp
struct RoutesError {
    RoutesErrorKind kind = RoutesErrorKind::empty;
    std::size_t index = 0;     // 出错条目
    std::size_t conflict = 0;  // 冲突条目
};
[[nodiscard]] auto describe(const RoutesError&) -> std::string;
```

现在报的是 `route[1] duplicates the page type of route[0]`。

## 四、类型擦除导航放在哪里

要消掉文档里那条 `if (key == …) navigate<PageT>()` 链，就得让 `Navigation` 能按运行时
的 `PageKey` 进入页面。实现必须查路由表，但 `RouteEntry` 定义在 `nan_router.hpp`，而
`nan_router.hpp` 又包含 `nan_page.hpp` —— 循环包含。

我一开始想给 `NavigationState` 加一个返回 `const RouteEntry*` 的查找函数，写着写着发现
它根本不需要知道 `RouteEntry` 长什么样：只要把"进入"这件事本身类型擦除过去就行。

```cpp
struct NavigationState {
    std::move_only_function<bool(PageKey, std::unique_ptr<PageBase>)> submit;
    std::move_only_function<std::expected<bool, NavigationError>(PageKey)> activate;
};
```

Router 在构造时装上 `activate`，析构时清掉。顺带解决了生命周期：窗口关掉之后
`navigate_to()` 报 `unavailable`，而不是去碰一个已经析构的路由表。

顺带一个副产品：`activate == nullptr`（页面需要参数、不能"点一下就进去"）本来是个隐式
约定，现在变成了 `NavigationError::requires_params`，可以被明确处理。

## 五、我推翻自己的地方

**一：`RoutesBuilder`。** 原本建议做"`RoutesBuilder` → `Routes`"两阶段类型。真动手时
发现 `Routes` 的不变性其实已经成立：`configure(Routes routes)` 按值收，Router 持有的
那份从不被修改，`push_back` 只作用于调用方的局部副本。于是没造新类型，只加了
`Routes::validate()`，让声明期检查可以先于 Router 存在（测试里 `Routes{}.validate()` 就
是这么用的）。收益一样，API 表少一个类型。

**二：改名。** 我建议把 `RouteOptions::key` 改成 `address`，但第一轮**搁置了**，理由是
测试里有 40 多处 `{.key = "..."}`，其中大量行完全相同（`app::route<PlainPage>({.key =
"plain"}),` 出现十几次），逐条替换容易改错。这个成本估计是错的：改用语言服务器的符号
重命名，一次改完 **7 个文件 92 处**，`current_key()` → `current_address()` 又是 4 个文件
23 处。

教训是我把"文本搜索替换"当成了唯一手段，于是高估了成本、低估了收益，还顺手把一个真实
的可读性问题留在原地——`RouteOptions::key`（显示文字）和 `RouteEntry::page_key`（身份）
同名，正是作者说"很难核验传入的到底是什么"的一部分来源。

## 六、一次假绿

中途有一次我改完跑测试，输出是：

```
All tests passed (355 assertions in 35 test cases)
```

但那是**假的**。真实情况是我在同一个命令行里先编译、再跑测试：

```fish
ninja -C buildDir tests/router_tests 2>&1 | grep -E "error:" -A6 | head -30
./buildDir/tests/router_tests
```

编译失败了（一次编辑在 `nan_page.hpp:182` 留下了一个多余的 `u`），但 `grep` 把它过滤成
了上下文行、`head` 又截断了末尾，我没看出 `FAILED`；第二条命令跑的是**上一次成功构建**
留下的旧二进制，"All tests passed" 说的是旧代码。

同一次里还有个小坑值得记：给 `RouteEntry` 写 `operator==(...) = default` 之后，编译器报
"使用了被删除的函数"。原因是 `RouteOptions` 没有 `operator==`，成员不可比较，默认化的
比较运算符就被隐式删除了——`RouteOptions` 也得补一个。

## 七、最后长什么样

- 类型键：`PageKey` / `ParamsKey` / `StoreKey`，工厂 `page_key<T>()` / `params_key<T>()`
  / `store_key<T>()`；`nan_type_key<T, Tag = PageTag>()` 是底层原语，`NanTypeKey` 保留为
  `PageKey` 的兼容别名。
- `Routes`：`find`（`noexcept`，未命中 `nullptr`）、`at`（抛 `std::out_of_range`）、
  `contains`、`index_of`、`begin`/`end`、`nav_entries()`、`validate()`。
- `NanRouter`：`configure()` 返回 `std::expected<void, RoutesError>`；新增
  `current_entry()` / `is_current()`；`current_key()` 更名 `current_address()`。
- `Navigation::navigate_to(PageKey)` 返回 `std::expected<bool, NavigationError>`。
- `RouteOptions::key` 更名 `address`，注释里写明它不参与匹配。
- showcase 侧边栏不再硬编码两个按钮，改为遍历 `nav_entries()`：

```cpp
for (const auto* entry: context.routes().nav_entries()) {
    const auto page_key = entry->page_key;
    navigation_buttons.get().add(
        ui.make<widget::Button>(nav_label(*entry))
            .on_click([navigation, page_key] {
                if (const auto entered = navigation.navigate_to(page_key); !entered) {
                    log::error("showcase sidebar: cannot enter route: {}",
                               describe(entered.error()));
                }
            })
            .build());
}
```

`navigate<PageT>()` 仍然返回 `bool`：页面类型在编译期确定，失败只可能是"忘了注册"这类
编程错误；只有运行时键的路径需要 `expected` 来说明原因。这个分工写进了
[Page / Router 合约](../docs/references/page_and_router.md) 第 3 节。

## 八、给下一个人的话

- 想把 `void*` 换成 `std::any`/`type_info` 之前，先确认要解决的是"存不下"还是"编得过"。
  这里只需要后者，而后者用标签就够了，且不引入 RTTI。
- `optional<T*>` 基本总是错的。要分清"没有值"和"值本身为空"，先问清楚失败到底有几种原因。
- 改一个符号的名字之前，先确认手边有没有符号级重命名。我用文本替换的直觉估了一次成本，
  错了 20 倍，还因此把问题留在了原地。
- **不要把编译和运行串在一个管道里就以为自己在验证。** 编译失败 + `grep`/`head` 过滤 =
  你测的是上一次的产物。
