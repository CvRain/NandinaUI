module;

#include <thorvg-1/thorvg.h>
#include <vector>
#include <variant>

export module nandina.render.thorvg_adapter;

import nandina.render.scene;

export namespace nandina::render {

    class ThorVGSceneAdapter {
    public:
        auto submit(const Scene& scene, tvg::SwCanvas& canvas) const -> void {
            std::vector<PushClip> clip_stack;
            for (const auto& command : scene.commands()) {
                std::visit([&](const auto& value) {
                    submit_one(value, canvas, clip_stack);
                }, command);
            }
        }

    private:
        static auto apply_clip(tvg::Paint& paint, const PushClip& clip) -> void {
            auto* clipper = tvg::Shape::gen();
            clipper->appendRect(
                clip.rect.x(),
                clip.rect.y(),
                clip.rect.width(),
                clip.rect.height(),
                clip.radius_x,
                clip.radius_y);
            paint.clip(clipper);
        }

        static auto submit_one(const DrawRect& command, tvg::SwCanvas& canvas, const std::vector<PushClip>& clip_stack) -> void {
            const auto rgb = command.color.to<NanRgb>();
            auto* shape = tvg::Shape::gen();
            shape->appendRect(
                command.rect.x(),
                command.rect.y(),
                command.rect.width(),
                command.rect.height(),
                0.0f,
                0.0f);
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            if (!clip_stack.empty()) {
                apply_clip(*shape, clip_stack.back());
            }
            canvas.add(shape);
        }

        static auto submit_one(const DrawRoundedRect& command, tvg::SwCanvas& canvas, const std::vector<PushClip>& clip_stack) -> void {
            const auto rgb = command.color.to<NanRgb>();
            auto* shape = tvg::Shape::gen();
            shape->appendRect(
                command.rect.x(),
                command.rect.y(),
                command.rect.width(),
                command.rect.height(),
                command.radius_x,
                command.radius_y);
            shape->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            if (!clip_stack.empty()) {
                apply_clip(*shape, clip_stack.back());
            }
            canvas.add(shape);
        }

        static auto submit_one(const DrawText& command, tvg::SwCanvas& canvas, const std::vector<PushClip>& clip_stack) -> void {
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
            if (!clip_stack.empty()) {
                apply_clip(*text, clip_stack.back());
            }
            canvas.add(text);
        }

        static auto submit_one(const PushClip& command, tvg::SwCanvas&, std::vector<PushClip>& clip_stack) -> void {
            clip_stack.push_back(command);
        }

        static auto submit_one(const PopClip&, tvg::SwCanvas&, std::vector<PushClip>& clip_stack) -> void {
            if (!clip_stack.empty()) {
                clip_stack.pop_back();
            }
        }
    };
}
