module;

#include <utility>
#include <string>
#include <variant>
#include <vector>

export module nandina.render.scene;

export import nandina.foundation.color;
export import nandina.foundation.nan_rect;

export namespace nandina::render {

    struct DrawRect {
        geometry::NanRect rect{};
        NanColor color{};
    };

    struct DrawRoundedRect {
        geometry::NanRect rect{};
        float radius_x{0.0f};
        float radius_y{0.0f};
        NanColor color{};
    };

    struct DrawText {
        std::string text{};
        float x{0.0f};
        float y{0.0f};
        float font_size{14.0f};
        NanColor color{};
        std::string font_family{};
        float layout_width{0.0f};
        float layout_height{0.0f};
        float align_x{0.0f};
        float align_y{0.0f};
    };

    struct PushClip {
        geometry::NanRect rect{};
        float radius_x{0.0f};
        float radius_y{0.0f};
    };

    struct PopClip {};

    using DrawCommand = std::variant<DrawRect, DrawRoundedRect, DrawText, PushClip, PopClip>;

    class Scene {
    public:
        auto clear() noexcept -> void {
            commands_.clear();
        }

        auto push_rect(const geometry::NanRect& rect, const NanColor& color) -> Scene& {
            commands_.push_back(DrawRect{rect, color});
            return *this;
        }

        auto push_rounded_rect(
            const geometry::NanRect& rect,
            const float radius_x,
            const float radius_y,
            const NanColor& color) -> Scene& {
            commands_.push_back(DrawRoundedRect{rect, radius_x, radius_y, color});
            return *this;
        }

        auto push_text(
            std::string text,
            const float x,
            const float y,
            const float font_size,
            const NanColor& color,
            std::string font_family = {},
            const float layout_width = 0.0f,
            const float layout_height = 0.0f,
            const float align_x = 0.0f,
            const float align_y = 0.0f) -> Scene& {
            commands_.push_back(DrawText{
                .text = std::move(text),
                .x = x,
                .y = y,
                .font_size = font_size,
                .color = color,
                .font_family = std::move(font_family),
                .layout_width = layout_width,
                .layout_height = layout_height,
                .align_x = align_x,
                .align_y = align_y,
            });
            return *this;
        }

        auto push_clip(const geometry::NanRect& rect, const float radius_x = 0.0f, const float radius_y = 0.0f) -> Scene& {
            commands_.push_back(PushClip{rect, radius_x, radius_y});
            return *this;
        }

        auto pop_clip() -> Scene& {
            commands_.push_back(PopClip{});
            return *this;
        }

        [[nodiscard]] auto command_count() const noexcept -> std::size_t {
            return commands_.size();
        }

        [[nodiscard]] auto commands() const noexcept -> const std::vector<DrawCommand>& {
            return commands_;
        }

    private:
        std::vector<DrawCommand> commands_{};
    };
}
