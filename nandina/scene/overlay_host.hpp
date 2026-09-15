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
    struct OverlayOptions {
        int order = 0;
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
        [[nodiscard]] auto as_overlay_host() -> OverlayHost* override { return this; }
        [[nodiscard]] auto as_overlay_host() const -> const OverlayHost* override { return this; }

        [[nodiscard]] auto present(
            std::shared_ptr<NanControl> overlay,
            OverlayOptions options = {}
        ) -> OverlayHandle;

        [[nodiscard]] auto overlay_count() const -> std::size_t;
        [[nodiscard]] auto contains(std::uint64_t id) const -> bool;

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
