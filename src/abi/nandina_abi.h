// nandina_abi.h — NandinaUI C ABI 公共接口
//
// 所有语言绑定（C++ / Python / Lua 等）都基于此头文件。
// 遵循 Core + Bindings 架构：绑定层在 C ABI 之上构建，不直接依赖 Zig Core。
//
// 约定：
//   - 不透明句柄为 `nandina_*_t` 类型，指向 Zig 堆分配对象。
//   - 所有函数返回 `nandina_error_t`：0 表示成功，非 0 表示错误代码。
//   - 输出参数通过指针传递（返回值或错误码）。
//   - 字符串以 UTF-8 零结尾传入；调用方负责其生命周期。
//   - 回调以函数指针 + `void* user_data` 传入。

#ifndef NANDINA_ABI_H
#define NANDINA_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═════════════════════════════════════════════════════════════════════════════
// 基础类型
// ═════════════════════════════════════════════════════════════════════════════

/// 错误码：0 表示成功。
typedef int32_t nandina_error_t;

/// 颜色（RRGGBBAA 32-bit，各通道 8-bit）。
typedef uint32_t nandina_color_t;

/// 矩形（浮点坐标）。
typedef struct {
  float x, y, w, h;
} nandina_rect_t;

/// 尺寸。
typedef struct {
  float width, height;
} nandina_size_t;

/// 内边距（四边统一为浮点值）。
typedef struct {
  float left, top, right, bottom;
} nandina_insets_t;

// ═════════════════════════════════════════════════════════════════════════════
// 不透明句柄类型
// ═════════════════════════════════════════════════════════════════════════════

typedef struct nandina_graph_t nandina_graph_t; // reactive.Graph
typedef struct nandina_tree_t nandina_tree_t;   // runtime.Tree
typedef struct nandina_node_t nandina_node_t;   // runtime.Node 基类
typedef struct nandina_signal_i32 nandina_signal_i32_t;
typedef struct nandina_signal_f32 nandina_signal_f32_t;
typedef struct nandina_signal_bool nandina_signal_bool_t;
typedef struct nandina_signal_color nandina_signal_color_t;
typedef struct nandina_signal_insets nandina_signal_insets_t;
typedef struct nandina_signal_string nandina_signal_string_t;
// TODO: Computed / Effect ABI 导出暂缓。nandina_effect_scope_t 句柄已预留，
// 但对应的 create / destroy / listen 等函数尚未实现。等基本框架稳定后补充。
typedef struct nandina_effect_scope_t nandina_effect_scope_t;
typedef struct nandina_scene_t nandina_scene_t; // render.Scene
typedef struct nandina_software_backend_t nandina_software_backend_t;
typedef struct nandina_text_field_t nandina_text_field_t; // widgets.TextField
typedef struct nandina_checkbox_t nandina_checkbox_t;     // widgets.Checkbox
typedef struct nandina_switch_t nandina_switch_t;         // widgets.Switch
typedef struct nandina_page_host_t nandina_page_host_t;   // app.PageHost
typedef struct nandina_app_t nandina_app_t;               // 全包窗口应用

// ═════════════════════════════════════════════════════════════════════════════
// 生命周期
// ═════════════════════════════════════════════════════════════════════════════

/// 获取库版本信息。返回的字符串在库生命周期内有效。
const char *nandina_version(void);

/// 初始化库（当前为 no-op，保留扩展用）。
/// 应在任何其他 nandina_* 函数之前调用。
nandina_error_t nandina_init(void);

/// 反初始化库（当前为 no-op）。
void nandina_deinit(void);

// ═════════════════════════════════════════════════════════════════════════════
// Graph — 响应式调度图
// ═════════════════════════════════════════════════════════════════════════════

/// 创建一个响应式 Graph。返回的句柄须用 `nandina_graph_destroy` 释放。
nandina_error_t nandina_graph_create(nandina_graph_t **out);

/// 销毁 Graph。必须在所有依附其上的 Signal/Tree 等销毁之后调用。
void nandina_graph_destroy(nandina_graph_t *g);

/// 执行批处理：在 `fn` 中对 Signal 的多次 set 合并为一次 flush。
/// `fn` 原型：`void fn(void* user_data)`。
nandina_error_t nandina_graph_batch(nandina_graph_t *g, void (*fn)(void *),
                                    void *user_data);

