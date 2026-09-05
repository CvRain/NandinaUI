//
// widget/avatar - circular initials badge (pure display; image later).
//

#include "avatar.hpp"

#include "primitives/box_painter.hpp"
#include "../foundation/utf8.hpp"
#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto near(const float lhs, const float rhs) -> bool {
            return std::abs(lhs - rhs) <= foundation::nan_epsilon;
        }

        [[nodiscard]] auto
        same_text_style(const primitives::TextStyle& lhs, const primitives::TextStyle& rhs)
            -> bool {
            return lhs.color.approx_equals(rhs.color) && near(lhs.font_size, rhs.font_size)
                && lhs.font == rhs.font && lhs.overflow == rhs.overflow
                && lhs.max_lines == rhs.max_lines;
        }
    } // namespace

    Avatar::Avatar(std::string name, theme::NanTheme theme): name_(std::move(name)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        text_.set_text(initials());
        apply_text_style();
        const auto style = resolved_style();
        set_size(foundation::NanSize(style.metrics.box_size, style.metrics.box_size));
    }

    auto Avatar::create(std::string name, theme::NanTheme theme) -> std::shared_ptr<Avatar> {
        return std::make_shared<Avatar>(std::move(name), theme);
    }

    void Avatar::set_name(std::string name) {
        name_ = std::move(name);
        text_.set_text(initials());
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Avatar::name() const -> std::string_view {
        return name_;
    }

    auto Avatar::initials() const -> std::string {
        if (name_.empty()) {
            return {};
        }
        const auto decoded = foundation::utf8::decode(name_);
        if (decoded.empty()) {
            return {};
        }
        char32_t first = decoded.front().value;
        // 仅对 ASCII 小写字母转大写；CJK/其它文字按原样保留首个码点。
        if (first >= U'a' && first <= U'z') {
            first = static_cast<char32_t>(first - U'a' + U'A');
        }
        return foundation::utf8::encode(first);
    }

    void Avatar::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_style();
        mark_layout_dirty();
    }

    auto Avatar::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Avatar::set_override(theme::AvatarRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_style();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Avatar::resolved_style() const -> theme::ResolvedAvatarStyle {
        auto style = theme::resolve_avatar(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Avatar::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        mark_layout_dirty();
    }

    void Avatar::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Avatar::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Avatar::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_style();
        mark_layout_dirty();
    }

    void Avatar::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_style();
        mark_layout_dirty();
    }

    void Avatar::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::BoxPainter::paint(context, world, style.container, context.opacity());

        apply_text_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const float text_width = context.logical_to_screen(text_.measured_text_width());
        const auto text_position = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);
    }

    auto Avatar::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        const float size = style.metrics.box_size;
        return constraints.constrain(foundation::NanSize(size, size));
    }

    auto Avatar::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::generic,
            .label = name_,
        };
    }

    void Avatar::apply_text_style() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.label.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style.label.font_size,
            .font = context.font_from_context ? context.font : text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(text_.style(), text_style)) {
            text_.set_style(text_style);
        }
    }
} // namespace nandina::widget
