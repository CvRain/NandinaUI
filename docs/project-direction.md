# Project Direction

## 项目定位

NandinaUI 将从“Qt/QML 组件库”演进为“可扩展 UI runtime + design-system-driven widgets”的正式项目。

## 从 NandinaUI(QML) 到正式项目的演进原因

1. QML 阶段验证了主题、语义命名、组件组合方式的有效性。
2. 同时也暴露出对既有绑定机制与控件生态的强依赖，限制了 runtime 抽象与跨语言能力。
3. 若继续在旧结构上迭代，会放大技术债并弱化架构边界。

## 想解决的问题

- 继续保留 Qt/QML 的工程效率优点（迭代快、可视化强）。
- 规避对单一 UI 框架机制的深度绑定。
- 将“组件库”提升为“runtime + 组件层 + 应用层”清晰分层。
- 预留对脚本层（TypeScript/Lua 等）与多语言绑定的演进空间。

## 设计哲学

- **Design-system-first**：先 token/theme/semantic API，再具体 widget。
- **组合优于继承**：鼓励原语拼装，减少深继承组件树。
- **语义 API 优先**：接口表达业务语义，不暴露实现细节。
- **现代响应式**：围绕 State/Effect/Prop 等抽象建设数据驱动更新。
- **脚本层可扩展**：将脚本支持视为架构能力，而非后期补丁。

## 决策状态

- **已决定**：QML 旧主线归档，不再作为 `main` 的开发基底。
- **建议方向**：吸收 `nandina_experiment` 中分层与响应式设计思路。
- **待验证**：具体脚本方案、渲染后端组合、语言绑定策略。

