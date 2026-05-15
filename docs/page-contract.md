# Page / Router 合约（MVP）

> 状态：已决定（2026-05）
> 目的：在继续扩大 Router API 表面积之前，先固定当前主线对 `NanPage`、`NanRouter`、`NanPageHost` 的职责边界与后续演进方向。

## 1. 当前范围

当前主线已落地以下能力：

- `NanPage`：页面元数据 + `build()` 工厂
- `NanRouter`：页面注册、当前 route key、`navigate_to()`、导航通知、按需构建当前页面
- `NanPageHost`：订阅导航事件、延迟换页、bounds 传播、layout 刷新

对应源码：

- `app/src/nan_page.cppm`
- `app/src/nan_router.cppm`
- `app/src/nan_page_host.cppm`

## 2. 当前合约

### 2.1 `NanPage` 是“页面描述 + 构建入口”，不是长期存活的页面实例

`NanPage` 当前只承担三类职责：

- 提供稳定的 `route_key()`
- 提供展示元数据（`title()`、`icon_type()`）
- 提供 `build()`，按需创建页面根组件

这意味着当前主线中的 `NanPage` 更接近“页面描述对象”，而不是“可在生命周期内反复 enter/leave 的组件实例”。

### 2.2 页面默认策略是“每次进入重新构建”

当前 Router + PageHost 的组合语义是：

- `navigate_to()` 切换当前 key
- `NanPageHost` 在下一次 `draw()` 前延迟执行页面替换
- 替换时重新调用目标 `NanPage::build()`

也就是说，当前默认行为是：

- 不缓存页面根组件
- 不保留页面内部局部状态
- 页面切换后的状态恢复由上层显式状态管理负责，而不是由 Router 隐式 keep-alive

这是当前阶段的有意选择，不是遗漏。

原因：

- MVP 先验证 Page / Router / PageHost 的最小闭环
- 避免在 layout、theme、authoring 仍在收口时过早引入缓存生命周期复杂度
- 让 `build()` 语义保持单纯，便于测试与排错

### 2.3 当前 Router 是“注册表 + 当前路由”，不是历史栈

当前 `NanRouter` 明确只承诺：

- 注册页面
- 通过 `route_key` 查找页面
- 维护当前活动 route
- 触发导航回调

当前不承诺以下能力：

- `push/pop/back/replace`
- 历史栈
- URL 风格路径解析
- 参数序列化
- 页面缓存策略

因此，当前 `navigate_to("...")` 应理解为“切换当前展示页面”，而不是更完整的导航框架。

### 2.4 当前 Page 元数据应作为单一数据源使用

`route_key()`、`title()`、`icon_type()` 属于页面自身契约。

showcase、sidebar、registry 或未来文档导航，不应再各自维护一份重复元数据。当前 showcase 已收口到由 page metadata 驱动 sidebar 展示，这是后续 app 层的基准做法。

## 3. 与 authoring 的关系

当前建议的页面编写方式是：

- 页面类本身实现 `NanPage`
- `build()` 内部优先返回基于 `mount(Node)` 的 authoring 根组件

换句话说：

- Page 是导航层契约
- Node / Ref / mount 是页面内部 authoring 契约

两者应分层演进，不要把 Router 扩展需求直接压到 authoring API 上。

## 4. 下一阶段已决定的演进顺序

在继续扩 Router API 之前，先按下面顺序推进：

1. 补齐 app / showcase 导航测试面
2. 用 showcase 持续验证 authoring 体验
3. 再决定页面缓存、typed params、历史栈这些增强能力

原因很直接：如果先扩 `push/pop/replace/params`，但页面生命周期和 authoring 体验没有定住，后面很容易返工。

## 5. 下一阶段待决策项

### 5.1 页面缓存策略

后续需要明确：

- 是否允许页面 keep-alive
- 缓存单位是“page instance”还是“built root component”
- 缓存失效由 Router 决定，还是由 Page 元数据声明

建议方向：

- 默认仍保持 `rebuild-on-enter`
- 若引入缓存，采用显式策略而不是隐式全局缓存

例如未来可考虑：

- `PageCachePolicy::rebuild_on_enter`
- `PageCachePolicy::keep_alive`

但这不是当前 MVP 的一部分。

### 5.2 typed params

如果后续确实要支持参数化导航，建议走类型安全路线，而不是继续扩 string 驱动接口。

更推荐的方向是：

- 页面定义自己的 `Props` / `Params` 值类型
- Router 对外暴露类型安全入口
- showcase / app 层以静态页面描述组织 registry

不建议的方向：

- `std::unordered_map<std::string, std::string>` 风格参数包
- 运行时字符串解析驱动页面构建

### 5.3 历史栈

`push/back/replace` 只有在以下前提明确后才值得推进：

- 页面是否缓存
- 页面切出时状态是否保留
- 返回时是否复用先前实例

否则 `back()` 只是一个表面 API，无法给出稳定语义。

## 6. 当前 Do / Don't

### Do

- 把 `NanPage` 视为页面描述对象
- 把 `build()` 视为当前页面根组件的工厂
- 把 page metadata 当作导航 UI 的单一数据源
- 用测试锁住当前 `navigate_to() + PageHost` 的行为

### Don't

- 在当前阶段假定页面会自动缓存
- 在没有文档化语义之前先加 `push/pop/replace`
- 让 sidebar / registry / docs 再复制一份页面元数据
- 把参数设计建立在字符串字典和运行时解析上

## 7. 相关文档

- [阶段路线图](roadmap.md)
- [开发 Issue 清单](develop-issue.md)
- [组件 Authoring 与挂载 API 设计](component-authoring-and-mounting.md)
- [无显式 move 的组件组合 API（V1 草案）](component-composition-api-v1.md)