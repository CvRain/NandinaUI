module;

#include <cstdint>

export module nandina.render.backend;

import nandina.render.scene;

export namespace nandina::render {

    enum class RenderTargetFormat : std::uint8_t {
        argb8888,
    };

    struct RenderTargetView {
        std::uint32_t* pixels{nullptr};
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::uint32_t stride{0};
        RenderTargetFormat format{RenderTargetFormat::argb8888};
    };

    class RenderBackend {
    public:
        virtual ~RenderBackend() = default;

        virtual auto begin_frame(const RenderTargetView& target) -> bool = 0;

        virtual auto submit(const Scene& scene) -> bool = 0;

        virtual auto present() -> bool = 0;

        [[nodiscard]] virtual auto ready() const noexcept -> bool = 0;

        [[nodiscard]] virtual auto current_target() const noexcept -> RenderTargetView = 0;
    };
}
