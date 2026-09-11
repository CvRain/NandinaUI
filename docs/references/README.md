# 开发参考

本目录记录 NandinaUI 的开发约束和架构共识，主要读者是框架维护者及组件贡献者。应用开发者通常只需要阅读 `getting_started/` 与 `components/`。

## 当前文档

- [组件公共契约](component_contract.md)：新组件在进入推荐 API 前必须满足的状态、事件、绑定、主题、输入和无障碍要求。
- [组件开发路线图](component_roadmap.md)：当前组件盘点、基础设施依赖和推荐开发顺序。
- [浮层架构](overlay_architecture.md)：OverlayHost、portal 生命周期、分层规则与后续基础设施边界。
- [Linux 窗口后端与缩放](linux_window_scaling.md)：Wayland/X11 选择与多显示器 DPI 验证。

## 编写原则

开发参考应记录可复用的决策，而不是某次实现过程。内容需要回答以下问题：

- 为什么选择当前设计，替代方案有什么代价；
- 哪些规则属于公共兼容承诺，哪些仍是实验性约定；
- 新组件如何复用已有 primitive，而不是重复实现输入、主题或布局；
- 设计变化会影响哪些组件、测试和公开文档；
- 何种条件满足后，功能才可以从 experimental 进入 recommended。

如果一次变更只修复局部实现且不产生长期约束，应写在提交和测试中，不必创建参考文档。若变更建立了跨组件规则，应同步更新本目录中对应文档。

## 文档状态

参考文档使用以下术语：

- **recommended**：推荐给应用开发者，避免破坏性调整；
- **experimental**：方向基本成立，但允许在 alpha 阶段调整接口；
- **internal**：实现细节，不构成兼容承诺；
- **planned**：已经进入路线图，但尚无可用实现。

Getting Started 只应依赖 recommended API。组件文档可以介绍 experimental API，但必须明确标注状态。
