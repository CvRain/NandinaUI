// nandina.hpp —— NandinaUI 的 C++ 前端层（header-only）
//
// 在 C ABI（nandina_abi.h）之上做 RAII + 链式声明式封装，复刻 Zig
// `app.authoring`
// 的体验：`nd::column({.gap=20}).child(nd::label(...)).child(nd::button(...))`。
//
// 设计：
//   - 仅依赖 nandina_abi.h，绝不触碰 Zig 内部符号。
//   - Widget 是对 nandina_node_t* 的轻量「非拥有视图」；节点所有权归 Tree/App，
//     由 core 在销毁时递归释放（与 Zig 侧一致）。
//   - 回调用 CallbackRegistry 托管 std::function 的生命周期 + trampoline 桥接到
//     C 的「函数指针 + void* user_data」。
//   - 工厂函数从「当前 Graph」取响应式图；用 nd::Graph 构造时自动设为当前。
//
// 用法见 showcase/cpp/main.cpp。

#ifndef NANDINA_HPP
#define NANDINA_HPP

#include "nandina_abi.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace nd {

// ── 基础值类型
// ────────────────────────────────────────────────────────────────

/// 颜色：0xRRGGBBAA。提供 hex 构造便利。
using Color = std::uint32_t;

/// 从 0xRRGGBB 构造不透明颜色（alpha = 0xFF）。
constexpr Color rgb(std::uint32_t hex) { return (hex << 8) | 0xFF; }
/// 从 0xRRGGBBAA 构造。
constexpr Color rgba(std::uint32_t hex) { return hex; }
/// 调整 alpha（0..255）。
constexpr Color withAlpha(Color c, std::uint8_t a) {
  return (c & 0xFFFFFF00u) | a;
}

struct Insets {
  float left = 0, top = 0, right = 0, bottom = 0;
  static Insets all(float v) { return {v, v, v, v}; }
  static Insets symmetric(float h, float v) { return {h, v, h, v}; }
  nandina_insets_t toAbi() const {
    return nandina_insets_t{left, top, right, bottom};
  }
};

// ── 错误处理
// ──────────────────────────────────────────────────────────────────

/// ABI 调用失败时抛出。
struct Error {
  std::string message;
};

inline void check(nandina_error_t err) {
  if (err != 0)
    throw Error{nandina_error_message(err)};
}

// ── 回调托管
// ──────────────────────────────────────────────────────────────────
//
// C ABI 回调是「函数指针 + void* user_data」。这里把 std::function
// 堆分配并登记， 保证其生命周期覆盖整个程序（节点存活期）；trampoline 把 C
// 回调转回 std::function。

class CallbackRegistry {
public:
  static CallbackRegistry &instance() {
    static CallbackRegistry reg;
    return reg;
  }

  std::function<void()> *retainClick(std::function<void()> fn) {
    auto *p = new std::function<void()>(std::move(fn));
    clicks_.emplace_back(p);
    return p;
  }

  std::function<void(bool)> *retainBool(std::function<void(bool)> fn) {
    auto *p = new std::function<void(bool)>(std::move(fn));
    bools_.emplace_back(p);
    return p;
  }

private:
  std::vector<std::unique_ptr<std::function<void()>>> clicks_;
  std::vector<std::unique_ptr<std::function<void(bool)>>> bools_;
};

namespace detail {
inline void clickTrampoline(void *ud) {
  (*static_cast<std::function<void()> *>(ud))();
}
inline void boolTrampoline(void *ud, bool checked) {
  (*static_cast<std::function<void(bool)> *>(ud))(checked);
}
} // namespace detail

// ── 当前
// Graph（工厂函数从此取响应式图）─────────────────────────────────────────

namespace detail {
inline nandina_graph_t *&currentGraphSlot() {
  static thread_local nandina_graph_t *g = nullptr;
  return g;
}
} // namespace detail

inline nandina_graph_t *currentGraph() { return detail::currentGraphSlot(); }

/// RAII 响应式图。构造时设为「当前 Graph」，析构时还原。
class Graph {
public:
  Graph() {
    check(nandina_graph_create(&g_));
    prev_ = detail::currentGraphSlot();
    detail::currentGraphSlot() = g_;
  }
  ~Graph() {
    if (detail::currentGraphSlot() == g_)
      detail::currentGraphSlot() = prev_;
    if (g_)
      nandina_graph_destroy(g_);
  }
  Graph(const Graph &) = delete;
  Graph &operator=(const Graph &) = delete;

  nandina_graph_t *raw() const { return g_; }
  void makeCurrent() { detail::currentGraphSlot() = g_; }

private:
  nandina_graph_t *g_ = nullptr;
  nandina_graph_t *prev_ = nullptr;
};

// ── Widget —— 节点的链式视图
// ──────────────────────────────────────────────────

class Widget {
public:
  explicit Widget(nandina_node_t *node) : node_(node) {}

  nandina_node_t *raw() const { return node_; }

  /// 追加一个子节点，返回自身以支持链式。
  Widget &child(Widget c) {
    check(nandina_node_add_child(node_, c.node_));
    return *this;
  }

  /// 一次追加多个子节点。
  template <typename... W> Widget &children(W... cs) {
    (child(cs), ...);
    return *this;
  }

private:
  nandina_node_t *node_;
};

// ── 容器工厂
// ──────────────────────────────────────────────────────────────────

struct ColumnOpts {
  float gap = 0;
};
inline Widget column(ColumnOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_column_create(o.gap, &n));
  return Widget{n};
}

struct RowOpts {
  float gap = 0;
};
inline Widget row(RowOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_row_create(o.gap, &n));
  return Widget{n};
}

