# 组件文档覆盖率

> 本文件是**进度看板**，不是组件文档。目的是让"哪些组件还没有使用文档"一眼可见。
> 写完一个组件文档后，把对应行的 `—` 改成 `✅` 并补上链接。

## 现状

- 公开组件：**24**（有 `ComponentTraits`、可供应用直接使用）
- 已有使用文档：**7**

## 分工约定

本项目约 70% 代码由 AI 编写。为了让成果可靠、可验证、可实际使用，分工是：

| 产出 | 负责人 |
| --- | --- |
| 框架代码、内部测试 | AI |
| **示例程序、使用文档、说明文字** | **人工手写** |
| 基于本框架编写的应用 | 人工手写 |

因此：AI 可以提供**模板、提纲、占位符、符号名与默认值的核对**，但**不代写叙述性正文**。
模板见 [`_template.md`](_template.md)。

## 文档分层

| 目录 | 面向 | 性质 |
| --- | --- | --- |
| `docs/getting_started/` | 初次使用者 | 循序渐进的学习路径 |
| `docs/components/` | 使用者 | 组件使用手册（本看板覆盖的范围） |
| `docs/references/` | 框架开发者 | **开发手册**：架构决策、契约、迁移记录、构建约束 |

> `docs/references/` 不是给框架使用者深入学习的材料。面向使用者的深入内容应有独立归属，
> 不要塞进 references。

## 覆盖情况

| 组件 | 文档 | 优先级 |
| --- | --- | --- |
| `Button` | ✅ [button.md](button.md) | 高 |
| `TextField` | — | 高 |
| `Select` | — | 高 |
| `Card` | — | 高 |
| `Badge` | — | 高 |
| `Switch` | — | 高 |
| `Checkbox` | — | 高 |
| `Label` | — | 高 |
| `Spinner` | ✅ [spinner.md](spinner.md) | 中 |
| `Skeleton` | ✅ [skeleton.md](skeleton.md) | 中 |
| `EmptyState` | ✅ [empty_state.md](empty_state.md) | 中 |
| `Alert` | ✅ [alert.md](alert.md) | 中 |
| `Slider` | ✅ [slider.md](slider.md) | 中 |
| `Dialog` | ✅ [dialog.md](dialog.md) | 中 |
| `Tabs` | — | 中 |
| `Tooltip` | — | 中 |
| `ProgressBar` | — | 中 |
| `Divider` | — | 中 |
| `Chip` | — | 中 |
| `Avatar` | — | 中 |
| `Image` | — | 中 |
| `RadioButton` | — | 中 |
| `PointerArea` | ✅ [pointer_and_gesture_areas.md](pointer_and_gesture_areas.md) | 中 |
| `GestureArea` | ✅ [pointer_and_gesture_areas.md](pointer_and_gesture_areas.md) | 中 |

> 交叉主题文档（不属于单个组件）：[selection_and_navigation.md](selection_and_navigation.md)
> —— 选择与导航类组件的键盘模型。

## 建议顺序

1. **高优先级 8 个**（最高频、且当前完全无文档）：`TextField`、`Select`、`Card`、`Badge`、
   `Switch`、`Checkbox`、`Label`、以及 `Tabs`；
2. 中优先级按实际被示例程序用到的顺序补；
3. 每补一个组件文档，同步更新本文件的 ✅ 与 `README.md` 索引表。

## 待办

- [ ] 确认 `docs/components/README.md` 的索引表与本看板一致
- [ ] 决定是否新增面向"深入使用"的独立文档区（用于放不属于开发手册的内容）
