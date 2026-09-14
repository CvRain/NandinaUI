# 编码与 API 规范

框架的长期可维护性很大程度上取决于「一致性」而不是「聪明」。当十多个模块由不同时间、不同背景的贡献者共同维护时，命名、头文件组织、API 形态和检查门槛只要有轻微漂移，使用者和后续实现者就要在每处细节上重新做判断。本文把这些反复出现的判断固化成规则，目标是让新代码在写出来之前就能预期它应该长什么样。

本文讨论**怎么写**，`component_contract.md` 讨论**组件应满足什么契约**。两者职责不同：公共组件在状态、事件、绑定、主题、输入和无障碍方面的门槛以组件契约为准，本文不重复。当文档与源码不一致时，以 `nandina/` 下的现有代码为准，并应通过修改本文来消除分歧。

## 1. 模块与命名空间

框架按 12 个模块组织：`foundation`、`reactive`、`animation`、`text`、`render`、`resource`、`scene`、`widget`、`theme`、`app`、`semantics`、`physics2d`。目录名就是模块名，命名空间与之一一对应：模块 `scene` 中的代码位于 `namespace nandina::scene`。所有公共类型都必须落在 `nandina` 之下，不在全局命名空间引入任何名字。

模块内部可以再划分更细的子命名空间，例如 `nandina::widget::primitives`、`nandina::widget::authoring`、`nandina::foundation::utf8`。当上层模块需要在自己的命名空间中简化对下层类型的引用时，应使用 `using` 声明（如 `using foundation::NanColor;`）把**具体的名字**引入，而不是 `using namespace` 整个下层命名空间：前者让依赖关系在头文件里显式可查，后者会让包含顺序影响重载解析。因此库内头文件不出现 `using namespace`。

跨模块依赖只能沿「向下」方向发生。一个模块若需要识别另一个模块的具体类型，正确做法是把设施移动到它真正依赖的那一层，而不是让底层头文件去命名上层类型——这条规则的具体推论见 `component_contract.md` 第 8 节。

## 2. 命名

公共类型统一使用 `Nan` 前缀，例如 `NanNode`、`NanControl`、`NanNode2D`、`NanTheme`、`NanColor`、`NanPoint`。前缀的作用是让读者在看不出 `using namespace` 的上下文里也能立刻判断一个名字来自框架；因此它是公共类型的约定，不因类型大小、是否聚合或是否在子命名空间中而例外。

其余命名与主流 C++ 风格一致：文件与目录使用 `snake_case`（`scene_tree.hpp`、`design_system.cpp`），函数与方法使用 `lower_case`（`set_content`、`world_bounds`），私有成员变量带尾随下划线（`tree_`、`semantics_id_`），`enum class` 的枚举值使用 `lower_case`（`ControlOverflow::clip`、`Easing::ease_in_out`），编译期常量与宏使用 `UPPER_CASE`。

响应式 API 采用一组固定的短名字：可写状态是 `Signal<T>`，派生值是 `Computed<T>`，副作用是 `Effect`，可订阅事件是 `Event<T>`，依赖图与作用域分别由 `Graph` 和 `ReactiveScope` 管理，`batch(graph, fn)` 把作用域内的多次写入合并为一次刷新。这些名字已经稳定，新代码不应另造同义 API。

上述命名同时受 `.clang-tidy` 的 `readability-identifier-*` 规则约束，改动前先确认它不会与检查冲突。

## 3. 头文件与包含

当前主线使用**普通头文件**，不使用 C++20 modules。每个头文件必须有包含保护，宏形如 `NANDINA_EXPERIMENT_<模块>_<文件>_HPP`，例如 `nandina/widget/authoring.hpp` 使用 `NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP`。绝大多数头文件已经遵循带模块路径的写法，少数早期文件只保留文件名；新文件一律使用完整路径形式，避免不同模块出现同名文件时发生宏碰撞。

头文件内的 `#include` 分为三组，组间空行分隔、组内按字典序排列（`.clang-format` 的 `SortIncludes` 与 `IncludeBlocks: Preserve` 会维持这一顺序）：

1. 框架内部头文件，使用相对路径，如 `"../foundation/geometry.hpp"`；
2. 第三方依赖，如 `<raylib.h>`、`<catch2/catch_test_macros.hpp>`；
3. 标准库，如 `<memory>`、`<string_view>`。

模块**外部**使用者（应用、测试、playground）则通过仓库根包含，如 `<nandina/scene/scene_tree.hpp>`、`<nandina/widget/controls.hpp>`；不要从外部依赖库内相对路径。头文件应当自包含：只引入自身声明所需的类型，需要完整定义时优先用前向声明，并把前向声明集中放在对应的 `namespace` 块内。`.clangd` 打开了 `UnusedIncludes: Strict`，多余和缺失的包含都会在编辑期暴露，因此不要靠传递依赖掩盖真实的依赖关系。

## 4. API 设计

### 4.1 尾置返回类型与限定符

函数统一使用尾置返回类型，写成 `auto measure() -> NanSize`、`auto tick(float dt) -> const T&`，而不是把返回类型写在函数名前。这一写法让类型名与参数列表在长签名中更容易对齐，也便于后续把普通返回类型替换为推导或 `decltype` 表达式。构造函数与析构函数不涉及返回类型，operator 的写法保持 `auto operator=(const T&) -> T&`。

查询与读取接口应标记 `[[nodiscard]]`，因为忽略一个纯查询的结果几乎总是 bug；返回自身的链式配置方法（如 `auto set_duration(float) -> Behavior&`）则不需要。不抛异常的接口尽量标 `noexcept`，尤其是析构路径、热路径和句柄访问器。参数优先按值传递小类型，按 `const T&` 或 `std::string_view` 传递只读的大对象；能确定为编译期约束的用 concept 表达，而不是留到运行时断言。