inline Widget stack() {
  nandina_node_t *n = nullptr;
  check(nandina_stack_create(&n));
  return Widget{n};
}

struct SurfaceOpts {
  Color bg_color = rgb(0x1E1E2E);
  float corner_radius = 0;
  Insets padding{};
  Color border_color = 0;
  float border_width = 0;
};
inline Widget surface(SurfaceOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_surface_create(currentGraph(), o.bg_color, o.corner_radius,
                               o.padding.toAbi(), o.border_color,
                               o.border_width, &n));
  return Widget{n};
}

struct PanelOpts {
  Color bg_color = rgb(0x11111B);
  float corner_radius = 6;
  Insets padding = Insets::all(12);
  Color border_color = rgb(0x313244);
  float border_width = 1;
};
inline Widget panel(PanelOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_panel_create(currentGraph(), o.bg_color, o.corner_radius,
                             o.padding.toAbi(), o.border_color, o.border_width,
                             &n));
  return Widget{n};
}

// ── 内容工厂
// ──────────────────────────────────────────────────────────────────

struct LabelOpts {
  Color color = rgb(0xCDD6F4);
  float font_size = 14;
};
inline Widget label(const char *text, LabelOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_label_create(currentGraph(), text, o.color, o.font_size, &n));
  return Widget{n};
}

struct ButtonOpts {
  Color bg_color = rgb(0x89B4FA);
  Color bg_hover_color = rgb(0x74C7EC);
  Color bg_pressed_color = rgb(0x89DCEB);
  Color text_color = rgb(0x1E1E2E);
  float font_size = 14;
  float corner_radius = 6;
  Insets padding = Insets::symmetric(20, 10);
  std::function<void()> on_click = nullptr;
};
inline Widget button(const char *text, ButtonOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_button_create(
      currentGraph(), text, o.bg_color, o.bg_hover_color, o.bg_pressed_color,
      o.text_color, o.font_size, o.corner_radius, o.padding.toAbi(), &n));
  if (o.on_click) {
    auto *ud = CallbackRegistry::instance().retainClick(std::move(o.on_click));
    nandina_button_set_on_click(n, &detail::clickTrampoline, ud);
  }
  return Widget{n};
}

struct CardOpts {
  Color bg_color = rgb(0x181825);
  float corner_radius = 8;
  float title_font_size = 18;
  float desc_font_size = 13;
};
inline Widget card(const char *title, const char *description,
                   CardOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_card_create(currentGraph(), title, description, o.bg_color,
                            o.corner_radius, o.title_font_size,
                            o.desc_font_size, &n));
  return Widget{n};
}

enum class IconShape : std::int32_t { Rect = 0, Circle = 1 };
struct IconOpts {
  Color color = rgb(0x89B4FA);
  float size = 16;
  IconShape shape = IconShape::Rect;
};
inline Widget icon(IconOpts o = {}) {
  nandina_node_t *n = nullptr;
  check(nandina_icon_create(currentGraph(), o.color, o.size,
                            static_cast<std::int32_t>(o.shape), &n));
  return Widget{n};
}

// ── PageHost —— 多页导航
// ──────────────────────────────────────────────────────
//
// 每个页面是一个 `std::function<Widget()>`；用 trampoline + 索引桥接到 C build
// 回调。

class PageHost {
public:
  using Builder = std::function<Widget()>;

  PageHost(std::vector<Builder> builders, std::size_t initial = 0)
      : builders_(std::move(builders)) {
    // 为每个页面准备 C 回调 + user_data（指向本 PageHost 的页面槽）。
    slots_.reserve(builders_.size());
    fns_.reserve(builders_.size());
    uds_.reserve(builders_.size());
    for (std::size_t i = 0; i < builders_.size(); ++i) {
      slots_.push_back(Slot{this, i});
    }
    for (std::size_t i = 0; i < builders_.size(); ++i) {
      fns_.push_back(&PageHost::buildTrampoline);
      uds_.push_back(&slots_[i]);
    }
    check(nandina_page_host_create(currentGraph(), fns_.data(), uds_.data(),
                                   builders_.size(), initial, &host_));
  }

  nandina_page_host_t *raw() const { return host_; }

  Widget node() const { return Widget{nandina_page_host_node(host_)}; }

  void navigateTo(std::size_t index) {
    check(nandina_page_host_navigate_to(host_, index));
  }

private:
  struct Slot {
    PageHost *host;
    std::size_t index;
  };

  static nandina_node_t *buildTrampoline(void *ud) {
    auto *slot = static_cast<Slot *>(ud);
    return slot->host->builders_[slot->index]().raw();
  }

  std::vector<Builder> builders_;
  std::vector<Slot> slots_;
  std::vector<nandina_page_build_fn> fns_;
  std::vector<void *> uds_;
  nandina_page_host_t *host_ = nullptr;
};

// ── App —— 全包窗口入口
// ────────────────────────────────────────────────────────

class App {
public:
  App(const char *title, int width, int height) {
    check(nandina_app_create(title, width, height, &app_));
  }
  ~App() {
    if (app_)
      nandina_app_destroy(app_);
  }
  App(const App &) = delete;
  App &operator=(const App &) = delete;

  nandina_app_t *raw() const { return app_; }

  /// 挂载根节点并进入阻塞主循环。
  void run(Widget root) {
    check(nandina_app_set_root(app_, root.raw()));
    check(nandina_app_run(app_));
  }

private:
  nandina_app_t *app_ = nullptr;
};

} // namespace nd

#endif // NANDINA_HPP
