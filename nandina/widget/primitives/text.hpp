//
// widget/primitives/text — minimal text drawing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP

#include "../../animation/property_endpoint.hpp"
#include "../../reactive/property.hpp"
#include "../visual_property.hpp"
#include "text_layout_backend.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace nandina::text
{
    class FontPipeline;
    class FontPipelineCache;
} // namespace nandina::text

namespace nandina::widget::primitives
{

    class Text;

    class TextColorProperty {
    public:
        explicit TextColorProperty(Text& text) noexcept: text_(&text) {}
        void set(foundation::NanColor color);
        void set_behavior(animation::Behavior<foundation::NanColor> behavior);
        [[nodiscard]] auto value() const noexcept -> const foundation::NanColor*;
        [[nodiscard]] auto target() const noexcept -> const foundation::NanColor*;

    private:
        Text* text_;
    };

    class TextFontSizeProperty {
    public:
        explicit TextFontSizeProperty(Text& text) noexcept: text_(&text) {}
        void set(float size);
        void set_behavior(animation::Behavior<float> behavior);
        void set_spring(animation::SpringSpec spec);
        [[nodiscard]] auto value() const noexcept -> const float*;
        [[nodiscard]] auto target() const noexcept -> const float*;

    private:
        Text* text_;
    };

    class Text: public scene::NanControl {
        friend class TextColorProperty;
        friend class TextFontSizeProperty;

    public:
        explicit Text(
            std::string text = {},
            const ITextLayoutBackend& backend = deterministic_text_layout_backend()
        );

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        [[nodiscard]] auto text_property() -> reactive::Property<std::string>&;
        [[nodiscard]] auto text_property() const -> reactive::ReadProperty<std::string>;

        void set_style(TextStyle style);
        [[nodiscard]] auto style() const -> const TextStyle&;

        void set_color(foundation::NanColor color);
        [[nodiscard]] auto color() const -> foundation::NanColor;

        void set_font_size(float size);
        [[nodiscard]] auto font_size() const -> float;
        [[nodiscard]] auto visual_part(visual::label_t) noexcept -> Text&;
        [[nodiscard]] auto property(visual::color_t) noexcept -> TextColorProperty;
        [[nodiscard]] auto property(visual::font_size_t) noexcept -> TextFontSizeProperty;
        void set_font(text::FontRequest request);
        void set_font_family(resource::ResourceKey family);
        void clear_font_family();
        void set_font_weight(int weight);
        void set_font_slant(text::FontSlant slant);
        [[nodiscard]] auto font() const -> const text::FontRequest&;

        void set_overflow(TextOverflow overflow);
        [[nodiscard]] auto overflow() const -> TextOverflow;

        void set_max_lines(int lines);
        [[nodiscard]] auto max_lines() const -> int;

        [[nodiscard]] auto measured_text_width() const -> float;
        [[nodiscard]] auto measured_text_height() const -> float;
        [[nodiscard]] auto laid_out_font_size() const -> float;
        [[nodiscard]] auto layout_result() const -> const TextLayoutResult&;

        void set_text_pipeline(TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> TextPipeline;
        void apply_default_text_pipeline(const TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;

        /// The referenced backend must outlive this Text instance.
        void set_layout_backend(const ITextLayoutBackend& backend);
        [[nodiscard]] auto layout_backend() const -> const ITextLayoutBackend&;
        void set_layout_renderer(ITextLayoutRenderer* renderer);
        [[nodiscard]] auto layout_renderer() const -> ITextLayoutRenderer*;

        void draw_at(render::DrawContext& ctx, foundation::NanPoint position);
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        void apply_component_color(foundation::NanColor color);
        void apply_component_font_size(float size);
        void clear_explicit_color() noexcept;

    private:
        void apply_text(const std::string& text);
        void
        update_metrics(scene::LayoutConstraints constraints = scene::LayoutConstraints::loose());
        void resolve_font();

        reactive::Property<std::string> text_;
        TextStyle style_ {};
        animation::PropertyEndpoint<foundation::NanColor> color_presentation_;
        animation::PropertyEndpoint<float> font_size_presentation_;
        TextLayoutResult layout_ {};
        const ITextLayoutBackend* backend_ = nullptr;
        ITextLayoutRenderer* renderer_ = nullptr;
        bool pipeline_explicit_ = false;
        bool color_explicit_ = false;
        bool font_size_explicit_ = false;
        bool font_explicit_ = false;
        text::FontPipelineCache* font_context_ = nullptr;
        std::shared_ptr<text::FontPipeline> resolved_font_pipeline_;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
