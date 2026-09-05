//
// Created by cvrain on 2026/7/4.
//
// app/nan_window — 窗口与主循环封装。
//
// NanWindow 吸收掉此前 example 里手写的全部底层样板:
//   - raylib 窗口生命周期 (InitWindow / CloseWindow / WindowShouldClose);
//   - 渲染设备构造 + 每帧 begin/clear/draw/end;
//   - 输入轮询 → 翻译成 scene::InputEvent → 派发给 SceneTree;
//   - per-frame process(dt)。
//
// 开发者只需:
//   1. 构造 NanWindow (或继承它);
//   2. set_content(root_control) 挂载页面根组件;
//   3. app.run(window)。
//
// 可继承覆写:
//   - on_setup():  窗口就绪后调用一次, 用于注册页面 / 搭建全局框架 (未来接 Router)。
//   - on_frame(dt): 每帧 process 之后、绘制之前调用, 用于动画驱动等。
//
// raylib 类型不出现在本头文件; 全部收敛在 nan_window.cpp。
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_WINDOW_HPP
#define NANDINA_EXPERIMENT_APP_NAN_WINDOW_HPP

#include "../render/render_device.hpp"
#include "../render/texture_cache.hpp"
#include "../scene/scene_tree.hpp"
#include "../text/font_pipeline.hpp"
#include "nan_router.hpp"
#include "viewport_scaling.hpp"
#include "window_config.hpp"
#include "window_metrics.hpp"

#include <memory>
#include <optional>

namespace nandina::reactive
{
    class Graph;
}

namespace nandina::app
{

    class NanApplication;

    class NanWindow {
    public:
        NanWindow(NanApplication& app, WindowConfig config);
        virtual ~NanWindow();

        NanWindow(const NanWindow&) = delete;
        auto operator=(const NanWindow&) -> NanWindow& = delete;
        NanWindow(NanWindow&&) = delete;
        auto operator=(NanWindow&&) -> NanWindow& = delete;

        /// 挂载页面根组件 (成为 SceneTree 的 root)。
        void set_content(std::shared_ptr<scene::NanNode2D> root);

        /// 创建一个 keep-alive Router, 并把 Router host 挂为窗口内容。
        [[nodiscard]] auto use_router() -> NanRouter&;

        [[nodiscard]] auto router() -> NanRouter* {
            return router_.get();
        }

        /// 访问响应式图 (来自 App)。
        [[nodiscard]] auto graph() -> reactive::Graph&;

        [[nodiscard]] auto theme() const -> const theme::NanTheme&;

        /// 访问场景树 (高级用途; 一般通过 set_content 即可)。
        [[nodiscard]] auto scene_tree() -> scene::NanSceneTree& {
            return tree_;
        }

        [[nodiscard]] auto config() const -> const WindowConfig& {
            return config_;
        }

        /// Access the active render device for advanced window-owned resources.
        /// Returns null before open() and after close().
        [[nodiscard]] auto render_device() -> render::IRenderDevice* {
            return device_.get();
        }
        [[nodiscard]] auto default_text_pipeline() const -> const widget::primitives::TextPipeline*;

        // ── 由 NanApplication::run 驱动 ─────────────────────────────────────────────

        /// 打开 raylib 窗口 + 创建渲染设备。run 开始时调用一次。
        void open();

        /// 是否收到关闭请求 (点 X / Alt-F4)。
        [[nodiscard]] auto should_close() const -> bool;

        /// 执行一帧: 轮询输入 → 派发 → process(dt) → on_frame → 绘制。
        void tick();

        /// 关闭 raylib 窗口。run 结束时调用。
        void close();

    protected:
        /// 窗口就绪 (open 之后) 调用一次。子类覆写以注册页面 / 搭建全局框架。
        virtual void on_setup() {}

        /// 每帧 process(dt) 之后、绘制之前调用。子类覆写以驱动动画等。
        virtual void on_frame(float /*dt*/) {}

        /// Called during close() before the render device and native window are released.
        virtual void on_teardown() {}

    private:
        void poll_and_dispatch_input();
        void update_viewport_mapping(foundation::NanSize screen_size);

        NanApplication& app_;
        WindowConfig config_;
        scene::NanSceneTree tree_;
        std::unique_ptr<NanRouter> router_;
        std::unique_ptr<render::IRenderDevice> device_;
        std::unique_ptr<render::TextureCache> texture_cache_;
        std::unique_ptr<text::FontPipelineCache> font_pipeline_cache_;
        std::shared_ptr<text::FontPipeline> default_font_pipeline_;
        std::optional<widget::primitives::TextPipeline> default_text_pipeline_;
        bool opened_ = false;
        std::optional<ViewportMapping> viewport_mapping_;

        // 上一帧鼠标位置 (用于计算 delta 与 move 事件)。
        float last_mouse_x_ = 0.0F;
        float last_mouse_y_ = 0.0F;
        bool has_mouse_ = false;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_WINDOW_HPP
