// showcase/cpp/main —— NandinaUI C++ 前端 showcase
//
// 与 showcase/zig 同一组页面的 C++ 对照实现：只用 nandina.hpp（C ABI 之上的
// 链式封装），不触碰 Zig 内部符号。验证「C++ 前端与 Zig 前端能力对齐」。

#include "nandina/nandina.hpp"

#include <array>
#include <cstdio>
#include <vector>

namespace {

// Catppuccin Mocha 调色板
struct Palette {
  nd::Color base = nd::rgb(0x1E1E2E);
  nd::Color mantle = nd::rgb(0x181825);
  nd::Color crust = nd::rgb(0x11111B);
  nd::Color text = nd::rgb(0xCDD6F4);
  nd::Color blue = nd::rgb(0x89B4FA);
  nd::Color green = nd::rgb(0xA6E3A1);
  nd::Color red = nd::rgb(0xF38BA8);
  nd::Color peach = nd::rgb(0xFAB387);
  nd::Color mauve = nd::rgb(0xCBA6F7);
  nd::Color yellow = nd::rgb(0xF9E2AF);
  nd::Color teal = nd::rgb(0x94E2D5);
  nd::Color surface0 = nd::rgb(0x313244);
  nd::Color surface1 = nd::rgb(0x45475A);
};
const Palette C;

// 分区标题
nd::Widget sectionHeader(const char *title, nd::Color accent) {
  return nd::surface({.bg_color = nd::withAlpha(accent, 0x26),
                      .corner_radius = 4,
                      .padding = nd::Insets::symmetric(12, 6)})
      .child(nd::label(title, {.color = accent, .font_size = 15}));
}

// ── 页面构建函数 ───────────────────────────────────────────────────────────

nd::Widget buildOverview() {
  auto col = nd::column({.gap = 20});
  col.child(nd::label("NandinaUI", {.color = C.text, .font_size = 30}))
      .child(nd::label("C++ 前端 · 链式声明式 · 复刻 authoring 体验",
                       {.color = C.blue, .font_size = 14}))
      .child(sectionHeader("分层架构", C.mauve))
      .child(nd::card(
          "依赖方向",
          "foundation → reactive / render / layout / theme / text → "
          "runtime → widgets → app",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}))
      .child(sectionHeader("Core + Bindings", C.green))
      .child(nd::card(
          "C++ 前端", "仅依赖 nandina_abi.h，链接 libnandina_abi.a",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}))
      .child(nd::card(
          "Zig 前端", "直通 Core，零开销；两端共享同一运行时与渲染后端",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}));
  return col;
}

nd::Widget buildWidgets() {
  auto page = nd::column({.gap = 20});
  page.child(nd::label("组件库", {.color = C.text, .font_size = 24}))
      .child(nd::label("Primitives · Controls",
                       {.color = C.blue, .font_size = 13}))
      .child(sectionHeader("Primitives", C.mauve));

  // Primitives 行（真正的 Row 横排）
  {
    auto r = nd::row({.gap = 10});
    r.child(
        nd::surface({.bg_color = nd::withAlpha(C.blue, 0x33),
                     .corner_radius = 6,
                     .padding = nd::Insets::all(12),
                     .border_color = nd::withAlpha(C.blue, 0x66),
                     .border_width = 1})
            .child(nd::label("Surface", {.color = C.text, .font_size = 12})));

    struct IconItem {
      const char *name;
      nd::Color color;
      nd::IconShape shape;
    };
    const std::array<IconItem, 3> icons = {{
        {"  ● 指示点", C.green, nd::IconShape::Circle},
        {"  ■ 标记", C.blue, nd::IconShape::Rect},
        {"  ● 警告", C.peach, nd::IconShape::Circle},
    }};
    for (const auto &ic : icons) {
      r.child(nd::surface({.bg_color = C.surface0,
                           .corner_radius = 4,
                           .padding = nd::Insets::all(8)})
                  .child(nd::label(ic.name, {.color = C.text, .font_size = 12}))
                  .child(nd::icon(
                      {.color = ic.color, .size = 10, .shape = ic.shape})));
    }
    page.child(r);
  }

  // Controls 行
  page.child(sectionHeader("Controls", C.teal));
  {
    auto r = nd::row({.gap = 10});
    r.child(nd::surface({.bg_color = C.surface0,
                         .corner_radius = 6,
                         .padding = nd::Insets::all(10)})
                .child(nd::label("★ Label 文本标签",
                                 {.color = C.text, .font_size = 14})));
    r.child(nd::surface({.bg_color = C.surface0,
                         .corner_radius = 6,
                         .padding = nd::Insets::all(10)})
                .child(nd::button("  Button 按钮  ",
                                  {.bg_color = C.blue,
                                   .text_color = C.base,
                                   .font_size = 13,
                                   .corner_radius = 5,
                                   .padding = nd::Insets::symmetric(16, 8)})));
    r.child(nd::panel({.bg_color = C.mantle,
                       .corner_radius = 6,
                       .padding = nd::Insets::all(12),
                       .border_color = C.surface1,
                       .border_width = 1})
                .child(nd::label("Panel  面板容器",
                                 {.color = C.text, .font_size = 13})));
    r.child(nd::card(
        "Card 卡片", "标题 + 描述的结构化容器",
        {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}));
    page.child(r);
  }
  return page;
}

