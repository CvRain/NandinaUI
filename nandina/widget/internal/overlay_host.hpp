#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_OVERLAY_HOST_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_OVERLAY_HOST_HPP

#include "../../scene/canvas_layer.hpp"
#include "../../scene/control.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace nandina::widget::internal
{
    struct OverlayOptions {
        int order = 0;
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

    class OverlayHost final: public scene::LayerStack {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<OverlayHost>;

        auto set_content(std::shared_ptr<scene::NanControl> content) -> scene::NanControl&;
        [[nodiscard]] auto content() const -> scene::NanControl*;

        [[nodiscard]] auto present(
            std::shared_ptr<scene::NanControl> overlay,
            OverlayOptions options = {}
        ) -> OverlayHandle;

        [[nodiscard]] auto overlay_count() const -> std::size_t;
        [[nodiscard]] auto contains(std::uint64_t id) const -> bool;

    private:
        struct Entry {
            std::uint64_t id = 0;
            std::weak_ptr<scene::NanControl> control;
        };

        OverlayHost() = default;
        void initialize();
        auto close(std::uint64_t id) -> bool;

        friend class OverlayHandle;

        std::shared_ptr<scene::CanvasLayer> content_layer_;
        std::shared_ptr<scene::CanvasLayer> overlay_layer_;
        std::shared_ptr<scene::NanControl> overlay_surface_;
        std::vector<Entry> entries_;
        std::uint64_t next_id_ = 1;
    };
}

#endif