### 4.2 语义化命名

方法名描述意图，而不是实现或设备：`set_content()`、`present()`、`on_click`、`bind_text()` 都直接说明调用者想做什么。返回值语义也要在名字里体现——`peek()` 与 `get()` 的区别、`world_bounds()` 与 `local_rect()` 的区别，都是调用者必须一眼看懂的契约。避免为了复用而发明含义模糊的 `process()`、`handle()`，它们会迫使读者跳进实现才能确定行为。返回值语义与所有权同样相关：返回引用表示借用，返回值表示独立结果，两者不应靠文档去补救命名上的含糊。

### 4.3 authoring 与链式构建

面向应用开发者的组合 API 由 `NodeBuilder` 承担，它把「描述组件树」和「手工管理节点生命周期」分离开。使用者通过 `BuildContext::make<T>()` 创建组件，用 `.child(...)`、`.configure(...)` 描述结构，配置方法返回 `NodeBuilder&` 以支持链式书写。`NodeBuilder` 可以接受普通控件或另一个 builder，内部由 `materialize()` 统一取得节点。应用回调的安装必须绑定构建作用域生命周期（`guard_callbacks()`、`bind_scope()`），避免回调在作用域清理后仍被触发。

### 4.4 所有权与句柄

场景树节点以 `std::shared_ptr` 共享持有，`NodeBuilder<Node>::build()` 产出 `std::shared_ptr<Node>`，因此 authoring 层不应把裸指针所有权转移当作主要心智模型。`std::unique_ptr` 主要保留给两类场景：PIMPL 私有实现（`std::unique_ptr<Impl>`）和不参与共享的独占资源。只读借用一律用 `const T&`，跨对象观察用 `std::weak_ptr`。需要表达「调用方仍拥有这次托管」时使用 move-only 句柄（如浮层的 `OverlayHandle`），而不是让调用方保存容器内部指针。

### 4.5 错误处理

参数不满足前置条件时抛 `std::invalid_argument`，运行期状态不满足要求时抛 `std::runtime_error`，调用顺序或逻辑误用抛 `std::logic_error`。`assert` 只用于框架自身的内部不变量，不用于校验来自应用的输入，因为它在发布构建中会被移除。构造函数失败应通过异常报告，而不是留下半构造对象；析构与 `noexcept` 路径不得抛出。

## 5. 类型识别与零 RTTI

核心代码默认不依赖 RTTI 和宏：类型识别通过显式的虚访问器完成，基类返回 `nullptr`，派生类返回自身类型，并保持 const / 非 const 对称；不得用未经验证的 `static_cast` 假定父节点类型。组件内部服务优先经 `BuildContext` 注入，窗口、页面与组件不得通过全局单例取得服务。完整规则和例外说明见 `component_contract.md` 第 8 节，新增节点类型时应回到该节逐条核对。

## 6. 注释与语言

项目使用 `cpp_std=c++26`，可以在合适的地方使用 concepts、`std::variant` 和结构化绑定来表达约束，但要避免为了展示语言特性而增加读者负担。标识符一律使用英文；注释以「为什么」和「不变量」为主，不复述签名，且应保持同一文件内的语言一致。

每个实现文件顶部用一段简短注释说明该文件（或该模块）的职责与边界，公共声明使用 `///` 文档注释，重点写清所有权、生命周期、线程约束和边界条件，而不是重抄类型。例如浮层句柄要说明「移动即转移关闭责任」「Host 先销毁后句柄安全失效」这类无法从签名看出的约定。注释掉的死代码应删除而不是保留，需要追踪的工作用 issue 描述，不要用无出处的注释长期占位。

## 7. 格式化与静态检查

格式化由 `.clang-format` 统一，关键取值包括 `ColumnLimit: 100`、`IndentWidth: 4`、`UseTab: Never`、`PointerAlignment: Left`、`NamespaceIndentation: All`，以及 `BreakBeforeBraces: Custom`（命名空间左花括号换行）。提交前应对改动文件运行 clang-format；不要为了格式化而重排未改动的代码，以免把功能变更淹没在噪声 diff 中。

`.clang-tidy` 启用了 `readability-identifier-*`（类型 `CamelCase`，命名空间、函数、变量、成员 `lower_case`，宏与全局常量 `UPPER_CASE`），以及 `modernize-use-override`、`modernize-use-nullptr`、`modernize-use-emplace`、`bugprone-use-after-move`、`bugprone-dangling-handle` 等检查。新增代码不应引入新的告警；若必须抑制，应在代码中说明原因，而不是关闭整条检查。

## 8. 测试门槛

测试位于 `tests/` 下的 `*_tests.cpp`，使用 Catch2 的 `TEST_CASE` / `CHECK` / `REQUIRE` 宏编写。新增或修改公共 API 必须在同一次变更中补上测试，覆盖正常路径、边界值与无效参数，并尽量通过公共 API 驱动，而不是直接摆弄内部状态。组件进入 recommended 之前的完整测试清单见 `component_contract.md` 第 10 节；本文只强调底线：没有测试的公共 API 视为未完成。

## 9. 提交前复查

改动完成后，按以下顺序自查一遍：命名是否遵循本文与现有代码；新头文件是否有完整路径的保护宏且自包含；函数是否使用尾置返回类型并补齐 `[[nodiscard]]` / `noexcept`；所有权与生命周期是否在类型和注释中说清；是否复用了已有组件和 primitive；格式化与静态检查是否有新增告警；公共 API 是否有对应测试。若某项规则在当前代码中尚未统一，应先对齐代码再更新本文，而不是让文档和实现各说各话。