// ═════════════════════════════════════════════════════════════════════════════
// Signal — 可写响应式状态（int32 / float / bool / color / insets / string）
// ═════════════════════════════════════════════════════════════════════════════

nandina_error_t nandina_signal_i32_create(nandina_graph_t *g, int32_t initial,
                                          nandina_signal_i32_t **out);
int32_t nandina_signal_i32_get(nandina_signal_i32_t *s);
void nandina_signal_i32_set(nandina_signal_i32_t *s, int32_t value);
void nandina_signal_i32_destroy(nandina_signal_i32_t *s);

nandina_error_t nandina_signal_f32_create(nandina_graph_t *g, float initial,
                                          nandina_signal_f32_t **out);
float nandina_signal_f32_get(nandina_signal_f32_t *s);
void nandina_signal_f32_set(nandina_signal_f32_t *s, float value);
void nandina_signal_f32_destroy(nandina_signal_f32_t *s);

nandina_error_t nandina_signal_bool_create(nandina_graph_t *g, bool initial,
                                           nandina_signal_bool_t **out);
bool nandina_signal_bool_get(nandina_signal_bool_t *s);
void nandina_signal_bool_set(nandina_signal_bool_t *s, bool value);
void nandina_signal_bool_destroy(nandina_signal_bool_t *s);

nandina_error_t nandina_signal_color_create(nandina_graph_t *g,
                                            nandina_color_t initial,
                                            nandina_signal_color_t **out);
nandina_color_t nandina_signal_color_get(nandina_signal_color_t *s);
void nandina_signal_color_set(nandina_signal_color_t *s, nandina_color_t value);
void nandina_signal_color_destroy(nandina_signal_color_t *s);

nandina_error_t nandina_signal_insets_create(nandina_graph_t *g,
                                             nandina_insets_t initial,
                                             nandina_signal_insets_t **out);
nandina_insets_t nandina_signal_insets_get(nandina_signal_insets_t *s);
void nandina_signal_insets_set(nandina_signal_insets_t *s,
                               nandina_insets_t value);
void nandina_signal_insets_destroy(nandina_signal_insets_t *s);

// 字符串 Signal：调用方负责 set 时传入的字符串在调用期间有效；
// get 返回 Signal 内部持有的切片指针，在 Signal 下次 set 或 destroy 前有效。
nandina_error_t nandina_signal_string_create(nandina_graph_t *g,
                                             const char *initial,
                                             nandina_signal_string_t **out);
const char *nandina_signal_string_get(nandina_signal_string_t *s);
void nandina_signal_string_set(nandina_signal_string_t *s, const char *value);
void nandina_signal_string_destroy(nandina_signal_string_t *s);

// ═════════════════════════════════════════════════════════════════════════════
// Tree — UI 树容器
// ═════════════════════════════════════════════════════════════════════════════

/// 创建一个 Tree。Tree 拥有根节点，驱动一帧的布局与绘制。
/// `allocator`：Zig 风格的分配器回调（见 nandina_allocator_t）。若传 NULL，
/// 使用内部默认分配器。
nandina_error_t nandina_tree_create(nandina_tree_t **out);

/// 销毁 Tree（递归释放所有节点）。
void nandina_tree_destroy(nandina_tree_t *tree);

/// 挂载根节点。若已有根，旧根被释放。
nandina_error_t nandina_tree_set_root(nandina_tree_t *tree,
                                      nandina_node_t *root);

/// 设置视口尺寸（像素）。变化时触发重新布局。
void nandina_tree_set_viewport(nandina_tree_t *tree, float width, float height);

/// 执行一帧：若布局脏则重新布局，若绘制脏则重建场景。
/// 返回是否有新帧产生。
bool nandina_tree_frame(nandina_tree_t *tree);

// ═════════════════════════════════════════════════════════════════════════════
// Node — UI 节点
// ═════════════════════════════════════════════════════════════════════════════

/// 追加一个子节点（后添加的在上层）。
nandina_error_t nandina_node_add_child(nandina_node_t *parent,
                                       nandina_node_t *child);

/// 标记节点需要重新布局。
void nandina_node_mark_layout_dirty(nandina_node_t *node);

/// 标记节点需要重新绘制。
void nandina_node_mark_paint_dirty(nandina_node_t *node);

/// 设置节点可见性。
void nandina_node_set_visible(nandina_node_t *node, bool visible);

