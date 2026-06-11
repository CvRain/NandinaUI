module;

#include <algorithm>
#include <memory>

#include <thorvg-1/thorvg.h>

export module nandina.render.thorvg_backend;

import nandina.render.backend;
import nandina.render.scene;
import nandina.render.thorvg_adapter;

export namespace nandina::render {

    class ThorVGRenderBackend final : public RenderBackend {
    public:
        [[nodiscard]] auto canvas() const noexcept -> tvg::SwCanvas* {
            return canvas_.get();
        }

        auto begin_frame(const RenderTargetView& target) -> bool override {
            if (!target.pixels || target.width == 0 || target.height == 0 || target.stride < target.width) {
                target_ = {};
                canvas_.reset();
                return false;
            }

            target_ = target;
            canvas_.reset(tvg::SwCanvas::gen());
            if (!canvas_) {
                target_ = {};
                return false;
            }

            std::fill_n(target.pixels, static_cast<std::size_t>(target.stride) * target.height, 0u);

            const auto result = canvas_->target(
                target.pixels,
                target.stride,
                target.width,
                target.height,
                tvg::ColorSpace::ARGB8888);
            if (result != tvg::Result::Success) {
                canvas_.reset();
                target_ = {};
                return false;
            }
            return true;
        }

        auto submit(const Scene& scene) -> bool override {
            if (!ready()) {
                return false;
            }
            adapter_.submit(scene, *canvas_);
            return true;
        }

        auto present() -> bool override {
            if (!ready()) {
                return false;
            }
            return canvas_->draw(true) == tvg::Result::Success && canvas_->sync() == tvg::Result::Success;
        }

        [[nodiscard]] auto ready() const noexcept -> bool override {
            return canvas_ != nullptr && target_.pixels != nullptr;
        }

        [[nodiscard]] auto current_target() const noexcept -> RenderTargetView override {
            return target_;
        }

    private:
        RenderTargetView target_{};
        std::unique_ptr<tvg::SwCanvas> canvas_{};
        ThorVGSceneAdapter adapter_{};
    };
}
