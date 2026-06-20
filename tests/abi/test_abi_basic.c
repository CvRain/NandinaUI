// test_abi_basic.c —— C ABI 基础功能测试
//
// 验证 NandinaUI C ABI 的核心功能：生命周期、Graph、Signal、Tree、Widget 工厂。
// 编译：gcc -I/workspace/Cpp/NandinaUI/zig-out/include \
//           -L/workspace/Cpp/NandinaUI/zig-out/lib \
//           test_abi_basic.c -lnandina_abi -lpthread -lm -o test_abi_basic
//
// 注意：需要链接 pthread 和 math 库，因为 Zig 运行时依赖它们。

#include "nandina_abi.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 生命周期 ─────────────────────────────────────────────────────────────────

static void test_lifecycle(void) {
  printf("  test_lifecycle ... ");
  const char *ver = nandina_version();
  assert(ver != NULL);
  assert(strlen(ver) > 0);
  printf("version=%s ... ", ver);

  nandina_error_t err = nandina_init();
  assert(err == 0);

  nandina_deinit();
  printf("OK\n");
}

// ── Graph ────────────────────────────────────────────────────────────────────

static void test_graph_signal_i32(void) {
  printf("  test_graph_signal_i32 ... ");
  nandina_graph_t *g = NULL;
  nandina_error_t err = nandina_graph_create(&g);
  assert(err == 0);
  assert(g != NULL);

  nandina_signal_i32_t *s = NULL;
  err = nandina_signal_i32_create(g, 42, &s);
  assert(err == 0);
  assert(s != NULL);

  int32_t val = nandina_signal_i32_get(s);
  assert(val == 42);

  nandina_signal_i32_set(s, 100);
  val = nandina_signal_i32_get(s);
  assert(val == 100);

  nandina_signal_i32_destroy(s);
  nandina_graph_destroy(g);
  printf("OK\n");
}