// ═════════════════════════════════════════════════════════════════════════════
// Widget 工厂函数
// ═════════════════════════════════════════════════════════════════════════════

/// Surface —— 基础背景容器。
nandina_error_t
nandina_surface_create(nandina_graph_t *g, nandina_color_t bg_color,
                       float corner_radius, nandina_insets_t padding,
                       nandina_color_t border_color, float border_width,
                       nandina_node_t **out);

/// Surface 响应式版本——所有属性通过 Signal 传入（可传 NULL 表示使用默认值）。
nandina_error_t nandina_surface_create_reactive(
    nandina_graph_t *g, nandina_signal_color_t *bg_color,
    nandina_signal_f32_t *corner_radius, nandina_signal_insets_t *padding,
    nandina_signal_color_t *border_color, nandina_signal_f32_t *border_width,
    nandina_node_t **out);

/// Label —— 响应式文本标签。
nandina_error_t nandina_label_create(nandina_graph_t *g, const char *text,
                                     nandina_color_t color, float font_size,
                                     nandina_node_t **out);

/// Label 响应式版本。
nandina_error_t nandina_label_create_reactive(nandina_graph_t *g,
                                              nandina_signal_string_t *text,
                                              nandina_signal_color_t *color,
                                              nandina_signal_f32_t *font_size,
                                              nandina_node_t **out);

/// Button —— 按钮组件。
nandina_error_t nandina_button_create(
    nandina_graph_t *g, const char *label, nandina_color_t bg_color,
    nandina_color_t bg_hover_color, nandina_color_t bg_pressed_color,
    nandina_color_t text_color, float font_size, float corner_radius,
    nandina_insets_t padding, nandina_node_t **out);

/// Column —— 垂直堆叠容器。
nandina_error_t nandina_column_create(float gap, nandina_node_t **out);

/// Card —— 带标题/描述的容器。
nandina_error_t nandina_card_create(nandina_graph_t *g, const char *title,
                                    const char *description,
                                    nandina_color_t bg_color,
                                    float corner_radius, float title_font_size,
                                    float desc_font_size, nandina_node_t **out);

/// Panel —— 带边框/圆角/padding 的内容面板。
nandina_error_t nandina_panel_create(nandina_graph_t *g,
                                     nandina_color_t bg_color,
                                     float corner_radius,
                                     nandina_insets_t padding,
                                     nandina_color_t border_color,
                                     float border_width, nandina_node_t **out);

/// Row —— 水平排列容器。
nandina_error_t nandina_row_create(float gap, nandina_node_t **out);

/// Stack —— 子节点层叠容器。
nandina_error_t nandina_stack_create(nandina_node_t **out);

/// Icon —— 图标 primitive。shape：0 = rect，1 = circle。
nandina_error_t nandina_icon_create(nandina_graph_t *g, nandina_color_t color,
                                    float size, int32_t shape,
                                    nandina_node_t **out);

/// Field —— 语义表单容器（label + helper + 控件挂入）。
nandina_error_t nandina_field_create(nandina_graph_t *g, const char *label,
                                     const char *helper, bool required,
                                     nandina_node_t **out);

// ═════════════════════════════════════════════════════════════════════════════
// 事件回调
// ═════════════════════════════════════════════════════════════════════════════

/// 设置 Button 的点击回调。`fn` 原型：`void fn(void* user_data)`。
/// user_data 所有权归调用方；core 仅持有指针，须保证其存活至节点销毁。
void nandina_button_set_on_click(nandina_node_t *button,
                                 void (*fn)(void *user_data), void *user_data);

// ═════════════════════════════════════════════════════════════════════════════
// TextField — 文本输入控件
// ═════════════════════════════════════════════════════════════════════════════

/// 创建 TextField。返回独立句柄；用 `nandina_text_field_node` 取其节点挂入树。
nandina_error_t
nandina_text_field_create(nandina_graph_t *g, const char *placeholder,
                          float font_size, nandina_color_t color,
                          nandina_color_t bg_color, float min_width,
                          nandina_text_field_t **out);

/// 取 TextField 的节点指针（用于挂入树 / 添加子节点）。
nandina_node_t *nandina_text_field_node(nandina_text_field_t *tf);

/// 获取当前文本（返回线程本地缓冲，下次调用前有效）。
const char *nandina_text_field_text(nandina_text_field_t *tf);

/// 设置文本内容。
void nandina_text_field_set_text(nandina_text_field_t *tf, const char *text);

