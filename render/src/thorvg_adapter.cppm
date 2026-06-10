module;

#include <optional>
#include <thorvg-1/thorvg.h>
#include <variant>
#include <vector>

export module nandina.render.thorvg_adapter;

import nandina.render.scene;
import nandina.render.draw_clip;

export namespace nandina::render
{

    /**
     * ThorVGSceneAdapter — submits a Scene to a SwCanvas.
     *
     * PushClip / PopClip commands push and pop the unified
     * thread-local draw clip stack (from nandina.render.draw_clip).
     * This means Scene-based clip and direct-draw clip share
     * the same state, so a widget that emits Scene PushClip
     * can clip children that draw directly to the canvas.
     */
    class ThorVGSceneAdapter {
    public:
        auto submit(const Scene& scene, tvg::SwCanvas& canvas) const -> void {
                for (const auto& command: scene.commands()) {
                    std::visit([&](const auto& value) { submit_one(value, canvas); }, command);
                }
        }

    private:
        static auto submit_one(const DrawRect& command, tvg::SwCanvas& canvas) -> void {
            const auto rgb = command.color.to<NanRgb>();
            auto* shape = tvg::Shape::gen();
            shape->appendRect(
                command.rect.x(),
                command.rect.y(),
                command.rect.width(),
                command.rect.height(),
                0.0f,
                0.0f
            );
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            nandina::render::draw_clip::paint_with_clip(canvas, shape);
        }

        static auto submit_one(const DrawRoundedRect& command, tvg::SwCanvas& canvas) -> void {
            const auto rgb = command.color.to<NanRgb>();
            auto* shape = tvg::Shape::gen();
            shape->appendRect(
                command.rect.x(),
                command.rect.y(),
                command.rect.width(),
                command.rect.height(),
                command.radius_x,
                command.radius_y
            );
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            nandina::render::draw_clip::paint_with_clip(canvas, shape);
        }

        static auto submit_one(const DrawText& command, tvg::SwCanvas& canvas) -> void {
            const auto rgb = command.color.to<NanRgb>();
            auto* text = tvg::Text::gen();
            text->text(command.text.c_str());
            text->size(command.font_size);
                if (!command.font_family.empty()) {
                    text->font(command.font_family.c_str());
                }
                if (command.layout_width > 0.0f || command.layout_height > 0.0f) {
                    text->layout(command.layout_width, command.layout_height);
                    text->align(command.align_x, command.align_y);
                }
            text->fill(rgb.red(), rgb.green(), rgb.blue());
            text->translate(command.x, command.y);
            nandina::render::draw_clip::paint_with_clip(canvas, text);
        }

        static auto submit_one(const PushClip& command, tvg::SwCanvas&) -> void {
            nandina::render::draw_clip::push(command.rect, command.radius_x, command.radius_y);
        }

        static auto submit_one(const PopClip&, tvg::SwCanvas&) -> void {
            nandina::render::draw_clip::pop();
        }
    };
} // namespace nandina::render
