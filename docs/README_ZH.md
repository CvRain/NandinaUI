# NandinaUI 文档（中文）

> 当前主线：V2 重构

NandinaUI 正在从 V1 迁移到 V2：
- 组件逻辑参考 shadcn 的组合式思想。
- UI 风格参考 Svelte Skeleton 的简洁视觉体系。
- 主题系统继续基于 Catppuccin，并逐步完善 Token 层。

完整说明请优先阅读：
- [项目主 README](../README.md)

## 当前可用模块
- Nandina.Window
- Nandina.Theme
- Nandina.Color
- Nandina.Core

## 快速运行示例

```bash
cmake -S .. -B ../build/Debug
cmake --build ../build/Debug -j4
../build/Debug/example/NandinaExampleApp
```

## 说明
- 文档正在跟随 V2 持续更新。
- 若发现示例与文档不一致，请以仓库代码和主 README 为准，并欢迎提 Issue。
