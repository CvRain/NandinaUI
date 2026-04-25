# 编码与 API 规范

> 文档状态：草稿（Draft）

## 1. 命名空间分层规范（Namespace Layering）

NandinaUI 使用**三级命名空间**结构：**模块路径 = 命名空间层级**。

| 模块层级                     | 命名空间层级                     | 示例                            |
|--------------------------|----------------------------|-------------------------------|
| `nandina.<sub>`          | `nandina::<sub>`           | `nandina::runtime`            |
| `nandina.<sub>.<module>` | `nandina::<sub>::<module>` | `nandina::runtime::NanWindow` |

**设计原则**：
- **模块名即命名空间**：模块路径去掉 `.cppm` 后缀就是命名空间路径。
- **避免全局污染**：所有公共类型必须位于 `nandina` 下的子命名空间。
- **闭合性**：子命名空间可以 `using namespace` 引用兄弟命名空间，但不穿透顶层。

### 1.1 各子命名空间职能

| 子命名空间               | 职能说明                                       | 导出模块文件                                  |
|---------------------|--------------------------------------------|------------------------------------------|
| `nandina::types`    | 跨模块基础枚举（Axis、Align、Visibility 等）           | `nan_types`                              |
| `nandina::geometry` | 二维几何原语（点、尺寸、矩形、内边距、约束）                  | `nan_point`、`nan_size`、`nan_rect`、`nan_insets`、`nan_constraints` |
| `nandina::color`    | 颜色工具函数与 traits（parse_hex、ColorTupleTraits） | `nan_color_schema`、`nan_color_converter` |
| `nandina::log`      | 日志框架（基于 spdlog）                            | `log`                                    |
| `nandina::runtime`  | 运行时核心（窗口、Widget）                           | `nan_window`、`nan_widget`                |
| `nandina::app`      | 应用程序层（NanAppWindow、NanApplication）         | `application`                            |

### 1.2 类型命名约定

| 类型类别               | 命名规则                  | 示例                           |
|--------------------|-----------------------|------------------------------|
| 值对象（Value Objects） | `Nan` 前缀 + 颜色空间名      | `NanRgb`、`NanHsv`、`NanPoint` |
| 统一容器（Container）    | `Nan` + `Color` 后缀    | `NanColor`                   |
| 基础窗口               | `Nan` + 语义功能名         | `NanWindow`、`NanWidget`      |
| 应用层窗口              | `Nan` + `App` + 语义功能名 | `NanAppWindow`               |
| 异常                 | `Nan` 前缀 + `Error` 后缀 | `NanRuntimeError`            |

### 1.3 模块导入约定

```cpp
// 推荐：通过子命名空间限定引入
import nandina.geometry.nan_point;    // → nandina::geometry::NanPoint
import nandina.runtime.nan_window;    // → nandina::runtime::NanWindow
import nandina.app.application;       // → nandina::app::NanAppWindow

// 在命名空间内可以使用 using namespace 简化引用
export namespace nandina::app {
    using namespace nandina::geometry;  // 可选：简化同模块引用
}
```

## 2. 模块划分规范（Module Partitioning）

### 2.1 模块粒度

| 粒度       | 说明            | 适用场景                      |
|----------|---------------|---------------------------|
| **原子模块** | 单文件，定义单一类型/概念 | 值对象 traits、工具函数           |
| **聚合模块** | 重新导出多个子模块     | 主入口模块（如 `nan_color.cppm`） |

**示例**：
```cpp
// 原子模块：仅定义 NanRgb 结构体
export module nandina.foundation.nan_color_schema;

// 聚合模块：重新导出 NanRgb、NanHsv、NanOklab 等
export module nandina.foundation.color;
export import nandina.foundation.nan_color_schema;  // 继承原子模块的导出
export import nandina.foundation.nan_color_converter;
```

### 2.2 模块依赖规则

```
┌─────────────────────────────────────────────────────────┐
│                      app                               │
│         (NanAppWindow, NanApplication)                 │
└───────────────────────┬────────────────────────────────┘
                        │ imports
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
   ┌─────────┐   ┌───────────┐   ┌───────────┐
   │ runtime │   │  log      │   │ foundation│
   │(nan_win │   │           │   │  (color,  │
   │dget)    │   │           │   │  point)   │
   └────┬────┘   └───────────┘   └───────────┘
        │ imports
        ▼
   ┌───────────┐
   │  log      │
   │           │
   └───────────┘
```

**规则**：
- `foundation` **无上游依赖**。
- `runtime` 仅依赖 `log`（通过 PIMPL 隔离 SDL）。
- `app` 依赖 `runtime`、`log`、`foundation`。

## 3. C++ Modules (C++20) 实现约定

### 3.1 模块声明顺序

