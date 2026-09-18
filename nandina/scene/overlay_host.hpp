#ifndef NANDINA_EXPERIMENT_SCENE_OVERLAY_HOST_HPP
#define NANDINA_EXPERIMENT_SCENE_OVERLAY_HOST_HPP

#include "canvas_layer.hpp"
#include "control.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace nandina::scene
{
    /// 浮层的语义层级。组件按用途声明层级，不直接写 CanvasLayer 的 order 数值。
    enum class OverlayLevel: int {
        /// 提示、下拉与菜单：位于应用内容之上，彼此按加入顺序叠放。
        popup = 0,
        /// 模态内容：高于所有 popup，并阻断其下全部输入。
        modal = 100,
        /// 模态内容内部再展开的提示与下拉：必须高于模态，否则会被遮罩埋掉且点不到。
        nested_popup = 200,
    };

    struct OverlayOptions {
        OverlayLevel level = OverlayLevel::popup;
        bool block_below = false;
    };

    class OverlayHost;

    class OverlayHandle final {
    public:
        OverlayHandle() = default;
        ~OverlayHandle();

        OverlayHandle(const OverlayHandle&) = delete;
        auto operator=(const OverlayHandle&) -> OverlayHandle& = delete;
        OverlayHandle(OverlayHandle&& other) noexcept;
        auto operator=(OverlayHandle&& other) noexcept -> OverlayHandle&;

        [[nodiscard]] auto mounted() const -> bool;
        void close();

    private:
        friend class OverlayHost;
        OverlayHandle(std::weak_ptr<OverlayHost> host, std::uint64_t id);

        std::weak_ptr<OverlayHost> host_;
        std::uint64_t id_ = 0;
    };

    class OverlayHost final: public LayerStack {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<OverlayHost>;

        auto set_content(std::shared_ptr<NanControl> content) -> NanControl&;
        [[nodiscard]] auto content() const -> NanControl*;

        /**
         * 释放内容层持有的应用内容（没有内容时是 no-op）。
         *
         * 窗口关闭时必须先调用它：内容层是窗口成员，若不在这里主动释放，内容树会
         * 一直活到窗口析构，而那时 render device 已销毁 —— 控件里的文本资源
         * （FontPipeline → GlyphAtlasTexture）会在析构时对已销毁的 device 调用
         * destroy_texture()，导致关闭后 SIGSEGV。
         *
         * 浮层内容（present 出来的）不在这里清理，由各自的 OverlayHandle 负责。
         */
        void clear_content();

        /**
         * 关闭并释放所有当前浮层。
         *
         * 窗口销毁前必须调用它，使浮层中的字体、纹理等 GPU 持有者在
         * render device 仍然存活时析构。外部持有的 OverlayHandle 会变成未挂载状态。
         */
        void clear_overlays();
        [[nodiscard]] auto as_overlay_host() -> OverlayHost* override { return this; }
        [[nodiscard]] auto as_overlay_host() const -> const OverlayHost* override { return this; }

        [[nodiscard]] auto present(
            std::shared_ptr<NanControl> overlay,
            OverlayOptions options = {}
        ) -> OverlayHandle;

        [[nodiscard]] auto overlay_count() const -> std::size_t;
        [[nodiscard]] auto contains(std::uint64_t id) const -> bool;

        /// 该节点是否位于本 host 的浮层内容之下。浮层内部再展开的提示与下拉据此选择
        /// `OverlayLevel::nested_popup`，以免被模态遮罩盖住。
        [[nodiscard]] auto hosts_node(const NanNode& node) const -> bool;

        /// Viewport the screen-space layers were last laid out against. Zero before
        /// the first layout pass. Floating components use this to keep anchored
        /// content inside the window instead of the trigger's own bounds.
        [[nodiscard]] auto viewport_size() const -> foundation::NanSize;
        /// Safe non-owning reference for services whose lifetime may be shorter than
        /// a detached component. Expires instead of leaving a dangling raw pointer.
        [[nodiscard]] auto weak_self() const noexcept -> std::weak_ptr<OverlayHost>;

    private:
        struct Entry {
            std::uint64_t id = 0;
            std::weak_ptr<NanControl> control;
            bool block_below = false;
        };

        OverlayHost() = default;
        void initialize();
        auto close(std::uint64_t id) -> bool;
        void update_input_mode();

        friend class OverlayHandle;

        std::shared_ptr<CanvasLayer> content_layer_;
        std::shared_ptr<CanvasLayer> overlay_layer_;
        std::shared_ptr<NanControl> overlay_surface_;
        std::vector<Entry> entries_;
        std::uint64_t next_id_ = 1;
        std::weak_ptr<OverlayHost> self_;
    };
}

#endif
