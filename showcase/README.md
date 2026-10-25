# NandinaUI - showcase
## 此子项目用以展示组件的基本使用方法

运行 `./buildDir/showcase/nandina_showcase`。页面按 `type` 分组：首页、锚点属于“手册”，
Label 与 Image 属于“内容与展示”。

首页“动效体验”提供两组固定命中区域：

- Tween：透明度从 0.35 匀速变到 1；Spring：相同目标使用欠阻尼弹簧。
- 两组都附带围绕左侧中心的轻微缩放；快速移入、移出可观察中途改目标的连续性。
- 下方“组件速览”保留 scale 悬浮反馈，观察相邻条目是否稳定、文字是否保持固定字号。

Image 页演示 `res://` 资源图片：Basic usage（cover + 固定高度）、Scale modes
（stretch/contain/cover 对照）、Contain alignment（同一张方形图的 start/center/end）
与 Tint（同一素材的默认/着色对照）。图片来自构建期生成的资源包：

- 策略文件是 `showcase/resources/resources.toml`，扫描根为 `showcase/assets`。
- 构建 helper 把包、lock 和 `resource-location.json` 写到 `buildDir/showcase/resources`，
  不向源码树写生成文件；`application.cpp` 通过 `resource_package` 让应用自动挂载它。
- 资源包依赖资源工具链：本地 `-Dresource_tools=true`（默认）或
  `-Dexternal_nanres=<nanres>`；不需要工具的构建请用 `-Dshowcase=false`。

请在 Hyprland 上检查正常往返、快速反向、切页返回和切换主题后的体验。
自动化测试覆盖卡顿帧的积分正确性；本页面不主动阻塞 UI 来模拟卡顿。