```cpp
module;

// ─────────────────────────────────────────────────────────
// 全局模块片段（GMF）：所有消费者可见的头文件
// ─────────────────────────────────────────────────────────
#include <thorvg-1/thorvg.h>
#include <memory>

export module nandina.example.module;

// ─────────────────────────────────────────────────────────
// 导出接口
// ─────────────────────────────────────────────────────────
export namespace nandina::example {
    // 导出的类型声明
}

// ─────────────────────────────────────────────────────────
// 内部实现（对消费者不可见）
// ─────────────────────────────────────────────────────────
namespace {
    // 内部辅助函数
}

namespace nandina::example {
    // 非导出实现细节
}
```

### 3.2 全局模块片段（GMF）原则

| 类型               | 是否放入 GMF | 原因                 |
|------------------|----------|--------------------|
| 消费者头文件（如 ThorVG） | ✅ 是      | 接口层需要完整类型信息        |
| 实现库头文件（如 SDL3）   | ❌ 否      | 使用 PIMPL 隔离，不泄漏至接口 |

### 3.3 PIMPL 模式

```cpp
// 接口层（.cppm）
export namespace nandina::example {
    class ExampleWindow {
    public:
        auto draw(tvg::SwCanvas& canvas) -> void;
    private:
        struct Impl;                          // 前向声明
        std::unique_ptr<Impl> m_impl;         // 私有实现
    };
}

// 实现层（.cpp）
module nandina.example.window;

#include <SDL3/SDL.h>  // 仅在此可见

namespace nandina::example {
    struct ExampleWindow::Impl {
        SDL_Window* window;  // 实现细节
    };
}
```

**优势**：
- 消费者代码不会意外依赖 SDL 类型。
- 未来可无感替换渲染后端（如从 SDL 切换到 GLFW）。

## 4. API 设计原则

### 4.1 Godot-like 生命周期钩子

```cpp
class NanWindow {
protected:
    virtual auto on_ready() -> void {}           // 首次循环前
    virtual auto on_update(double dt) -> void {} // 每帧更新
    virtual auto on_draw(tvg::SwCanvas&) -> void {} // 每帧绘制
    virtual auto on_resize(int w, int h) -> void {} // 窗口大小变化
};
```

**使用模式**：
```cpp
class MyWindow final : public NanWindow {
protected:
    auto on_draw(tvg::SwCanvas& canvas) override {
        // 添加 ThorVG 图元
    }
};

MyWindow window;
window.run();  // 主循环自动调度 on_* 钩子
```

### 4.2 Builder 模式

```cpp
class NanWindow {
public:
    class Builder {
    public:
        [[nodiscard]] auto set_title(std::string_view) -> Builder&;
        [[nodiscard]] auto set_size(int, int) -> Builder&;
        [[nodiscard]] auto build() -> NanWindow;
    };
};

// 使用示例
auto window = NanWindow::Builder{}
    .set_title("My Window")
    .set_size(1280, 720)
    .build();
```

### 4.3 返回值语义

| 场景       | 返回类型                 | 示例                        |
|----------|----------------------|---------------------------|
| 拥有所有权    | `std::unique_ptr<T>` | `NanWidget::add_child()`  |
| 借用/引用    | `T&` / `const T&`    | `NanRect::intersect()`    |
| 计算结果（轻量） | 值语义 / `std::pair`    | `NanPoint::distance_to()` |
| 查询（不变）   | `[[nodiscard]]` 标记   | `NanWindow::width()`      |

### 4.4 约束与 Concepts

```cpp
// 约束颜色空间类型
template<color::ColorSpace T>
auto NanColor::from(const T& color) -> NanColor;

// 使用 concept 约束
template<typename T>
concept TupleLikeColor = requires(T t) {
    std::get<0>(t);
    { std::tuple_size<T>::value } -> std::same_as<std::size_t>;
};
```

## 5. 文档与注释约定

### 5.1 Doxygen 注释格式

```cpp
/**
 * @brief 简短描述（一句话）
 *
 * 详细描述（可选）。
 *
 * @param canvas 绘制目标画布
 * @return 是否绘制成功
 *
 * @note 注意事项
 * @see 相关链接
 */
export auto draw(tvg::SwCanvas& canvas) -> bool;
```

### 5.2 Issue 追踪注释

```cpp
// Issue 017: 最小渲染闭环
auto present_frame() -> void {
    // ...
}
```

## 6. 错误处理

| 场景                 | 处理方式                             |
|--------------------|----------------------------------|
| 构造函数中不可恢复错误        | 抛出 `std::runtime_error`          |
| 可恢复错误（如 resize 失败） | 记录日志，跳过当前帧                       |
| 逻辑验证失败             | 使用 `std::logic_error` 或 `assert` |
| API 误用             | 依赖编译器错误和文档说明                     |

## 7. 性能注意事项

| 注意点   | 建议                                           |
|-------|----------------------------------------------|
| 每帧分配  | 避免在 `on_draw()` 中分配内存                        |
| 模板实例化 | 颜色转换 traits 尽量内联                             |
| 锁竞争   | `runtime_bootstrap()` 使用 `std::mutex` 保护全局状态 |
| 模块依赖  | 避免循环依赖，优先使用前向声明                              |

---

**最后更新**：2026-04-22
**维护者**：CvRain
