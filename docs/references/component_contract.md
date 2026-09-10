# 组件公共契约

本文定义 NandinaUI 公开组件应遵循的共同规则。目标不是让所有组件拥有相同 API，而是让相同概念在不同组件中保持一致，减少使用成本和后续兼容性调整。

## 1. 先定义语义，再定义外观

组件名称应表达用户意图。Button 执行动作，Checkbox 表达独立布尔选择，Switch 表达立即生效的设置，Select 表达从集合中单选。

如果能力不属于某个组件的固有语义，应通过组合获得。例如双击和拖拽由 GestureArea 提供，而不是复制到 Label、Image、Card 和 Button。

只有满足以下条件时才新增公开组件：

- 它具有清晰且可测试的交互或展示语义；
- 单纯组合现有组件会反复产生相同样板代码；
- 它需要统一的键盘、焦点、无障碍或主题行为；
- 它不是只服务某一个页面的业务视图。

## 2. 状态模型

公开状态应区分以下来源：

- **配置状态**：应用主动设置，例如 disabled、read-only、invalid；
- **值状态**：可被应用读取或绑定，例如 checked、value、selected index；
- **瞬时状态**：由输入系统维护，例如 hovered、pressed、focused；
- **派生视觉状态**：由前三者解析，不单独作为应用事实来源。

设置同一个值应尽量保持幂等。程序调用 setter 是否发出 change 必须在同族组件中一致；推荐仅把用户操作视为 change，程序同步使用 setter 更新状态但不冒充用户输入。

## 3. 事件与响应式绑定

事件名称描述语义，不描述底层设备：

- 动作组件使用 `on_click` 或更准确的领域动作；
- 值组件使用 `on_change`，并提供对应的 reactive event；
- 文本提交使用 `on_submit`；
- 关闭型组件使用 `on_close` 或 `on_dismiss`，并明确二者差别。

可变值组件应逐步统一提供：

1. 读取当前值；
2. 程序化 setter；
3. 用户变化 callback；
4. 可订阅的 `reactive::Event<T>`；
5. `BuildContext::make()` 对 `Signal<T>` 的双向绑定适配。

通过 NodeBuilder 安装的应用回调必须受到构建作用域生命周期保护。

## 4. 组合与槽位

只有一个内容区域的组件使用 `.child()`。具有固定语义区域的组件使用命名槽位，例如 header、content、footer 或 actions。

槽位应满足：

- 接受普通控件或 NodeBuilder；
- 只描述结构职责，不重复布局系统；
- 缺省槽位具有合理表现；
- 替换槽位时安全处理场景树生命周期；
- 不要求应用访问组件内部节点才能完成常见配置。

## 5. 输入、焦点与键盘

所有交互组件必须定义：

- 鼠标或指针行为；
- Tab 焦点策略；
- Enter、Space、Escape 与方向键中适用的部分；
- disabled 状态下是否可聚焦、是否接收事件；
- 输入取消、控件销毁和离开场景树时的状态复位；
- 与父级滚动、手势和浮层的事件竞争关系。

原始输入由 PointerArea 观察，复合手势由 GestureArea 识别，语义组件不应重复实现通用手势集合。

## 6. 无障碍语义

公开交互组件必须提供正确的 role、label、value、state 和 action。键盘行为与语义 action 应汇聚到同一业务路径，避免鼠标可用但辅助技术无法操作。

复合组件还需定义子节点是独立暴露、合并到父节点还是隐藏。新增组件时至少测试一次语义树输出和一个语义 action。

## 7. 主题和视觉覆盖

推荐的样式解析优先级为：

```text
DesignSystem 默认值
→ 语义变体与尺寸
→ selector rule / typed recipe override
→ 实例 visual property
→ animation / binding
```

组件应优先公开 tone、treatment、size 等有限语义变体。不要为每个颜色、圆角和状态层新增平铺 setter；实例微调使用 typed visual property，跨实例规则使用 DesignSystem 或 recipe override。

必须测试亮色/暗色切换、主题替换和显式实例覆盖的优先级。

## 8. Builder 与头文件

常用操作应由 NodeBuilder 提供短方法，低频或成组配置可以通过 `.configure()` 完成。ComponentTraits 负责让 `BuildContext::make<T>()` 使用组件的推荐构造路径，并注入主题、资源、Graph 等上下文依赖。

推荐组件必须能通过 `<nandina/widget/controls.hpp>` 使用。primitive 和 internal 类型不应无意间成为入门示例的依赖。

## 9. 测试门槛

组件进入 recommended 前至少覆盖：

- 默认构造、测量和布局；
- 鼠标与键盘主路径；
- disabled/read-only/invalid 等适用状态；
- callback、reactive event 与 Signal 绑定；
- BuildContext 回调生命周期；
- 主题切换和 recipe override；
- 语义属性与语义 action；
- 从场景树移除或销毁时的清理；
- 边界值和无效参数。

## 10. 兼容性与文档

Getting Started 只使用 recommended API。实验性组件必须在组件文档中标记，不应成为后续教程必须依赖的基础。

破坏性调整前应先提供替代 API 和迁移说明；如果条件允许，保留一个发布周期的弃用入口。内部重构不应迫使应用改写构建器代码。

每个组件的公开文档必须与测试中的推荐构建方式一致。示例应短小、可编译，并优先展示语义接口而不是内部节点操作。
