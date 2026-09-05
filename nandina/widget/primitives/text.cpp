//
// widget/primitives/text — minimal text drawing primitive.
//

#include "text.hpp"
#include "../../render/draw_context.hpp"
#include "../../text/font_pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget::primitives
{

    void TextColorProperty::set(foundation::NanColor color) {
        text_->set_color(std::move(color));
    }

    void TextColorProperty::set_behavior(animation::Behavior<foundation::NanColor> behavior) {
        text_->color_presentation_.set_behavior(std::move(behavior));
    }

    auto TextColorProperty::value() const noexcept -> const foundation::NanColor* {
        return text_->color_presentation_.value();
    }

    auto TextColorProperty::target() const noexcept -> const foundation::NanColor* {
        return text_->color_presentation_.target();
    }

    void TextFontSizeProperty::set(const float size) {
        text_->set_font_size(size);
    }

    void TextFontSizeProperty::set_behavior(animation::Behavior<float> behavior) {
        text_->font_size_presentation_.set_behavior(std::move(behavior));
    }

    void TextFontSizeProperty::set_spring(animation::SpringSpec spec) {
        text_->font_size_presentation_.set_spring(std::move(spec));
    }

    auto TextFontSizeProperty::value() const noexcept -> const float* {
        return text_->font_size_presentation_.value();
    }

    auto TextFontSizeProperty::target() const noexcept -> const float* {
        return text_->font_size_presentation_.target();
    }

    Text::Text(std::string text, const ITextLayoutBackend& backend):
        text_(std::move(text), [this](const std::string& value) { apply_text(value); }),
        color_presentation_(*this, style_.color, scene::DirtyFlags::paint),
        font_size_presentation_(
            *this,
            style_.font_size,
            scene::layout_dirty_flags | scene::DirtyFlags::paint
        ),
        backend_(&backend) {
        update_metrics();
    }

    void Text::set_text(std::string text) {
        (void)text_.set(std::move(text));
    }

    void Text::apply_text(const std::string&) {
        mark_layout_dirty();
        mark_semantics_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::text() const -> std::string_view {
        return text_.get();
    }

    auto Text::text_property() -> reactive::Property<std::string>& {
        return text_;
    }

    auto Text::text_property() const -> reactive::ReadProperty<std::string> {
        return text_.as_readonly();
    }

    void Text::set_style(TextStyle style) {
        style.max_lines = std::max(1, style.max_lines);
        if (!std::isfinite(style.font_size) || style.font_size <= 0.0F) {
            throw std::invalid_argument("Text font size must be finite and positive");
        }
        if (style.font.weight < 1 || style.font.weight > 1000) {
            throw std::invalid_argument("Text font weight must be between 1 and 1000");
        }
        const bool font_changed = style_.font != style.font;
        style_ = style;
        color_presentation_.set(style_.color);
        font_size_presentation_.set(style_.font_size);
        color_explicit_ = true;
        font_size_explicit_ = true;
        font_explicit_ = true;
        if (font_changed) {
            resolve_font();
        }
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::style() const -> const TextStyle& {
        return style_;
    }

    void Text::set_color(foundation::NanColor color) {
        style_.color = color;
        color_presentation_.set(std::move(color));
        color_explicit_ = true;
    }

    auto Text::color() const -> foundation::NanColor {
        return style_.color;
    }

    void Text::set_font_size(float size) {
        if (!std::isfinite(size) || size <= 0.0F) {
            throw std::invalid_argument("Text font size must be finite and positive");
        }
        style_.font_size = size;
        font_size_presentation_.set(size);
        font_size_explicit_ = true;
        update_metrics(last_layout_constraints());
    }

    auto Text::font_size() const -> float {
        return style_.font_size;
    }

    auto Text::visual_part(visual::label_t) noexcept -> Text& {
        return *this;
    }

    auto Text::property(visual::color_t) noexcept -> TextColorProperty {
        return TextColorProperty {*this};
    }

    auto Text::property(visual::font_size_t) noexcept -> TextFontSizeProperty {
        return TextFontSizeProperty {*this};
    }

    void Text::set_font(text::FontRequest request) {
        if (request.weight < 1 || request.weight > 1000) {
            throw std::invalid_argument("Text font weight must be between 1 and 1000");
        }
        font_explicit_ = true;
        if (style_.font == request) {
            return;
        }
        style_.font = std::move(request);
        resolve_font();
    }

    void Text::set_font_family(resource::ResourceKey family) {
        auto request = style_.font;
        request.family = std::move(family);
        set_font(std::move(request));
    }

    void Text::clear_font_family() {
        auto request = style_.font;
        request.family.reset();
        set_font(std::move(request));
    }

    void Text::set_font_weight(const int weight) {
        auto request = style_.font;
        request.weight = weight;
        set_font(std::move(request));
    }

    void Text::set_font_slant(const text::FontSlant slant) {
        auto request = style_.font;
        request.slant = slant;
        set_font(std::move(request));
    }

    auto Text::font() const -> const text::FontRequest& {
        return style_.font;
    }

    void Text::set_overflow(TextOverflow overflow) {
        style_.overflow = overflow;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::overflow() const -> TextOverflow {
        return style_.overflow;
    }

    void Text::set_max_lines(int lines) {
        style_.max_lines = std::max(1, lines);
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::max_lines() const -> int {
        return style_.max_lines;
    }

    auto Text::measured_text_width() const -> float {
        float width = 0.0F;
        for (const auto& line: layout_.lines) {
            width = std::max(width, line.size.get_width());
        }
        return width;
    }

    auto Text::measured_text_height() const -> float {
        float height = 0.0F;
        for (const auto& line: layout_.lines) {
            height += line.size.get_height();
        }
        return height;
    }

    auto Text::laid_out_font_size() const -> float {
        return layout_.font_size;
    }

    auto Text::layout_result() const -> const TextLayoutResult& {
        return layout_;
    }

    void Text::set_text_pipeline(TextPipeline pipeline) {
        if (pipeline.backend == nullptr) {
            throw std::invalid_argument("TextPipeline requires a layout backend");
        }
        backend_ = pipeline.backend;
        renderer_ = pipeline.renderer;
        pipeline_explicit_ = true;
        resolved_font_pipeline_.reset();
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::text_pipeline() const -> TextPipeline {
        return {.backend = backend_, .renderer = renderer_};
    }

    void Text::apply_default_text_pipeline(const TextPipeline& pipeline) {
        if (pipeline_explicit_) {
            return;
        }
        backend_ = pipeline.backend;
        renderer_ = pipeline.renderer;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    void Text::apply_font_context(text::FontPipelineCache& context) {
        font_context_ = &context;
        resolve_font();
    }

    void Text::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        bool metrics_changed = false;
        bool font_changed = false;

        if (!color_explicit_) {
            style_.color = context.text_color;
            color_presentation_.set(style_.color);
        }
        if (!font_size_explicit_ && style_.font_size != context.font_size) {
            style_.font_size = context.font_size;
            font_size_presentation_.set(style_.font_size);
            metrics_changed = true;
        }
        if (!font_explicit_ && style_.font != context.font) {
            style_.font = context.font;
            font_changed = true;
        }

        if (font_changed) {
            resolve_font();
        }
        if (metrics_changed) {
            mark_layout_dirty();
            update_metrics(last_layout_constraints());
        }
    }

    void Text::set_layout_backend(const ITextLayoutBackend& backend) {
        backend_ = &backend;
        pipeline_explicit_ = true;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::layout_backend() const -> const ITextLayoutBackend& {
        return *backend_;
    }

    void Text::set_layout_renderer(ITextLayoutRenderer* renderer) {
        renderer_ = renderer;
        pipeline_explicit_ = true;
    }

    auto Text::layout_renderer() const -> ITextLayoutRenderer* {
        return renderer_;
    }

    void Text::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        const auto& color = *color_presentation_.value();
        if (layout_.lines.empty() || color.alpha() <= 0.0F) {
            return;
        }

        auto clip = style_.overflow == TextOverflow::clip
            ? ctx.clip().push(
                  foundation::NanRect::from_xywh(
                      position.get_x(),
                      position.get_y(),
                      // 测量 advance 之外允许字形墨迹悬垂 1~2px，否则最后一个字形的
                      // 右侧墨迹会被裁掉（见 glyph_overhang_allowance）。
                      ctx.logical_to_screen(
                          layout_.size.get_width() + glyph_overhang_allowance(layout_.font_size)
                      ),
                      ctx.logical_to_screen(layout_.size.get_height())
                  )
              )
            : render::ClipStack::Guard {nullptr, false};

        const auto draw_color = color.with_alpha(color.alpha() * ctx.opacity());
        if (renderer_ != nullptr) {
            renderer_->draw(layout_, ctx, position, draw_color);
            return;
        }

        float y = position.get_y();
        for (const auto& line: layout_.lines) {
            if (!line.visible_text.empty()) {
                ctx.device().draw_text(
                    line.visible_text,
                    foundation::NanPoint(position.get_x(), y),
                    ctx.logical_to_screen(layout_.font_size),
                    draw_color
                );
            }
            y += ctx.logical_to_screen(line.size.get_height());
        }
    }

    void Text::on_draw(render::DrawContext& ctx) {
        const auto pos = ctx.world_transform().transform_point(foundation::NanPoint::zero());
        draw_at(ctx, pos);
    }

    auto Text::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        update_metrics(constraints);
        return size();
    }

    auto Text::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::static_text,
            .label = std::string(text()),
        };
    }

    void Text::apply_component_color(foundation::NanColor color) {
        if (!color_explicit_) {
            style_.color = color;
            color_presentation_.set(std::move(color));
        }
    }

    void Text::clear_explicit_color() noexcept {
        color_explicit_ = false;
    }

    void Text::apply_component_font_size(const float size) {
        if (font_size_explicit_ || style_.font_size == size) {
            return;
        }
        style_.font_size = size;
        font_size_presentation_.set(size);
        update_metrics(last_layout_constraints());
    }

    void Text::update_metrics(scene::LayoutConstraints constraints) {
        auto presentation_style = style_;
        presentation_style.font_size = *font_size_presentation_.value();
        layout_ = backend_->layout(
            TextLayoutInput {
                .text = text_.get(),
                .style = std::move(presentation_style),
                .constraints = constraints,
            }
        );
        set_size(layout_.size);
    }

    void Text::resolve_font() {
        if (pipeline_explicit_ || font_context_ == nullptr) {
            return;
        }
        auto pipeline = font_context_->get(style_.font);
        if (!pipeline) {
            throw std::runtime_error("Text cannot resolve font: " + pipeline.error().message);
        }
        resolved_font_pipeline_ = std::move(*pipeline);
        const auto resolved = resolved_font_pipeline_->pipeline();
        backend_ = resolved.backend;
        renderer_ = resolved.renderer;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

} // namespace nandina::widget::primitives