static void test_graph_signal_f32(void) {
  printf("  test_graph_signal_f32 ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_signal_f32_t *s = NULL;
  nandina_signal_f32_create(g, 3.14f, &s);
  assert(s != NULL);

  float val = nandina_signal_f32_get(s);
  assert(val > 3.13f && val < 3.15f);

  nandina_signal_f32_set(s, 2.71f);
  val = nandina_signal_f32_get(s);
  assert(val > 2.70f && val < 2.72f);

  nandina_signal_f32_destroy(s);
  nandina_graph_destroy(g);
  printf("OK\n");
}

static void test_graph_signal_bool(void) {
  printf("  test_graph_signal_bool ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_signal_bool_t *s = NULL;
  nandina_signal_bool_create(g, true, &s);
  assert(s != NULL);
  assert(nandina_signal_bool_get(s) == true);

  nandina_signal_bool_set(s, false);
  assert(nandina_signal_bool_get(s) == false);

  nandina_signal_bool_destroy(s);
  nandina_graph_destroy(g);
  printf("OK\n");
}

static void test_graph_signal_color(void) {
  printf("  test_graph_signal_color ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_signal_color_t *s = NULL;
  // 白色：0xFFFFFFFF (RRGGBBAA)
  nandina_signal_color_create(g, 0xFFFFFFFF, &s);
  assert(s != NULL);
  assert(nandina_signal_color_get(s) == 0xFFFFFFFF);

  nandina_signal_color_set(s, 0xFF0000FF); // 红色
  assert(nandina_signal_color_get(s) == 0xFF0000FF);

  nandina_signal_color_destroy(s);
  nandina_graph_destroy(g);
  printf("OK\n");
}

static void test_graph_signal_string(void) {
  printf("  test_graph_signal_string ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_signal_string_t *s = NULL;
  nandina_signal_string_create(g, "Hello", &s);
  assert(s != NULL);

  const char *val = nandina_signal_string_get(s);
  assert(strcmp(val, "Hello") == 0);

  nandina_signal_string_set(s, "World");
  val = nandina_signal_string_get(s);
  assert(strcmp(val, "World") == 0);

  nandina_signal_string_destroy(s);
  nandina_graph_destroy(g);
  printf("OK\n");
}

// ── Tree / Node ──────────────────────────────────────────────────────────────

static void test_tree_create(void) {
  printf("  test_tree_create ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_tree_t *tree = NULL;
  nandina_error_t err = nandina_tree_create(&tree);
  assert(err == 0);
  assert(tree != NULL);

  nandina_tree_set_viewport(tree, 800, 600);

  // 创建一个 Surface 作为根节点
  nandina_node_t *surface = NULL;
  nandina_insets_t pad = {10, 10, 10, 10};
  err = nandina_surface_create(g, 0xFF1E1E2E, // bg_color: 深色
                               8.0f,          // corner_radius
                               pad,           // padding
                               0xFF45475A,    // border_color
                               1.0f,          // border_width
                               &surface);
  assert(err == 0);
  assert(surface != NULL);

  nandina_tree_set_root(tree, surface);

  // 添加一个 Label 作为子节点
  nandina_node_t *label = NULL;
  err = nandina_label_create(g, "Hello from C ABI", 0xFFCDD6F4, 18.0f, &label);
  assert(err == 0);
  assert(label != NULL);

  nandina_node_add_child(surface, label);

  // 驱动一帧
  bool had_frame = nandina_tree_frame(tree);
  assert(had_frame == true);

  // 第二次调用应该返回 false（没有变化）
  had_frame = nandina_tree_frame(tree);
  assert(had_frame == false);

  // 标记脏并再次驱动
  nandina_node_mark_layout_dirty(label);
  had_frame = nandina_tree_frame(tree);
  assert(had_frame == true);

  nandina_tree_destroy(tree);
  nandina_graph_destroy(g);
  printf("OK\n");
}

// ── Widget 工厂 ──────────────────────────────────────────────────────────────

static void test_widget_factories(void) {
  printf("  test_widget_factories ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_node_t *node = NULL;
  nandina_insets_t pad = {0, 0, 0, 0};

  // Column
  nandina_error_t err = nandina_column_create(8.0f, &node);
  assert(err == 0);
  assert(node != NULL);

  // Button
  err = nandina_button_create(g, "Click me", 0xFF89B4FA, 0xFF74C7EC, 0xFF89DCEB,
                              0xFF1E1E2E, 14.0f, 6.0f, pad, &node);
  assert(err == 0);
  assert(node != NULL);

  // Card
  err = nandina_card_create(g, "Card Title", "Card Description", 0xFF313244,
                            8.0f, 18.0f, 13.0f, &node);
  assert(err == 0);
  assert(node != NULL);

  // Panel
  err = nandina_panel_create(g, 0xFF11111B, 6.0f, pad, 0xFF313244, 1.0f, &node);
  assert(err == 0);
  assert(node != NULL);

  // Row
  err = nandina_row_create(8.0f, &node);
  assert(err == 0);
  assert(node != NULL);

  // Stack
  err = nandina_stack_create(&node);
  assert(err == 0);
  assert(node != NULL);

  // Icon（circle）
  err = nandina_icon_create(g, 0xFFA6E3A1, 12.0f, 1, &node);
  assert(err == 0);
  assert(node != NULL);

  // Field
  err = nandina_field_create(g, "用户名", "请输入用户名", true, &node);
  assert(err == 0);
  assert(node != NULL);

  nandina_graph_destroy(g);
  printf("OK\n");
}

// ── 事件回调 / TextField / Checkbox / Switch ─────────────────────────────────

static int g_click_count = 0;
static void on_click_cb(void *user_data) {
  int *counter = (int *)user_data;
  (*counter)++;
}

static bool g_last_checked = false;
static void on_change_cb(void *user_data, bool checked) {
  (void)user_data;
  g_last_checked = checked;
}

static void test_events_and_inputs(void) {
  printf("  test_events_and_inputs ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  // Button + on_click
  nandina_node_t *btn = NULL;
  nandina_insets_t pad = {12, 8, 12, 8};
  nandina_error_t err =
      nandina_button_create(g, "OK", 0xFF89B4FA, 0xFF74C7EC, 0xFF89DCEB,
                            0xFF1E1E2E, 14.0f, 6.0f, pad, &btn);
  assert(err == 0);
  g_click_count = 0;
  nandina_button_set_on_click(btn, on_click_cb, &g_click_count);

  // TextField
  nandina_text_field_t *tf = NULL;
  err = nandina_text_field_create(g, "请输入...", 14.0f, 0xFFCDD6F4, 0xFF313244,
                                  150.0f, &tf);
  assert(err == 0);
  assert(tf != NULL);
  assert(nandina_text_field_node(tf) != NULL);
  nandina_text_field_set_text(tf, "Hello");
  assert(strcmp(nandina_text_field_text(tf), "Hello") == 0);

  // Checkbox + on_change
  nandina_signal_bool_t *checked = NULL;
  nandina_signal_bool_create(g, false, &checked);
  nandina_checkbox_t *cb = NULL;
  err = nandina_checkbox_create(g, checked, 0xFF89B4FA, &cb);
  assert(err == 0);
  assert(cb != NULL);
  assert(nandina_checkbox_node(cb) != NULL);
  nandina_checkbox_set_on_change(cb, on_change_cb, NULL);

  // Switch + on_change
  nandina_switch_t *sw = NULL;
  err = nandina_switch_create(g, checked, 0xFFA6E3A1, &sw);
  assert(err == 0);
  assert(sw != NULL);
  assert(nandina_switch_node(sw) != NULL);
  nandina_switch_set_on_change(sw, on_change_cb, NULL);

  // 清理：节点由各自的 deinitTree 释放，这里仅释放 graph 与持有的 signal。
  // 注意：widget 节点未挂入 tree，测试中不单独释放（进程退出回收）。
  nandina_signal_bool_destroy(checked);
  nandina_graph_destroy(g);
  printf("OK\n");
}

// ── PageHost — 多页导航 ──────────────────────────────────────────────────────

static nandina_graph_t *g_page_graph = NULL;
static int g_page0_built = 0;
static int g_page1_built = 0;

static nandina_node_t *build_page0(void *user_data) {
  (void)user_data;
  g_page0_built++;
  nandina_node_t *n = NULL;
  nandina_label_create(g_page_graph, "Page 0", 0xFFFFFFFF, 16.0f, &n);
  return n;
}

static nandina_node_t *build_page1(void *user_data) {
  (void)user_data;
  g_page1_built++;
  nandina_node_t *n = NULL;
  nandina_label_create(g_page_graph, "Page 1", 0xFFFFFFFF, 16.0f, &n);
  return n;
}

static void test_page_host(void) {
  printf("  test_page_host ... ");
  nandina_graph_create(&g_page_graph);
  g_page0_built = 0;
  g_page1_built = 0;

  nandina_page_build_fn builds[2] = {build_page0, build_page1};
  void *user_datas[2] = {NULL, NULL};

  nandina_page_host_t *host = NULL;
  nandina_error_t err =
      nandina_page_host_create(g_page_graph, builds, user_datas, 2, 0, &host);
  assert(err == 0);
  assert(host != NULL);
  assert(nandina_page_host_node(host) != NULL);
  assert(g_page0_built == 1); // 初始页面已构建
  assert(g_page1_built == 0);

  // 导航到 page 1
  err = nandina_page_host_navigate_to(host, 1);
  assert(err == 0);
  assert(g_page1_built == 1);

  // 越界导航应报错
  err = nandina_page_host_navigate_to(host, 5);
  assert(err != 0);

  nandina_graph_destroy(g_page_graph);
  g_page_graph = NULL;
  printf("OK\n");
}

// ── SoftwareBackend ──────────────────────────────────────────────────────────

static void test_software_backend(void) {
  printf("  test_software_backend ... ");
  nandina_software_backend_t *backend = NULL;
  nandina_error_t err = nandina_software_backend_create(&backend);
  assert(err == 0);
  assert(backend != NULL);

  void *pixels = NULL;
  int32_t w = 0, h = 0;
  err = nandina_software_backend_pixels(backend, &pixels, &w, &h);
  // 在没有 beginFrame 的情况下调用，应该返回错误
  assert(err != 0);

  nandina_software_backend_destroy(backend);
  printf("OK\n");
}

// ── Batch ────────────────────────────────────────────────────────────────────

struct batch_data {
  nandina_signal_i32_t *sig;
  int expected_final;
};

static void batch_callback(void *user_data) {
  struct batch_data *data = (struct batch_data *)user_data;
  nandina_signal_i32_set(data->sig, 10);
  nandina_signal_i32_set(data->sig, 20);
  nandina_signal_i32_set(data->sig, 30);
}

static void test_batch(void) {
  printf("  test_batch ... ");
  nandina_graph_t *g = NULL;
  nandina_graph_create(&g);

  nandina_signal_i32_t *s = NULL;
  nandina_signal_i32_create(g, 0, &s);

  struct batch_data data;
  data.sig = s;

  nandina_graph_batch(g, batch_callback, &data);
  int32_t val = nandina_signal_i32_get(s);
  assert(val == 30); // batch 内的多次 set 合并，最终值为 30

  nandina_signal_i32_destroy(s);
  nandina_graph_destroy(g);
  printf("OK\n");
}

// ── Error handling ───────────────────────────────────────────────────────────

static void test_error_message(void) {
  printf("  test_error_message ... ");
  const char *msg = nandina_error_message(0);
  assert(msg != NULL);
  printf("OK\n");
}

// ── 主测试入口 ───────────────────────────────────────────────────────────────

int main(void) {
  printf("NandinaUI C ABI 测试:\n");

  test_lifecycle();
  test_graph_signal_i32();
  test_graph_signal_f32();
  test_graph_signal_bool();
  test_graph_signal_color();
  test_graph_signal_string();
  test_tree_create();
  test_widget_factories();
  test_events_and_inputs();
  test_page_host();
  test_software_backend();

  test_batch();

  test_error_message();

  printf("全部通过!\n");
  return 0;
}
