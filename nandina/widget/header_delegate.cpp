#include "header_delegate.hpp"

#include "../render/draw_context.hpp"
#include "../scene/control.hpp"
#include "label.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        /// 默认表头视觉根：单行省略 Label + 右侧排序三角（线段图元，不用 ▲/▼ 文本）。
        class TextHeaderVisual final: public scene::NanControl {
        public:
            TextHeaderVisual(reactive::Graph& graph, const theme::NanTheme& theme) {
                set_overflow(scene::ControlOverflow::clip);
                label_ = Label::create(graph, {}, theme);
                label_->set_typography_role(theme::TypographyRole::label);
                label_->set_max_lines(1);
                label_->set_overflow(text::TextOverflow::ellipsis);
                add_child(label_);
                apply(theme, {}, false);
            }

            void apply(
                const theme::NanTheme& theme,
                const std::optional<SortDirection> direction,
                const bool focused
            ) {
                indicator_color_ = theme::resolve_theme_color(
                    theme,
                    theme::ThemeColor::token(theme::ColorToken::foreground)
                );
                focus_color_ = theme::resolve_theme_color(
                    theme,
                    theme::ThemeColor::token(theme::ColorToken::ring)
                );
                direction_ = direction;
                focused_ = focused;
                mark_dirty(scene::DirtyFlags::paint);
            }

            void set_title(const std::string& title) {
                label_->set_text(title);
            }

        protected:
            auto on_layout() -> void override {
                (void)label_->measure_layout(foundation::NanLayoutConstraints::tight(size()));
                label_->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, width(), height()));
            }

            auto on_draw(render::DrawContext& ctx) -> void override {
                const auto world = ctx.world_transform();
                const float thickness = ctx.logical_to_screen(1.2F);
                if (direction_.has_value()) {
                    const float center_x = std::max(0.0F, width() - 10.0F);
                    const float center_y = height() * 0.5F;
                    const float half = 3.5F;
                    const bool ascending = *direction_ == SortDirection::ascending;
                    const auto apex = foundation::NanPoint(
                        center_x,
                        ascending ? center_y - half : center_y + half
                    );
                    const auto left = foundation::NanPoint(
                        center_x - half,
                        ascending ? center_y + half : center_y - half
                    );
                    const auto right = foundation::NanPoint(
                        center_x + half,
                        ascending ? center_y + half : center_y - half
                    );
                    const auto color =
                        indicator_color_.with_alpha(indicator_color_.alpha() * ctx.opacity());
                    const auto a = world.transform_point(apex);
                    const auto b = world.transform_point(left);
                    const auto c = world.transform_point(right);
                    ctx.device().draw_line(a, b, thickness, color);
                    ctx.device().draw_line(b, c, thickness, color);
                    ctx.device().draw_line(c, a, thickness, color);
                }
                if (focused_) {
                    // 焦点提示由默认视觉提供；作者 delegate 可通过 HeaderState.focused 自行绘制。
                    const auto color =
                        focus_color_.with_alpha(focus_color_.alpha() * ctx.opacity());
                    const auto underline_left =
                        world.transform_point(foundation::NanPoint(0.0F, height() - 1.0F));
                    const auto underline_right =
                        world.transform_point(foundation::NanPoint(width(), height() - 1.0F));
                    ctx.device().draw_line(underline_left, underline_right, thickness, color);
                }
            }

        private:
            std::shared_ptr<Label> label_;
            theme::NanColor indicator_color_ {};
            std::optional<SortDirection> direction_;
            bool focused_ = false;
            theme::NanColor focus_color_ {};
        };

        auto text_visual(scene::NanControl& control) -> TextHeaderVisual& {
            return static_cast<TextHeaderVisual&>(control);
        }
    } // namespace

    auto
    TextHeaderDelegate::create_header(const widget::BuildContext& ui, const HeaderBinding& binding)
        -> std::shared_ptr<scene::NanControl> {
        auto visual = std::make_shared<TextHeaderVisual>(ui.graph(), binding.theme);
        visual->set_title(binding.column.title);
        visual->apply(binding.theme, binding.state.sort_direction, binding.state.focused);
        return visual;
    }

    void
    TextHeaderDelegate::update_header(scene::NanControl& control, const HeaderBinding& binding) {
        auto& visual = text_visual(control);
        visual.set_title(binding.column.title);
        visual.apply(binding.theme, binding.state.sort_direction, binding.state.focused);
    }
} // namespace nandina::widget