nd::Widget buildLayout() {
  auto col = nd::column({.gap = 20});
  col.child(nd::label("布局系统", {.color = C.text, .font_size = 24}))
      .child(nd::label("Flex / Flow / Anchors 三套纯函数求解器",
                       {.color = C.blue, .font_size = 13}))
      .child(sectionHeader("Row 水平布局", C.teal));

  {
    auto demo = nd::surface({.bg_color = C.mantle,
                             .corner_radius = 8,
                             .padding = nd::Insets::all(16)});
    auto r = nd::row({.gap = 8});
    struct Item {
      const char *label;
      nd::Color color;
    };
    const std::array<Item, 3> items = {{
        {"Item 1", C.blue},
        {"Item 2", C.green},
        {"Item 3", C.peach},
    }};
    for (const auto &it : items) {
      r.child(
          nd::surface({.bg_color = nd::withAlpha(it.color, 0x4D),
                       .corner_radius = 5,
                       .padding = nd::Insets::all(10)})
              .child(nd::label(it.label, {.color = C.text, .font_size = 13})));
    }
    demo.child(r);
    col.child(demo);
  }

  col.child(sectionHeader("求解器特性", C.yellow))
      .child(nd::card(
          "Flex", "Column / Row / Stack · gap · align · flex · shrink",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}))
      .child(nd::card(
          "Anchors", "QML 风格锚点 · fill / centerIn / margins",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}));
  return col;
}

nd::Widget buildReactive() {
  auto col = nd::column({.gap = 20});
  col.child(nd::label("响应式核心", {.color = C.text, .font_size = 24}))
      .child(nd::label("Signal → Computed → Effect",
                       {.color = C.blue, .font_size = 13}))
      .child(nd::card(
          "Signal 信号", "可写状态容器 · get / set / asReadonly",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}))
      .child(nd::card(
          "Computed 派生", "惰性求值 · 自动依赖追踪",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}))
      .child(nd::card(
          "Effect 副作用", "自动重执行 · batch 批量更新",
          {.bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15}));
  return col;
}

nd::Widget buildTheme() {
  auto col = nd::column({.gap = 20});
  col.child(nd::label("主题系统", {.color = C.text, .font_size = 24}))
      .child(nd::label("Design Tokens · Palette · Resolver",
                       {.color = C.blue, .font_size = 13}))
      .child(sectionHeader("调色板", C.mauve));

  auto swatch = nd::surface({.bg_color = C.mantle,
                             .corner_radius = 8,
                             .padding = nd::Insets::all(16)});
  auto sw_row = nd::row({.gap = 8});
  const std::array<nd::Color, 7> swatches = {C.red,  C.peach, C.yellow, C.green,
                                             C.teal, C.blue,  C.mauve};
  for (auto sw : swatches) {
    sw_row.child(nd::surface({.bg_color = sw,
                              .corner_radius = 6,
                              .padding = nd::Insets::symmetric(16, 10)}));
  }
  swatch.child(sw_row);
  col.child(swatch);
  return col;
}

// 全局导航器指针（按钮回调用）。
nd::PageHost *g_host = nullptr;

} // namespace

int main() {
  std::printf("NandinaUI C++ 前端 showcase\n");

  // 声明顺序很重要：C++ 按声明逆序析构。
  // 反应式 Signal/Effect 的后备图是 graph；widget 节点（含其 EffectScope 与内部
  // Signal）由 app 的 Tree 在析构时统一拆解，而拆解过程会回访 graph（dispose effect、
  // detachSource）。因此 graph 必须比 app 活得更久 —— 即 graph 先声明、后析构，
  // app 后声明、先析构。否则关闭窗口时会对已释放的 graph 产生 use-after-free 崩溃。
  nd::Graph graph;

  nd::App app("NandinaUI Showcase (C++)", 800, 600);

  std::vector<nd::PageHost::Builder> builders = {
      buildOverview, buildWidgets, buildLayout, buildReactive, buildTheme,
  };
  nd::PageHost host(std::move(builders), 0);
  g_host = &host;

  // 根：surface + 主列（顶栏导航 + 内容区）
  auto root = nd::surface({.bg_color = C.base});
  auto main_col = nd::column({.gap = 0});

  // 顶栏
  auto header = nd::surface(
      {.bg_color = C.mantle, .padding = nd::Insets::symmetric(16, 12)});
  auto nav_col = nd::column({.gap = 8});
  nav_col.child(nd::label("NandinaUI Showcase (C++)",
                          {.color = C.text, .font_size = 20}));

  auto nav_row = nd::row({.gap = 8});
  const std::array<const char *, 5> names = {"概述", "Widgets", "Layout",
                                             "Reactive", "Theme"};
  for (std::size_t i = 0; i < names.size(); ++i) {
    nav_row.child(nd::button(names[i], {.bg_color = C.surface0,
                                        .bg_hover_color = C.blue,
                                        .bg_pressed_color = C.teal,
                                        .text_color = C.text,
                                        .font_size = 13,
                                        .corner_radius = 4,
                                        .padding = nd::Insets::symmetric(12, 6),
                                        .on_click = [i]() {
                                          if (g_host)
                                            g_host->navigateTo(i);
                                        }}));
  }
  nav_col.child(nav_row);
  header.child(nav_col);
  main_col.child(header);

  // 内容区
  auto content =
      nd::surface({.bg_color = C.base, .padding = nd::Insets::all(24)});
  content.child(host.node());
  main_col.child(content);

  root.child(main_col);

  app.run(root);
  return 0;
}
