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
    namespace
    {
        /// 记忆化键的样式比较：只比**影响排版**的字段，并且用精确相等（不是
        /// `TextStyle::approx_equals` 的容差比较）—— 缓存命中必须意味着结果一定相同。
        [[nodiscard]] auto
        same_layout_style(const text::TextStyle& left, const text::TextStyle& right) -> bool {
            return left.font_size == right.font_size && left.line_height == right.line_height
                && left.font == right.font && left.overflow == right.overflow
                && left.max_lines == right.max_lines;
        }

        [[nodiscard]] auto same_layout_constraints(
            const foundation::NanLayoutConstraints& left,
            const foundation::NanLayoutConstraints& right
        ) -> bool {
            return left.min_width == right.min_width && left.max_width == right.max_width
                && left.min_height == right.min_height && left.max_height == right.max_height;
        }

        void validate_text_style(text::TextStyle& style) {
            style.max_lines = std::max(1, style.max_lines);
            if (!std::isfinite(style.font_size) || style.font_size <= 0.0F) {
                throw std::invalid_argument("Text font size must be finite and positive");
            }
            if (!std::isfinite(style.line_height) || style.line_height < 0.0F) {
                throw std::invalid_argument("Text line height must be finite and non-negative");
            }
            if (style.font.weight < 1 || style.font.weight > 1000) {
                throw std::invalid_argument("Text font weight must be between 1 and 1000");
            }
        }
    } // namespace

    void TextColorProperty::set(foundation::NanColor color) {
        text_->set_color(std::move(color));
    }

    void TextColorProperty::set_behavior(motion::Behavior<foundation::NanColor> behavior) {
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

    void TextFontSizeProperty::set_behavior(motion::Behavior<float> behavior) {
        text_->font_size_presentation_.set_behavior(std::move(behavior));
    }

    void TextFontSizeProperty::set_spring(motion::SpringSpec spec) {
        text_->font_size_presentation_.set_spring(std::move(spec));
    }

    auto TextFontSizeProperty::value() const noexcept -> const float* {
        return text_->font_size_presentation_.value();
    }

    auto TextFontSizeProperty::target() const noexcept -> const float* {
        return text_->font_size_presentation_.target();
    }

    Text::Text(std::string text, const text::ITextLayoutBackend& backend):
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

    void Text::set_style(text::TextStyle style) {
        validate_text_style(style);
        const auto before = style_;
        auto saved = capture_override_state();
        color_explicit_ = true;
        font_size_explicit_ = true;
        line_height_explicit_ = true;
        font_family_explicit_ = true;
        font_weight_explicit_ = true;
        font_slant_explicit_ = true;
        overflow_explicit_ = true;
        max_lines_explicit_ = true;
        align_explicit_ = true;
        vertical_align_explicit_ = true;
        try {
            commit_reconciled_style(before, std::move(style), true, true);
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
        if (same_layout_style(before, style_)) {
            // A full style snapshot declares explicit overrides; keep historical paint-dirty
            // contract even when only paint fields (or override flags) were updated.
            mark_dirty(scene::DirtyFlags::paint);
        }
    }

    void Text::apply_derived_style(text::TextStyle style) {
        validate_text_style(style);
        auto saved = capture_override_state();
        derived_style_ = std::move(style);
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::clear_derived_style() {
        if (!derived_style_.has_value()) {
            return;
        }
        auto saved = capture_override_state();
        derived_style_.reset();
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    auto Text::has_derived_style() const noexcept -> bool {
        return derived_style_.has_value();
    }

    auto Text::style() const -> const text::TextStyle& {
        return style_;
    }

    void Text::set_color(foundation::NanColor color) {
        style_.color = color;
        color_presentation_.set(std::move(color));
        color_explicit_ = true;
    }

    void Text::clear_color() {
        if (!color_explicit_) {
            return;
        }
        color_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_color_override() const noexcept -> bool {
        return color_explicit_;
    }

    auto Text::color() const -> foundation::NanColor {
        return style_.color;
    }

    void Text::set_font_size(const float size) {
        if (!std::isfinite(size) || size <= 0.0F) {
            throw std::invalid_argument("Text font size must be finite and positive");
        }
        const auto before = style_;
        font_size_explicit_ = true;
        auto next = style_;
        next.font_size = size;
        if (!line_height_explicit_) {
            text::TextStyle fallback;
            resolve_component_style_fallbacks(resolved_style_context(), fallback);
            next.line_height = fallback.line_height;
        }
        commit_reconciled_style(before, std::move(next), false, true);
    }

    void Text::clear_font_size() {
        if (!font_size_explicit_) {
            return;
        }
        font_size_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_font_size_override() const noexcept -> bool {
        return font_size_explicit_;
    }

    auto Text::font_size() const -> float {
        return style_.font_size;
    }

    void Text::set_line_height(const float factor) {
        if (!std::isfinite(factor) || factor < 0.0F) {
            throw std::invalid_argument("Text line height must be finite and non-negative");
        }
        if (factor == 0.0F) {
            clear_line_height();
            return;
        }
        const auto before = style_;
        line_height_explicit_ = true;
        if (style_.line_height == factor) {
            return;
        }
        auto next = style_;
        next.line_height = factor;
        commit_reconciled_style(before, std::move(next));
    }

    void Text::clear_line_height() {
        if (!line_height_explicit_) {
            return;
        }
        line_height_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_line_height_override() const noexcept -> bool {
        return line_height_explicit_;
    }

    auto Text::line_height() const -> float {
        return style_.line_height;
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

    void Text::set_font(theme::FontRequest request) {
        if (request.weight < 1 || request.weight > 1000) {
            throw std::invalid_argument("Text font weight must be between 1 and 1000");
        }
        const auto before = style_;
        auto saved = capture_override_state();
        font_family_explicit_ = true;
        font_weight_explicit_ = true;
        font_slant_explicit_ = true;
        if (style_.font == request) {
            return;
        }
        auto next = style_;
        next.font = std::move(request);
        try {
            commit_reconciled_style(before, std::move(next));
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::clear_font() {
        if (!has_font_override()) {
            return;
        }
        auto saved = capture_override_state();
        font_family_explicit_ = false;
        font_weight_explicit_ = false;
        font_slant_explicit_ = false;
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::set_font_family(resource::ResourceKey family) {
        const auto before = style_;
        auto saved = capture_override_state();
        font_family_explicit_ = true;
        if (style_.font.family == family) {
            return;
        }
        auto next = style_;
        next.font.family = std::move(family);
        try {
            commit_reconciled_style(before, std::move(next));
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::clear_font_family() {
        if (!font_family_explicit_) {
            return;
        }
        auto saved = capture_override_state();
        font_family_explicit_ = false;
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::set_font_weight(const int weight) {
        if (weight < 1 || weight > 1000) {
            throw std::invalid_argument("Text font weight must be between 1 and 1000");
        }
        const auto before = style_;
        auto saved = capture_override_state();
        font_weight_explicit_ = true;
        if (style_.font.weight == weight) {
            return;
        }
        auto next = style_;
        next.font.weight = weight;
        try {
            commit_reconciled_style(before, std::move(next));
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::clear_font_weight() {
        if (!font_weight_explicit_) {
            return;
        }
        auto saved = capture_override_state();
        font_weight_explicit_ = false;
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::set_font_slant(const theme::FontSlant slant) {
        const auto before = style_;
        auto saved = capture_override_state();
        font_slant_explicit_ = true;
        if (style_.font.slant == slant) {
            return;
        }
        auto next = style_;
        next.font.slant = slant;
        try {
            commit_reconciled_style(before, std::move(next));
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::clear_font_slant() {
        if (!font_slant_explicit_) {
            return;
        }
        auto saved = capture_override_state();
        font_slant_explicit_ = false;
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    auto Text::has_font_override() const noexcept -> bool {
        return font_family_explicit_ || font_weight_explicit_ || font_slant_explicit_;
    }

    auto Text::has_font_family_override() const noexcept -> bool {
        return font_family_explicit_;
    }

    auto Text::has_font_weight_override() const noexcept -> bool {
        return font_weight_explicit_;
    }

    auto Text::has_font_slant_override() const noexcept -> bool {
        return font_slant_explicit_;
    }

    auto Text::font() const -> const theme::FontRequest& {
        return style_.font;
    }

    void Text::clear_style_overrides() {
        if (!color_explicit_ && !font_size_explicit_ && !line_height_explicit_
            && !has_font_override())
        {
            return;
        }
        auto saved = capture_override_state();
        color_explicit_ = false;
        font_size_explicit_ = false;
        line_height_explicit_ = false;
        font_family_explicit_ = false;
        font_weight_explicit_ = false;
        font_slant_explicit_ = false;
        try {
            reconcile_style(resolved_style_context());
        }
        catch (...) {
            restore_override_state(std::move(saved));
            throw;
        }
    }

    void Text::set_overflow(const text::TextOverflow overflow) {
        overflow_explicit_ = true;
        if (style_.overflow == overflow) {
            return;
        }
        style_.overflow = overflow;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    void Text::clear_overflow() {
        if (!overflow_explicit_) {
            return;
        }
        overflow_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_overflow_override() const noexcept -> bool {
        return overflow_explicit_;
    }

    auto Text::overflow() const -> text::TextOverflow {
        return style_.overflow;
    }

    void Text::set_max_lines(const int lines) {
        const int clamped = std::max(1, lines);
        max_lines_explicit_ = true;
        if (style_.max_lines == clamped) {
            return;
        }
        style_.max_lines = clamped;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    void Text::clear_max_lines() {
        if (!max_lines_explicit_) {
            return;
        }
        max_lines_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_max_lines_override() const noexcept -> bool {
        return max_lines_explicit_;
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

    auto Text::layout_result() const -> const text::TextLayoutResult& {
        return layout_;
    }

    auto Text::line_count() const noexcept -> std::size_t {
        return layout_.lines.size();
    }

    auto Text::overflowed() const noexcept -> bool {
        return layout_.overflowed;
    }

    void Text::set_text_pipeline(text::TextPipeline pipeline) {
        if (pipeline.backend == nullptr) {
            throw std::invalid_argument("text::TextPipeline requires a layout backend");
        }
        backend_ = pipeline.backend;
        renderer_ = pipeline.renderer;
        pipeline_explicit_ = true;
        resolved_font_pipeline_.reset();
        // 后端不在缓存键里：换后端必须清，否则会复用上一个后端的排版结果。
        clear_metrics_cache();
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::text_pipeline() const -> text::TextPipeline {
        return {.backend = backend_, .renderer = renderer_};
    }

    void Text::apply_default_text_pipeline(const text::TextPipeline& pipeline) {
        if (pipeline_explicit_) {
            return;
        }
        const bool backend_changed = backend_ != pipeline.backend;
        backend_ = pipeline.backend;
        renderer_ = pipeline.renderer;
        if (backend_changed) {
            clear_metrics_cache();
        }
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    void Text::apply_font_context(text::FontPipelineCache& context) {
        auto* previous = font_context_;
        font_context_ = &context;
        try {
            resolve_font();
        }
        catch (...) {
            font_context_ = previous;
            throw;
        }
    }

    void Text::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        reconcile_style(context);
    }

    void Text::set_layout_backend(const text::ITextLayoutBackend& backend) {
        const bool backend_changed = backend_ != &backend;
        backend_ = &backend;
        pipeline_explicit_ = true;
        if (backend_changed) {
            clear_metrics_cache();
        }
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::layout_backend() const -> const text::ITextLayoutBackend& {
        return *backend_;
    }

    void Text::set_layout_renderer(text::ITextLayoutRenderer* renderer) {
        renderer_ = renderer;
        pipeline_explicit_ = true;
    }

    auto Text::layout_renderer() const -> text::ITextLayoutRenderer* {
        return renderer_;
    }

    void Text::set_align(const theme::TextAlign align) {
        align_explicit_ = true;
        if (style_.align == align) {
            return;
        }
        style_.align = align;
        // 对齐只改绘制：测量尺寸、换行结果、caret 坐标都不受影响，所以不必重排。
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Text::clear_align() {
        if (!align_explicit_) {
            return;
        }
        align_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_align_override() const noexcept -> bool {
        return align_explicit_;
    }

    auto Text::align() const -> theme::TextAlign {
        return style_.align;
    }

    void Text::set_vertical_align(const text::TextVerticalAlign align) {
        vertical_align_explicit_ = true;
        if (style_.vertical_align == align) {
            return;
        }
        style_.vertical_align = align;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Text::clear_vertical_align() {
        if (!vertical_align_explicit_) {
            return;
        }
        vertical_align_explicit_ = false;
        reconcile_style(resolved_style_context());
    }

    auto Text::has_vertical_align_override() const noexcept -> bool {
        return vertical_align_explicit_;
    }

    auto Text::vertical_align() const -> text::TextVerticalAlign {
        return style_.vertical_align;
    }

    void Text::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        // 无指派盒子时不施加对齐，包括多行中较短的行；仍用自然文本范围裁剪。
        draw_text_box(
            ctx,
            foundation::NanRect::from_xywh(
                position.get_x(),
                position.get_y(),
                ctx.logical_to_screen(measured_text_width()),
                ctx.logical_to_screen(measured_text_height())
            ),
            false
        );
    }

    void Text::draw_in(render::DrawContext& ctx, foundation::NanRect rect) {
        draw_text_box(ctx, rect, true);
    }

    void Text::draw_text_box(
        render::DrawContext& ctx,
        const foundation::NanRect rect,
        const bool align_to_box
    ) {
        const auto& color = *color_presentation_.value();
        if (layout_.lines.empty() || color.alpha() <= 0.0F) {
            return;
        }

        // The assigned rect is already in screen units. Shaped metrics/origin_x stay
        // logical; convert the available box once, then scale only at the draw boundary.
        const float scale = ctx.scale().logical_to_screen;
        for (auto& line: layout_.lines) {
            line.origin_x = text::text_align_offset(
                align_to_box ? style_.align : theme::TextAlign::start,
                rect.get_width() / scale,
                line.size.get_width()
            );
        }
        const float vertical_slack =
            std::max(0.0F, rect.get_height() - ctx.logical_to_screen(measured_text_height()));
        const float offset_y = !align_to_box                           ? 0.0F
            : style_.vertical_align == text::TextVerticalAlign::center ? vertical_slack * 0.5F
            : style_.vertical_align == text::TextVerticalAlign::bottom ? vertical_slack
                                                                       : 0.0F;

        auto clip = style_.overflow == text::TextOverflow::clip
            ? ctx.clip().push(
                  foundation::NanRect::from_xywh(
                      rect.get_left(),
                      rect.get_top(),
                      // 测量 advance 之外允许字形墨迹悬垂 1~2px，否则最后一个字形的
                      // 右侧墨迹会被裁掉（见 text::glyph_overhang_allowance）。
                      rect.get_width()
                          + ctx.logical_to_screen(
                              text::glyph_overhang_allowance(layout_.font_size)
                          ),
                      rect.get_height()
                  )
              )
            : render::ClipStack::Guard {nullptr, false};

        const auto draw_color = color.with_alpha(color.alpha() * ctx.opacity());
        const auto position = foundation::NanPoint(rect.get_left(), rect.get_top() + offset_y);
        if (renderer_ != nullptr) {
            renderer_->draw(layout_, ctx, position, draw_color);
            return;
        }

        float y = position.get_y();
        for (const auto& line: layout_.lines) {
            if (!line.visible_text.empty()) {
                ctx.device().draw_text(
                    line.visible_text,
                    foundation::NanPoint(
                        position.get_x() + ctx.logical_to_screen(line.origin_x),
                        y
                    ),
                    ctx.logical_to_screen(layout_.font_size),
                    draw_color
                );
            }
            y += ctx.logical_to_screen(line.size.get_height());
        }
    }

    void Text::on_draw(render::DrawContext& ctx) {
        // 用被指派的矩形而不是自己的原点：这样布局中的 Text（例如被拉伸到容器宽度）
        // 直接获得 style().align 的效果，调用方不需要再算居中/靠右的偏移。
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        draw_in(ctx, world);
    }

    auto Text::on_measure(foundation::NanLayoutConstraints constraints) -> foundation::NanSize {
        update_metrics(constraints);
        return size();
    }

    auto Text::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::static_text,
            .label = std::string(text()),
        };
    }

    void Text::resolve_component_style_fallbacks(
        const theme::ResolvedStyleContext& context,
        text::TextStyle& fallback
    ) const {
        fallback = derived_style_.value_or(text::TextStyle {});
        if (context.text_color_from_context || !derived_style_.has_value()) {
            fallback.color = context.text_color;
        }
        if (context.font_size_from_context || !derived_style_.has_value()) {
            fallback.font_size = context.font_size;
        }
        if (context.font_from_context || !derived_style_.has_value()) {
            fallback.font = context.font;
        }
    }

    auto Text::capture_override_state() const -> OverrideStateSnapshot {
        return {
            .derived_style = derived_style_,
            .color = color_explicit_,
            .font_size = font_size_explicit_,
            .line_height = line_height_explicit_,
            .font_family = font_family_explicit_,
            .font_weight = font_weight_explicit_,
            .font_slant = font_slant_explicit_,
            .overflow = overflow_explicit_,
            .max_lines = max_lines_explicit_,
            .align = align_explicit_,
            .vertical_align = vertical_align_explicit_,
        };
    }

    void Text::restore_override_state(OverrideStateSnapshot snapshot) noexcept {
        derived_style_ = std::move(snapshot.derived_style);
        color_explicit_ = snapshot.color;
        font_size_explicit_ = snapshot.font_size;
        line_height_explicit_ = snapshot.line_height;
        font_family_explicit_ = snapshot.font_family;
        font_weight_explicit_ = snapshot.font_weight;
        font_slant_explicit_ = snapshot.font_slant;
        overflow_explicit_ = snapshot.overflow;
        max_lines_explicit_ = snapshot.max_lines;
        align_explicit_ = snapshot.align;
        vertical_align_explicit_ = snapshot.vertical_align;
    }

    void Text::reconcile_style(const theme::ResolvedStyleContext& context) {
        const auto before = style_;
        text::TextStyle fallback;
        resolve_component_style_fallbacks(context, fallback);

        auto next = style_;
        if (!color_explicit_) {
            next.color = fallback.color;
        }
        if (!font_size_explicit_) {
            next.font_size = fallback.font_size;
        }
        if (!line_height_explicit_) {
            next.line_height = fallback.line_height;
        }
        if (!font_family_explicit_) {
            next.font.family = fallback.font.family;
        }
        if (!font_weight_explicit_) {
            next.font.weight = fallback.font.weight;
        }
        if (!font_slant_explicit_) {
            next.font.slant = fallback.font.slant;
        }
        if (!overflow_explicit_) {
            next.overflow = fallback.overflow;
        }
        if (!max_lines_explicit_) {
            next.max_lines = fallback.max_lines;
        }
        if (!align_explicit_) {
            next.align = fallback.align;
        }
        if (!vertical_align_explicit_) {
            next.vertical_align = fallback.vertical_align;
        }
        commit_reconciled_style(before, std::move(next));
    }

    void Text::commit_reconciled_style(
        const text::TextStyle& before,
        text::TextStyle next,
        const bool force_color_presentation,
        const bool force_font_size_presentation
    ) {
        const bool color_changed = !(before.color == next.color);
        const bool font_size_changed = before.font_size != next.font_size;
        const bool font_changed = before.font != next.font;
        const bool metrics_changed = !same_layout_style(before, next);
        const bool paint_only_changed = color_changed || before.align != next.align
            || before.vertical_align != next.vertical_align;

        std::shared_ptr<text::FontPipeline> next_font_pipeline;
        if (font_changed && !pipeline_explicit_ && font_context_ != nullptr) {
            auto pipeline = font_context_->get(next.font);
            if (!pipeline) {
                throw std::runtime_error("Text cannot resolve font: " + pipeline.error().message);
            }
            next_font_pipeline = std::move(*pipeline);
        }

        const float before_presentation_font_size = *font_size_presentation_.value();
        style_ = std::move(next);
        if (force_color_presentation || color_changed
            || (color_presentation_.target() != nullptr
                && !(*color_presentation_.target() == style_.color)))
        {
            color_presentation_.set(style_.color);
        }
        if (force_font_size_presentation || font_size_changed
            || (font_size_presentation_.target() != nullptr
                && *font_size_presentation_.target() != style_.font_size))
        {
            font_size_presentation_.set(style_.font_size);
        }
        if (next_font_pipeline != nullptr) {
            resolved_font_pipeline_ = std::move(next_font_pipeline);
            const auto resolved = resolved_font_pipeline_->pipeline();
            backend_ = resolved.backend;
            renderer_ = resolved.renderer;
            clear_metrics_cache();
        }
        if (metrics_changed || font_changed
            || *font_size_presentation_.value() != before_presentation_font_size)
        {
            mark_layout_dirty();
            update_metrics(last_layout_constraints());
        }
        else if (paint_only_changed) {
            mark_dirty(scene::DirtyFlags::paint);
        }
    }

    void Text::clear_explicit_color() noexcept {
        color_explicit_ = false;
    }

    void Text::clear_explicit_line_height() noexcept {
        line_height_explicit_ = false;
    }

    auto Text::font_size_is_explicit() const noexcept -> bool {
        return font_size_explicit_;
    }

    auto Text::find_metrics_cache(const text::TextLayoutInput& input) const
        -> std::optional<std::size_t> {
        std::optional<std::size_t> width_invariant_match;
        for (std::size_t index = 0; index < metrics_cache_used_; ++index) {
            const auto& entry = metrics_cache_[index];
            if (entry.text != input.text || !same_layout_style(entry.style, input.style)) {
                continue;
            }
            if (same_layout_constraints(entry.constraints, input.constraints)) {
                return index;
            }
            // 后端整形与断行只依赖 max_width，不依赖 min_width/min_height/max_height；
            // 当文本未换行且自然宽 + 1 px ≤ 新 max_width，或新 max_width 仍落在后端证明等价的
            // 折行宽度区间 [wrap_width_floor, wrap_width_ceiling) 内时，断行与字形结果完全不变。
            const bool same_max_w = entry.constraints.max_width == input.constraints.max_width;
            const bool fits_unwrapped = entry.unwrapped
                && (!std::isfinite(input.constraints.max_width)
                    || entry.natural_width + 1.0F <= input.constraints.max_width);
            const bool same_wrapped_lines = entry.result.has_wrap_width_range()
                && std::isfinite(input.constraints.max_width)
                && input.constraints.max_width >= entry.result.wrap_width_floor
                && input.constraints.max_width < entry.result.wrap_width_ceiling;
            if (same_max_w || fits_unwrapped || same_wrapped_lines) {
                if (!width_invariant_match.has_value() || index == active_cache_slot_) {
                    width_invariant_match = index;
                }
            }
        }
        return width_invariant_match;
    }

    auto Text::metrics_entry_bytes(std::string_view text, const text::TextLayoutResult& result)
        -> std::size_t {
        std::size_t bytes = text.size() + sizeof(text::TextStyle);
        for (const auto& line: result.lines) {
            bytes += line.visible_text.size()
                + line.glyphs.size() * sizeof(text::TextLayoutLine::Glyph)
                + line.caret_stops.size() * sizeof(text::TextCaretStop);
        }
        return bytes;
    }

    auto
    Text::store_metrics_cache(const text::TextLayoutInput& input, text::TextLayoutResult result)
        -> std::size_t {
        float natural_width = 0.0F;
        float natural_height = 0.0F;
        for (const auto& line: result.lines) {
            natural_width = std::max(natural_width, line.size.get_width());
            natural_height += line.size.get_height();
        }
        const auto newline_count = static_cast<std::size_t>(std::ranges::count(input.text, '\n'));
        const bool unwrapped = !result.overflowed && result.lines.size() == newline_count + 1
            && (!std::isfinite(input.constraints.max_width)
                || natural_width + 1.0F <= input.constraints.max_width);
        const std::uint64_t generation = ++metrics_cache_generation_;

        const auto bytes = metrics_entry_bytes(input.text, result);
        if (bytes > kMetricsCacheByteBudget) {
            // 大结果：只留这一条，避免 8 份长文本排版结果同时驻留。
            clear_metrics_cache();
            auto& entry = metrics_cache_[0];
            entry.text = std::string(input.text);
            entry.style = input.style;
            entry.constraints = input.constraints;
            entry.result = std::move(result);
            entry.natural_width = natural_width;
            entry.natural_height = natural_height;
            entry.unwrapped = unwrapped;
            entry.generation = generation;
            metrics_cache_used_ = 1;
            metrics_cache_next_ = 1 % kMetricsCacheEntries;
            metrics_cache_bytes_ = bytes;
            return 0;
        }

        const std::size_t slot = metrics_cache_next_;
        auto& entry = metrics_cache_[slot];
        metrics_cache_bytes_ -=
            std::min(metrics_cache_bytes_, metrics_entry_bytes(entry.text, entry.result));
        entry.text = std::string(input.text);
        entry.style = input.style;
        entry.constraints = input.constraints;
        entry.result = std::move(result);
        entry.natural_width = natural_width;
        entry.natural_height = natural_height;
        entry.unwrapped = unwrapped;
        entry.generation = generation;
        metrics_cache_bytes_ += bytes;
        metrics_cache_next_ = (metrics_cache_next_ + 1) % kMetricsCacheEntries;
        metrics_cache_used_ = std::min(metrics_cache_used_ + 1, kMetricsCacheEntries);
        return slot;
    }

    void Text::clear_metrics_cache() noexcept {
        for (auto& entry: metrics_cache_) {
            entry.text.clear();
            entry.result = {};
            entry.natural_width = 0.0F;
            entry.natural_height = 0.0F;
            entry.unwrapped = false;
            entry.generation = 0;
        }
        metrics_cache_used_ = 0;
        metrics_cache_next_ = 0;
        metrics_cache_bytes_ = 0;
        active_cache_slot_ = kMetricsCacheEntries;
        active_cache_generation_ = 0;
    }

    auto Text::metrics_cache_hits() const noexcept -> long {
        return metrics_cache_hits_;
    }

    auto Text::metrics_cache_misses() const noexcept -> long {
        return metrics_cache_misses_;
    }

    void Text::update_metrics(foundation::NanLayoutConstraints constraints) {
        auto presentation_style = style_;
        presentation_style.font_size = *font_size_presentation_.value();
        const text::TextLayoutInput input {
            .text = text_.get(),
            .style = std::move(presentation_style),
            .constraints = constraints,
        };
        if (const auto hit_index = find_metrics_cache(input); hit_index.has_value()) {
            ++metrics_cache_hits_;
            const auto& entry = metrics_cache_[*hit_index];
            // 当前 layout_ 若已持有同一缓存槽位的结果，直接更新尺寸即可，避免重复拷贝 glyph/caret 数组。
            if (active_cache_slot_ != *hit_index || active_cache_generation_ != entry.generation) {
                layout_ = entry.result;
                active_cache_slot_ = *hit_index;
                active_cache_generation_ = entry.generation;
            }
            if (same_layout_constraints(entry.constraints, input.constraints)) {
                layout_.size = entry.result.size;
            }
            else {
                layout_.size = constraints.constrain(
                    foundation::NanSize(entry.natural_width, entry.natural_height)
                );
            }
        }
        else {
            ++metrics_cache_misses_;
            auto result = backend_->layout(input);
            layout_ = result;
            const std::size_t slot = store_metrics_cache(input, std::move(result));
            active_cache_slot_ = slot;
            active_cache_generation_ = metrics_cache_[slot].generation;
        }
        // 节点尺寸必须等于本次 measure 契约报告的值：整形结果的原始尺寸要**先收敛到约束内**
        // 再写入。否则在紧约束（fill / percent 宽度）配 wrap 的情况下，size_ 记录的是最长行的
        // 内容宽度、而 measured_size_ 记录的是可用宽度；同一布局周期里 arrange 又会把 size_
        // 写回可用宽度，于是每个周期产生两次尺寸跳变，每次都通过 set_size() 把祖先链标脏 ——
        // 父级在 layout_to() 里刚清完标记就又被标脏，布局永不收敛，整页文本每帧重新整形。
        // 同层既有约定见 EditableText::on_measure 与 TextField::on_measure：先收敛再写入。
        // 原始内容尺寸仍可经 layout_result() / measured_text_width() 查询。
        set_size(constraints.constrain(layout_.size));
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
        // 从 fallback 后端切到真实字体后端：清缓存（后端不在键里）。
        clear_metrics_cache();
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

} // namespace nandina::widget::primitives