// ═════════════════════════════════════════════════════════════════════════════
// Checkbox — 复选框
// ═════════════════════════════════════════════════════════════════════════════

nandina_error_t nandina_checkbox_create(nandina_graph_t *g,
                                        nandina_signal_bool_t *checked,
                                        nandina_color_t color,
                                        nandina_checkbox_t **out);
nandina_node_t *nandina_checkbox_node(nandina_checkbox_t *cb);
/// 勾选变化回调。`fn` 原型：`void fn(void* user_data, bool checked)`。
void nandina_checkbox_set_on_change(nandina_checkbox_t *cb,
                                    void (*fn)(void *user_data, bool checked),
                                    void *user_data);

// ═════════════════════════════════════════════════════════════════════════════
// Switch — 开关
// ═════════════════════════════════════════════════════════════════════════════

nandina_error_t nandina_switch_create(nandina_graph_t *g,
                                      nandina_signal_bool_t *checked,
                                      nandina_color_t color,
                                      nandina_switch_t **out);
nandina_node_t *nandina_switch_node(nandina_switch_t *sw);
/// 开关变化回调。`fn` 原型：`void fn(void* user_data, bool checked)`。
void nandina_switch_set_on_change(nandina_switch_t *sw,
                                  void (*fn)(void *user_data, bool checked),
                                  void *user_data);

// ═════════════════════════════════════════════════════════════════════════════
// PageHost — 多页导航容器
// ═════════════════════════════════════════════════════════════════════════════

/// 页面构建回调：返回该页面的根节点。原型 `nandina_node_t* build(void*
/// user_data)`。
typedef nandina_node_t *(*nandina_page_build_fn)(void *user_data);

/// 创建 PageHost。`builds`/`user_datas` 为长度 `count` 的并列数组，
/// 第 i 个页面用 builds[i](user_datas[i]) 构建。`initial_index` 为初始页面。
nandina_error_t nandina_page_host_create(nandina_graph_t *g,
                                         const nandina_page_build_fn *builds,
                                         void *const *user_datas, size_t count,
                                         size_t initial_index,
                                         nandina_page_host_t **out);

/// 取 PageHost 节点指针（挂入树）。
nandina_node_t *nandina_page_host_node(nandina_page_host_t *host);

/// 导航到指定索引页面（重建该页面子树）。
nandina_error_t nandina_page_host_navigate_to(nandina_page_host_t *host,
                                              size_t index);

// ═════════════════════════════════════════════════════════════════════════════
// App — 全包窗口入口（默认 SDL3 + 软件渲染后端）
// ═════════════════════════════════════════════════════════════════════════════
//
// 「一行起窗口」的便利层：core 内部开窗口、跑主循环、渲染。
// 适合快速自用；高级用户可改用 Tree + SoftwareBackend 自管窗口。

/// 创建应用窗口（标题 + 初始宽高）。失败返回非 0。
nandina_error_t nandina_app_create(const char *title, int32_t width,
                                   int32_t height, nandina_app_t **out);

/// 挂载根节点（内部建 Tree 持有）。
nandina_error_t nandina_app_set_root(nandina_app_t *app, nandina_node_t *root);

/// 进入阻塞主循环，直到窗口关闭。
nandina_error_t nandina_app_run(nandina_app_t *app);

/// 销毁应用（关闭窗口，释放 Tree 与字体资源）。
void nandina_app_destroy(nandina_app_t *app);

// ═════════════════════════════════════════════════════════════════════════════
// Render — 场景与后端
// ═════════════════════════════════════════════════════════════════════════════

/// 创建软件渲染后端。
nandina_error_t
nandina_software_backend_create(nandina_software_backend_t **out);

/// 销毁软件渲染后端。
void nandina_software_backend_destroy(nandina_software_backend_t *backend);

/// 获取后端的像素缓冲（ARGB8888）。
/// 返回的指针在下次 endFrame 或 destroy 前有效。
nandina_error_t
nandina_software_backend_pixels(nandina_software_backend_t *backend,
                                void **out_data, int32_t *out_width,
                                int32_t *out_height);

// ═════════════════════════════════════════════════════════════════════════════
// 错误处理
// ═════════════════════════════════════════════════════════════════════════════

/// 返回最后一次错误的描述字符串（线程本地缓冲）。
const char *nandina_error_message(nandina_error_t err);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NANDINA_ABI_H
