# Page / Router 合约

> 状态：已决定（MVP）
> 来源：改写自旧 C++ `archive/docs/page-contract.md`，吸收 Angular 的 page/router 思路。

## 目标范围

app 层提供三件套：

- `Page`：页面元数据 + `build()` 工厂。
- `Router`：页面注册、当前 route key、`navigateTo()`、导航通知、按需构建当前页面。
- `PageHost`：订阅导航事件、延迟换页、bounds 传播、layout 刷新。

## 合约

### Page 是「页面描述 + 构建入口」，不是长期存活实例
`Page` 只承担三类职责：
- 提供稳定的 `routeKey()`。
- 提供展示元数据（`title()`、`iconType()`）。
- 提供 `build()`，按需创建页面根组件。

它更接近「页面描述对象」，而不是「可反复 enter/leave 的组件实例」。

### 默认策略是「每次进入重新构建」
- `navigateTo()` 切换当前 key。
- `PageHost` 在下一帧绘制前延迟执行页面替换。
- 替换时重新调用目标 `Page.build()`。

即：默认不缓存页面根组件、不保留页面内部局部状态。状态恢复由上层显式管理，而不是 Router 隐式 keep-alive。
这是 MVP 阶段的有意选择，避免在 layout/theme/authoring 仍收口时过早引入缓存生命周期复杂度。

### Router 是「注册表 + 当前路由」，不是历史栈
当前承诺：注册页面、按 key 查找、维护当前活动 route、触发导航回调。
当前**不**承诺：`push/pop/back/replace`、历史栈、URL 路径解析、参数序列化、缓存策略。
因此 `navigateTo("...")` 应理解为「切换当前展示页面」。

### Page 元数据是单一数据源
`routeKey()` / `title()` / `iconType()` 属于页面自身契约。
sidebar / registry / 文档导航不应各自维护重复元数据，统一由 page metadata 驱动。

## 与 authoring 的关系

- 页面类型实现 `Page`，`build()` 内部返回基于 `mount(Node)` 的 authoring 根组件。
- Page 是导航层契约；Node / Ref / mount 是页面内部 authoring 契约。两者分层演进，不要把 Router 扩展需求压到 authoring API 上。

## 演进顺序

在扩 Router API 之前，先：
1. 补齐 app / showcase 导航测试面。
2. 用 showcase 持续验证 authoring 体验。
3. 再决定页面缓存、typed params、历史栈。

### 待决策项
- **页面缓存**：默认保持 `rebuild-on-enter`；若引入缓存，用显式策略（如 `keep_alive`）而非隐式全局缓存。
- **typed params**：走类型安全路线（页面定义自己的 `Params` 值类型），不要用字符串字典 + 运行时解析。
- **历史栈**：只有在页面缓存与状态保留语义明确后才推进 `push/back/replace`。

## Do / Don't

**Do**：把 Page 视为描述对象；把 `build()` 视为根组件工厂；把 metadata 当导航 UI 单一数据源；用测试锁住 `navigateTo() + PageHost` 行为。

**Don't**：假定页面会自动缓存；在没文档化语义前加 `push/pop/replace`；让 sidebar/registry 复制页面元数据；把参数建立在字符串字典上。
