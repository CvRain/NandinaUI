# Vendored 源码约定

每个新 vendor 目录放置自己的 `.clang-format`，至少包含：

```yaml
DisableFormat: true
```

让使用仓库配置的编辑器、手工 clang-format 和格式门禁统一保留上游排版，不在
`tools/check_format.py` 中逐个维护目录白名单。显式覆盖 style 仍可绕过该设置，
“原样副本”应另以来源校验或文件摘要验证。

各目录的 README 记录版本、来源与摘要、许可信息、必要本地补丁、编译选项和升级验证流程；
不要直接在第三方源码里做无关格式整理。现有示例见 [SQLite](sqlite3/README.md)。

格式工具固定为 23.1.1，取得与运行方式见 [编码规范](../docs/references/coding_conventions.md)
和 [CI format 任务](../.github/workflows/ci.yml)。CI 当前显式安装
`clang-format==23.1.1`；不要因镜像包版本不同而静默换版或调整门禁期望版本。
