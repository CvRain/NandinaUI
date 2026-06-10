#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.foundation.color;
import nandina.foundation.nan_rect;
import nandina.render;

namespace
{

    class ThorvgCanvasScope {
    public:
        ThorvgCanvasScope(const std::uint32_t width, const std::uint32_t height):
            pixels_(width * height, 0u),
            width_(width) {
                if (tvg::Initializer::init(0u) != tvg::Result::Success) {
                    throw std::runtime_error("tvg::Initializer::init failed");
                }
            canvas_.reset(tvg::SwCanvas::gen());
                if (!canvas_) {
                    tvg::Initializer::term();
                    throw std::runtime_error("tvg::SwCanvas::gen failed");
                }
                if (canvas_->target(pixels_.data(), width, width, height, tvg::ColorSpace::ARGB8888)
                    != tvg::Result::Success)
                {
                    canvas_.reset();
                    tvg::Initializer::term();
                    throw std::runtime_error("tvg::SwCanvas::target failed");
                }
        }

        ~ThorvgCanvasScope() {
            canvas_.reset();
            tvg::Initializer::term();
        }

        [[nodiscard]] auto canvas() const noexcept -> tvg::SwCanvas& {
            return *canvas_;
        }

        auto render() const -> void {
            ASSERT_EQ(canvas_->draw(true), tvg::Result::Success);
            ASSERT_EQ(canvas_->sync(), tvg::Result::Success);
        }

        [[nodiscard]] auto pixel_at(const std::uint32_t x, const std::uint32_t y) const noexcept
            -> std::uint32_t {
            return pixels_.at(static_cast<std::size_t>(y) * width_ + x);
        }

    private:
        std::vector<std::uint32_t> pixels_ {};
        std::unique_ptr<tvg::SwCanvas> canvas_ {};
        std::uint32_t width_ {};
    };

    class ThorvgInitializerScope {
    public:
        ThorvgInitializerScope() {
                if (tvg::Initializer::init(0u) != tvg::Result::Success) {
                    throw std::runtime_error("tvg::Initializer::init failed");
                }
        }

        ~ThorvgInitializerScope() {
            tvg::Initializer::term();
        }
    };

} // namespace

TEST(RenderSceneTest, SceneCollectsCommands) {
    nandina::render::Scene scene;
    scene
        .push_rect(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {4.0f, 6.0f},
                nandina::geometry::NanSize {24.0f, 18.0f}
            },
            nandina::NanColor::from(nandina::NanRgb {10, 20, 30})
        )
        .push_rounded_rect(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {30.0f, 10.0f},
                nandina::geometry::NanSize {20.0f, 16.0f}
            },
            4.0f,
            4.0f,
            nandina::NanColor::from(nandina::NanRgb {40, 50, 60})
        )
        .push_text(
            "A",
            8.0f,
            24.0f,
            14.0f,
            nandina::NanColor::from(nandina::NanRgb {255, 255, 255})
        )
        .push_clip(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {0.0f, 0.0f},
                nandina::geometry::NanSize {16.0f, 16.0f}
            }
        )
        .pop_clip();

    EXPECT_EQ(scene.command_count(), 5u);
}

TEST(RenderSceneTest, ThorVGAdapterRendersRectCommandsIntoCanvas) {
    nandina::render::Scene scene;
    scene
        .push_rect(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {8.0f, 8.0f},
                nandina::geometry::NanSize {20.0f, 20.0f}
            },
            nandina::NanColor::from(nandina::NanRgb {255, 0, 0})
        )
        .push_rounded_rect(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {40.0f, 8.0f},
                nandina::geometry::NanSize {20.0f, 20.0f}
            },
            6.0f,
            6.0f,
            nandina::NanColor::from(nandina::NanRgb {0, 255, 0})
        );

    nandina::render::ThorVGSceneAdapter adapter;
    ThorvgCanvasScope canvas_scope {96u, 48u};
    adapter.submit(scene, canvas_scope.canvas());
    canvas_scope.render();

    EXPECT_NE(canvas_scope.pixel_at(12u, 12u), 0u);
    EXPECT_NE(canvas_scope.pixel_at(48u, 16u), 0u);
}

TEST(RenderSceneTest, ThorVGRenderBackendProvidesBeginSubmitPresentLoop) {
    ThorvgInitializerScope initializer;
    std::vector<std::uint32_t> pixels(96u * 48u, 0u);
    nandina::render::RenderTargetView target {
        .pixels = pixels.data(),
        .width = 96u,
        .height = 48u,
        .stride = 96u,
        .format = nandina::render::RenderTargetFormat::argb8888,
    };

    nandina::render::Scene scene;
    scene.push_rect(
        nandina::geometry::NanRect {
            nandina::geometry::NanPoint {10.0f, 10.0f},
            nandina::geometry::NanSize {16.0f, 16.0f}
        },
        nandina::NanColor::from(nandina::NanRgb {0, 0, 255})
    );

    nandina::render::ThorVGRenderBackend backend;
    ASSERT_TRUE(backend.begin_frame(target));
    EXPECT_TRUE(backend.ready());
    EXPECT_TRUE(backend.submit(scene));
    EXPECT_TRUE(backend.present());
    EXPECT_NE(pixels[static_cast<std::size_t>(12u) * 96u + 12u], 0u);
}

TEST(RenderSceneTest, ThorVGAdapterAppliesTopClipToSubsequentPaints) {
    nandina::render::Scene scene;
    scene
        .push_clip(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {0.0f, 0.0f},
                nandina::geometry::NanSize {8.0f, 8.0f}
            }
        )
        .push_rect(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {0.0f, 0.0f},
                nandina::geometry::NanSize {20.0f, 20.0f}
            },
            nandina::NanColor::from(nandina::NanRgb {255, 0, 0})
        )
        .pop_clip();

    nandina::render::ThorVGSceneAdapter adapter;
    ThorvgCanvasScope canvas_scope {32u, 32u};
    adapter.submit(scene, canvas_scope.canvas());
    canvas_scope.render();

    EXPECT_NE(canvas_scope.pixel_at(2u, 2u), 0u);
    EXPECT_EQ(canvas_scope.pixel_at(16u, 16u), 0u);
}

TEST(RenderSceneTest, ThorVGAdapterIntersectsNestedClips) {
    nandina::render::Scene scene;
    scene
        .push_clip(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {0.0f, 0.0f},
                nandina::geometry::NanSize {20.0f, 20.0f}
            }
        )
        .push_clip(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {10.0f, 10.0f},
                nandina::geometry::NanSize {8.0f, 8.0f}
            }
        )
        .push_rect(
            nandina::geometry::NanRect {
                nandina::geometry::NanPoint {0.0f, 0.0f},
                nandina::geometry::NanSize {32.0f, 32.0f}
            },
            nandina::NanColor::from(nandina::NanRgb {0, 255, 0})
        )
        .pop_clip()
        .pop_clip();

    nandina::render::ThorVGSceneAdapter adapter;
    ThorvgCanvasScope canvas_scope {32u, 32u};
    adapter.submit(scene, canvas_scope.canvas());
    canvas_scope.render();

    EXPECT_EQ(canvas_scope.pixel_at(6u, 6u), 0u);
    EXPECT_NE(canvas_scope.pixel_at(12u, 12u), 0u);
    EXPECT_EQ(canvas_scope.pixel_at(24u, 24u), 0u);
}
