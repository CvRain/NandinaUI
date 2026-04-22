# NandinaUI Godot-like 开发范式（草案）

> 状态：**草案（Draft）**  
> 目的：定义当前阶段的应用/窗口继承式开发体验，后续会随着 runtime、widget、event 系统演进持续修订。

## 目标

- 让开发者像 Godot 一样，通过继承基类并重写生命周期方法构建 UI。
- 保持平台层细节（SDL）隐藏在 runtime，实现 API 简洁与后端可替换。
- 为后续 Widget 树与统一 Event 系统预留稳定钩子，不在 M1 过早绑定具体控件实现。

## 核心模型

- `nandina::runtime::NanWindow`：窗口与帧循环基类，负责平台事件轮询与绘制闭环。
- `nandina::NanApplication`：应用层基类，内部桥接 `NanWindow`，提供更高层的生命周期编排入口。
- `NanWindow::Builder` + `NanWindow::Config`：配置窗口参数；继承类通过 `Config` 构造。

## 生命周期（当前草案）

### Window 层

- `on_ready()`：主循环第一次迭代前调用一次。
- `on_update(double delta_seconds)`：每帧逻辑更新。
- `on_draw(tvg::SwCanvas&)`：每帧绘制回调，canvas 在回调前已清空。
- `on_resize(int, int)`：逻辑尺寸变化后触发。
- `on_close_requested()`：收到关闭请求时触发。

### Input 层（预留统一事件入口）

- `on_pointer_move(const PointerMoveEvent&)`
- `on_pointer_down(const PointerButtonEvent&)`
- `on_pointer_up(const PointerButtonEvent&)`
- `on_pointer_wheel(const PointerWheelEvent&)`
- `on_key_down(const KeyEvent&)`
- `on_key_up(const KeyEvent&)`
- `on_text_input(std::string_view)`

这些钩子当前由 SDL 事件翻译得到，后续将接入 runtime 统一 Event 类型体系。

### Application 层

- `configure()`：返回 `AppConfig`（窗口标题、尺寸、DPI 等）。
- `on_ready/on_update/on_draw/on_resize/on_close_requested`：由桥接窗口转发。main
- `on_shutdown()`：run 退出后调用（包括异常路径）。

## 推荐用法

1. 继承 `NanApplication`。
2. 覆写 `configure()` 指定窗口参数。
3. 按需覆写 `on_ready/on_update/on_draw`。
4. 在 `on_draw()` 内添加 ThorVG 图元。
5. 在输入钩子中处理交互逻辑（当前阶段建议记录状态，不直接耦合平台）。

## 设计边界（草案约束）

- 不暴露 SDL 类型到模块接口。
- 输入事件先走轻量结构体，避免与后续 Widget Event 冲突。
- 应用层不直接操作窗口底层资源，只通过生命周期与 `request_quit()` 协作。

## 后续迭代方向

- 将输入钩子接入 `runtime/event` 统一事件体系。
- 在 `NanApplication` 增加页面/场景管理协作接口。
- 引入 Widget 树后，on_draw 逐步转向 scene/draw command 提交。
