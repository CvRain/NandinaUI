# NandinaUI - showcase
## 此子项目用于以中文展示组件的基本用法

运行 `./buildDir/showcase/nandina_showcase`。页面按 `nav_group` 分组：

- **手册**：首页、锚点；
- **内容与展示**：标签（Label）、排版（Typography）、图片（Image）；
- **输入与焦点**：输入与焦点。

首页包含「简介」「核心特性」和「组件速览」快速入口；标签页按字号阶梯、字体样式、语义颜色、
换行、溢出、对齐、响应式文本、语义逐节演示，重节通过 P4.8b 渐进式挂载；排版页演示 13 档
字号、6 档行高、7 个语义角色和正文默认；输入与焦点页演示模态提示、RadioGroup/Tabs/下拉菜单
漫游与弹窗焦点恢复。

Image 页演示 `res://` 资源图片：基础用法（覆盖 + 固定高度）、缩放模式
（拉伸/包含/覆盖对照）、包含对齐（同一张方形图的起始/居中/末尾）与着色（同一素材的
默认/着色对照）。图片来自构建期生成的资源包：

- 策略文件是 `showcase/resources/resources.toml`，扫描根为 `showcase/assets`。
- 构建 helper 把包、lock 和 `resource-location.json` 写到 `buildDir/showcase/resources`，
  不向源码树写生成文件；`application.cpp` 通过 `resource_package` 让应用自动挂载它。
- 资源包依赖资源工具链：本地 `-Dresource_tools=true`（默认）或
  `-Dexternal_nanres=<nanres>`；不需要工具的构建请用 `-Dshowcase=false`。

## 文案约定

- 面向用户的页面文案使用简体中文与全角标点；组件名、API/类型名、颜色 token、单位（px）等
  技术标识保留英文（如 `Label`、`TextField`、`foreground`、`TextSize::xl`）。
- 页面标题使用「中文（English）」双语形式（如「标签（Label）」），侧栏导航使用中文短名。
- 多行说明/正文显式使用 `theme::Leading::normal`（1.5），与 P4.6 Step 2b 的 body 默认行高
  一致，避免中文段落显得拥挤；单行标题与控件标签保持紧凑行高。
- 新增文案请同时检查 1200×800 与高视口截图，确认没有截断、溢出或行距回退。

请在 Hyprland 上检查正常往返、快速反向、切页返回和切换主题后的体验。
自动化测试覆盖卡顿帧的积分正确性；本页面不主动阻塞 UI 来模拟卡顿。
