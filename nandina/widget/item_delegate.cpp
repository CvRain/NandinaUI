#include "item_delegate.hpp"

#include "../render/draw_context.hpp"
#include "../theme/nan_style.hpp"
#include "../theme/theme_manager.hpp"
#include "label.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        inline constexpr float kCellCornerRadius = 4.0F;
        inline constexpr float kCellRingThickness = 1.5F;
        inline constexpr float kCellHorizontalInset = 6.0F;

        /// 默认文本单元格视觉：继承 Label 以保持 cell_node() 到 Label* 的兼容，
        /// 并按 ItemState::selected / current 绘制语义背景与当前格焦点框。
        class TextItemCell final: public Label {
        public:
            TextItemCell(reactive::Graph& graph, std::string text, theme::NanTheme theme):
                Label(graph, std::move(text), theme),
                theme_(std::move(theme)) {
                set_typography_role(theme::TypographyRole::label);
                set_max_lines(1);
                primitives::Text::set_overflow(text::TextOverflow::ellipsis);
                set_vertical_align(text::TextVerticalAlign::center);
                scene::NanControl::set_overflow(scene::ControlOverflow::clip);
                resolve_colors();
            }

            void apply_item_state(const ItemState& state) {
                const bool selection_changed = selected_ != state.selected;
                const bool current_changed = current_ != state.current;
                selected_ = state.selected;
                current_ = state.current;
                if (selection_changed) {
                    set_color_token(
                        selected_ ? theme::ColorToken::accent_foreground
                                  : theme::ColorToken::on_surface
                    );
                }
                if (selection_changed || current_changed) {
                    mark_dirty(scene::DirtyFlags::paint);
                }
            }

            void on_draw(render::DrawContext& ctx) override {
                const auto world =
                    render::world_bounds_from_local(ctx.world_transform(), local_rect());
                const float radius = ctx.logical_to_screen(kCellCornerRadius);
                if (selected_) {
                    ctx.device().draw_rounded_rect(
                        world,
                        radius,
                        fill_color_.with_alpha(fill_color_.alpha() * ctx.opacity())
                    );
                }

                const float inset = std::min(
                    ctx.logical_to_screen(kCellHorizontalInset),
                    world.get_width() * 0.25F
                );
                const auto text_rect = foundation::NanRect::from_xywh(
                    world.get_left() + inset,
                    world.get_top(),
                    std::max(0.0F, world.get_width() - inset * 2.0F),
                    world.get_height()
                );
                draw_in(ctx, text_rect);

                if (current_) {
                    const float thickness = ctx.logical_to_screen(kCellRingThickness);
                    ctx.device().draw_rounded_rect_outline(
                        world,
                        radius,
                        thickness,
                        ring_color_.with_alpha(ring_color_.alpha() * ctx.opacity())
                    );
                }
            }

        protected:
            void on_theme_changed(const theme::ThemeManager& manager) override {
                theme_ = manager.theme();
                resolve_colors();
                Label::on_theme_changed(manager);
                mark_dirty(scene::DirtyFlags::paint);
            }

        private:
            void resolve_colors() {
                fill_color_ = theme::resolve_theme_color(
                    theme_,
                    theme::ThemeColor::token(theme::ColorToken::accent)
                );
                ring_color_ = theme::resolve_theme_color(
                    theme_,
                    theme::ThemeColor::token(theme::ColorToken::ring)
                );
            }

            theme::NanTheme theme_;
            theme::NanColor fill_color_ {};
            theme::NanColor ring_color_ {};
            bool selected_ = false;
            bool current_ = false;
        };
    } // namespace

    TextItemDelegate::TextItemDelegate(TextFunction text): text_(std::move(text)) {}

    auto TextItemDelegate::text_for(const ItemBinding& binding) const -> std::string {
        return text_ ? text_(binding.model, binding.model_row) : std::string {};
    }

    auto TextItemDelegate::create_item(const widget::BuildContext& ui, const ItemBinding& binding)
        -> std::shared_ptr<scene::NanControl> {
        auto cell = std::make_shared<TextItemCell>(ui.graph(), text_for(binding), ui.theme());
        cell->apply_item_state(binding.state);
        return cell;
    }

    void TextItemDelegate::update_item(scene::NanControl& control, const ItemBinding& binding) {
        // 只由本 delegate 创建 TextItemCell；无 RTTI 构建下用静态转换（所有权在宿主侧）。
        auto& cell = static_cast<TextItemCell&>(control);
        cell.set_text(text_for(binding));
        cell.apply_item_state(binding.state);
    }
} // namespace nandina::widget
