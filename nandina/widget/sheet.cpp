//
// widget/sheet - edge-docked modal overlay drawer (scrim + docked panel + focus trap).
//

#include "sheet.hpp"

#include "../render/draw_context.hpp"
#include "../scene/animation_host.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"
#include "internal/dismiss_layer.hpp"
#include "internal/focus_scope.hpp"
#include "internal/text_style_bridge.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        constexpr float kHeaderTextGap = 4.0F;
        constexpr int kDescriptionMaxLines = 4;

        [[nodiscard]] constexpr auto valid_sheet_side(const SheetSide side) noexcept -> bool {
            switch (side) {
                case SheetSide::top:
                case SheetSide::right:
                case SheetSide::bottom:
                case SheetSide::left:
                    return true;
            }
            return false;
        }

        void require_valid_side(const SheetSide side) {
            if (!valid_sheet_side(side)) {
                throw std::invalid_argument("Sheet: side is invalid");
            }
        }

        [[nodiscard]] auto fade_behavior(const theme::DesignSystem& system, const bool entering)
            -> motion::Behavior<float> {
            return motion::Behavior<float>(
                system.tokens.motion.long_duration,
                entering ? motion::Easing::ease_out : motion::Easing::ease_in
            );
        }

        [[nodiscard]] auto scrim_style(const theme::ResolvedDialogStyle& style)
            -> theme::ResolvedBoxStyle {
            return theme::ResolvedBoxStyle {
                .fill = style.scrim,
                .border = style.scrim,
                .border_width = 0.0F,
                .radius = 0.0F,
            };
        }
    } // namespace

    namespace internal
    {
        class SheetPanel final: public scene::NanControl {
        public:
            SheetPanel() {
                style_ = theme::resolve_dialog(
                    *theme::default_design_system_shared(),
                    theme::ColorAppearance::light
                );
                style_.panel.radius = 0.0F;
            }

            void set_side(const SheetSide side) noexcept {
                side_ = side;
                mark_layout_dirty();
            }

            [[nodiscard]] auto side() const noexcept -> SheetSide {
                return side_;
            }

            void set_explicit_extent(const std::optional<float> extent) noexcept {
                explicit_extent_ = extent;
                mark_layout_dirty();
            }

            auto
            set_header(const scene::NanControl& owner, std::shared_ptr<scene::NanControl> header)
                -> scene::NanControl& {
                const auto current = custom_header_.lock();
                if (current != nullptr && current == header) {
                    return *current;
                }
                validate_slot_node(owner, header, "set_header");
                detach_builtin_header();
                if (current != nullptr) {
                    remove_and_delete(*current);
                }
                custom_header_ = header;
                add_child(std::move(header));
                mark_layout_dirty();
                return *custom_header_.lock();
            }

            auto
            set_content(const scene::NanControl& owner, std::shared_ptr<scene::NanControl> content)
                -> scene::NanControl& {
                const auto current = content_.lock();
                if (current != nullptr && current == content) {
                    return *current;
                }
                validate_slot_node(owner, content, "set_content");
                if (current != nullptr) {
                    remove_and_delete(*current);
                }
                content_ = content;
                add_child(std::move(content));
                mark_layout_dirty();
                return *content_.lock();
            }

            auto
            set_footer(const scene::NanControl& owner, std::shared_ptr<scene::NanControl> footer)
                -> scene::NanControl& {
                const auto current = footer_.lock();
                if (current != nullptr && current == footer) {
                    return *current;
                }
                validate_slot_node(owner, footer, "set_footer");
                if (current != nullptr) {
                    remove_and_delete(*current);
                }
                footer_ = footer;
                add_child(std::move(footer));
                mark_layout_dirty();
                return *footer_.lock();
            }

            void set_title(std::string title) {
                if (auto current = custom_header_.lock()) {
                    remove_and_delete(*current);
                    custom_header_.reset();
                }
                title_ = std::move(title);
                sync_builtin_header();
                mark_layout_dirty();
                mark_semantics_dirty();
            }

            [[nodiscard]] auto title() const noexcept -> std::string_view {
                return title_;
            }

            void set_description(std::string description) {
                if (auto current = custom_header_.lock()) {
                    remove_and_delete(*current);
                    custom_header_.reset();
                }
                description_ = std::move(description);
                sync_builtin_header();
                mark_layout_dirty();
                mark_semantics_dirty();
            }

            [[nodiscard]] auto description() const noexcept -> std::string_view {
                return description_;
            }

            [[nodiscard]] auto header() const -> scene::NanControl* {
                if (auto custom = custom_header_.lock()) {
                    return custom.get();
                }
                if (!title_.empty() && title_text_ != nullptr) {
                    return title_text_.get();
                }
                if (!description_.empty() && description_text_ != nullptr) {
                    return description_text_.get();
                }
                return nullptr;
            }

            [[nodiscard]] auto content() const -> scene::NanControl* {
                return content_.lock().get();
            }

            [[nodiscard]] auto footer() const -> scene::NanControl* {
                return footer_.lock().get();
            }

            void set_style(theme::ResolvedDialogStyle style) {
                style_ = std::move(style);
                apply_text_styles();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
            }

            [[nodiscard]] auto style() const noexcept -> const theme::ResolvedDialogStyle& {
                return style_;
            }

            void set_text_pipeline(text::TextPipeline pipeline) {
                pipeline_ = pipeline;
                has_explicit_pipeline_ = true;
                if (title_text_ != nullptr) {
                    title_text_->set_text_pipeline(pipeline);
                }
                if (description_text_ != nullptr) {
                    description_text_->set_text_pipeline(std::move(pipeline));
                }
                mark_layout_dirty();
            }

            void apply_default_text_pipeline(const text::TextPipeline& pipeline) override {
                if (title_text_ != nullptr) {
                    title_text_->apply_default_text_pipeline(pipeline);
                }
                if (description_text_ != nullptr) {
                    description_text_->apply_default_text_pipeline(pipeline);
                }
                mark_layout_dirty();
            }

            void apply_font_context(text::FontPipelineCache& context) override {
                if (title_text_ != nullptr) {
                    title_text_->apply_font_context(context);
                }
                if (description_text_ != nullptr) {
                    description_text_->apply_font_context(context);
                }
                mark_layout_dirty();
            }

            void on_draw(render::DrawContext& context) override {
                apply_text_styles();
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                primitives::BoxPainter::paint(context, world, style_.panel, context.opacity());
            }

        protected:
            [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
                -> foundation::NanSize override {
                apply_text_styles();
                const float pad_x = std::max(0.0F, style_.metrics.padding_x);
                const float pad_y = std::max(0.0F, style_.metrics.padding_y);
                const float gap = std::max(0.0F, style_.metrics.gap);
                const float configured_extent = std::max(0.0F, style_.metrics.panel_width);

                const bool horizontal_dock = side_ == SheetSide::left || side_ == SheetSide::right;
                const float panel_width = horizontal_dock
                    ? (std::isfinite(constraints.max_width)
                           ? std::min(configured_extent, std::max(0.0F, constraints.max_width))
                           : configured_extent)
                    : (std::isfinite(constraints.max_width) ? std::max(0.0F, constraints.max_width)
                                                            : configured_extent);

                const float inner_w = std::max(0.0F, panel_width - pad_x * 2.0F);
                const float max_inner_h = std::isfinite(constraints.max_height)
                    ? std::max(0.0F, constraints.max_height - pad_y * 2.0F)
                    : std::numeric_limits<float>::infinity();

                const auto header_size = measure_header(inner_w, max_inner_h);
                const bool has_header = header_size.has_value();
                const float header_h = has_header ? header_size->get_height() : 0.0F;

                float footer_h = 0.0F;
                const auto footer_node = footer_.lock();
                const bool has_footer = footer_node != nullptr;
                if (has_footer) {
                    footer_h = footer_node
                                   ->measure_layout(
                                       foundation::NanLayoutConstraints {
                                           .min_width = 0.0F,
                                           .max_width = inner_w,
                                           .min_height = 0.0F,
                                           .max_height = max_inner_h,
                                       }
                                   )
                                   .get_height();
                }

                float content_h = 0.0F;
                const auto content_node = content_.lock();
                const bool has_content = content_node != nullptr;
                if (has_content) {
                    float avail_content_h = max_inner_h;
                    if (std::isfinite(avail_content_h)) {
                        if (has_header) {
                            avail_content_h = std::max(0.0F, avail_content_h - header_h - gap);
                        }
                        if (has_footer) {
                            avail_content_h = std::max(0.0F, avail_content_h - footer_h - gap);
                        }
                    }
                    content_h = content_node
                                    ->measure_layout(
                                        foundation::NanLayoutConstraints {
                                            .min_width = 0.0F,
                                            .max_width = inner_w,
                                            .min_height = 0.0F,
                                            .max_height = avail_content_h,
                                        }
                                    )
                                    .get_height();
                }

                float natural_h = pad_y * 2.0F;
                int sections = 0;
                if (has_header) {
                    natural_h += header_h;
                    ++sections;
                }
                if (has_content) {
                    natural_h += content_h;
                    ++sections;
                }
                if (has_footer) {
                    natural_h += footer_h;
                    ++sections;
                }
                if (sections > 1) {
                    natural_h += gap * static_cast<float>(sections - 1);
                }
                natural_h = std::max(natural_h, std::max(0.0F, style_.metrics.min_height));

                float panel_height = natural_h;
                if (horizontal_dock) {
                    if (std::isfinite(constraints.max_height)) {
                        panel_height = std::max(0.0F, constraints.max_height);
                    }
                }
                else {
                    const float desired_h =
                        explicit_extent_.has_value() ? *explicit_extent_ : natural_h;
                    panel_height = std::isfinite(constraints.max_height)
                        ? std::min(desired_h, std::max(0.0F, constraints.max_height))
                        : desired_h;
                }

                return constraints.constrain(foundation::NanSize(panel_width, panel_height));
            }

            void on_layout() override {
                apply_text_styles();
                const float pad_x = std::max(0.0F, style_.metrics.padding_x);
                const float pad_y = std::max(0.0F, style_.metrics.padding_y);
                const float gap = std::max(0.0F, style_.metrics.gap);
                const float inner_w = std::max(0.0F, width() - pad_x * 2.0F);
                const float inner_h = std::max(0.0F, height() - pad_y * 2.0F);

                bool has_header = false;
                float header_h = 0.0F;
                if (auto custom = custom_header_.lock()) {
                    const auto measured = custom->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = inner_w,
                            .min_height = 0.0F,
                            .max_height = inner_h,
                        }
                    );
                    custom->layout_to(
                        foundation::NanRect::from_xywh(
                            pad_x,
                            pad_y,
                            measured.get_width(),
                            measured.get_height()
                        )
                    );
                    has_header = true;
                    header_h = measured.get_height();
                }
                else if (!title_.empty() || !description_.empty()) {
                    float y = pad_y;
                    if (!title_.empty() && title_text_ != nullptr) {
                        const auto measured = title_text_->measure_layout(
                            foundation::NanLayoutConstraints {
                                .min_width = 0.0F,
                                .max_width = inner_w,
                                .min_height = 0.0F,
                                .max_height = inner_h,
                            }
                        );
                        title_text_->layout_to(
                            foundation::NanRect::from_xywh(
                                pad_x,
                                y,
                                measured.get_width(),
                                measured.get_height()
                            )
                        );
                        y += measured.get_height();
                    }
                    if (!description_.empty() && description_text_ != nullptr) {
                        if (!title_.empty()) {
                            y += kHeaderTextGap;
                        }
                        const auto measured = description_text_->measure_layout(
                            foundation::NanLayoutConstraints {
                                .min_width = 0.0F,
                                .max_width = inner_w,
                                .min_height = 0.0F,
                                .max_height = std::max(0.0F, height() - pad_y - y),
                            }
                        );
                        description_text_->layout_to(
                            foundation::NanRect::from_xywh(
                                pad_x,
                                y,
                                measured.get_width(),
                                measured.get_height()
                            )
                        );
                        y += measured.get_height();
                    }
                    has_header = true;
                    header_h = y - pad_y;
                }

                const auto footer_node = footer_.lock();
                const bool has_footer = footer_node != nullptr;
                foundation::NanSize footer_measured {};
                if (has_footer) {
                    footer_measured = footer_node->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = inner_w,
                            .min_height = 0.0F,
                            .max_height = inner_h,
                        }
                    );
                }

                const float content_top = pad_y + (has_header ? header_h + gap : 0.0F);
                const float footer_pinned_y = std::max(
                    content_top,
                    height() - pad_y - (has_footer ? footer_measured.get_height() : 0.0F)
                );

                const auto content_node = content_.lock();
                float after_content_y = content_top;
                if (content_node != nullptr) {
                    const float avail_content_h =
                        std::max(0.0F, footer_pinned_y - content_top - (has_footer ? gap : 0.0F));
                    const auto content_measured = content_node->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = inner_w,
                            .min_height = 0.0F,
                            .max_height = avail_content_h,
                        }
                    );
                    content_node->layout_to(
                        foundation::NanRect::from_xywh(
                            pad_x,
                            content_top,
                            content_measured.get_width(),
                            content_measured.get_height()
                        )
                    );
                    after_content_y =
                        content_top + content_measured.get_height() + (has_footer ? gap : 0.0F);
                }

                if (has_footer) {
                    const float footer_y = std::max(after_content_y, footer_pinned_y);
                    footer_node->layout_to(
                        foundation::NanRect::from_xywh(
                            pad_x,
                            footer_y,
                            footer_measured.get_width(),
                            footer_measured.get_height()
                        )
                    );
                }
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                return {
                    .role = semantics::Role::dialog,
                    .label = title_,
                };
            }

        private:
            void validate_slot_node(
                const scene::NanControl& owner,
                const std::shared_ptr<scene::NanControl>& next,
                const char* what
            ) const {
                const std::string label = std::string("Sheet::") + what;
                if (!next) {
                    throw std::invalid_argument(label + ": node is null");
                }
                if (next.get() == &owner || next->is_ancestor_of(owner) || next.get() == this
                    || next->is_ancestor_of(*this))
                {
                    throw std::invalid_argument(label + ": node cannot contain its owning Sheet");
                }
                if (slot_in_use(next.get())) {
                    throw std::invalid_argument(label + ": node already belongs to a Sheet slot");
                }
                if (next->parent() != nullptr) {
                    throw std::logic_error(label + ": node must be detached");
                }
            }

            [[nodiscard]] auto slot_in_use(const scene::NanControl* node) const noexcept -> bool {
                const auto matches = [node](const std::weak_ptr<scene::NanControl>& slot) {
                    return slot.lock().get() == node;
                };
                return matches(custom_header_) || matches(content_) || matches(footer_)
                    || title_text_.get() == node || description_text_.get() == node;
            }

            void detach_builtin_header() {
                if (title_text_ != nullptr && title_text_->parent() == this) {
                    remove_and_delete(*title_text_);
                }
                if (description_text_ != nullptr && description_text_->parent() == this) {
                    remove_and_delete(*description_text_);
                }
            }

            void sync_builtin_header() {
                if (!title_.empty()) {
                    if (title_text_ == nullptr) {
                        title_text_ = std::make_shared<primitives::Text>("");
                        if (has_explicit_pipeline_) {
                            title_text_->set_text_pipeline(pipeline_);
                        }
                    }
                    title_text_->set_text(title_);
                    if (title_text_->parent() == nullptr) {
                        add_child(title_text_);
                    }
                }
                else if (title_text_ != nullptr && title_text_->parent() == this) {
                    remove_and_delete(*title_text_);
                }

                if (!description_.empty()) {
                    if (description_text_ == nullptr) {
                        description_text_ = std::make_shared<primitives::Text>("");
                        if (has_explicit_pipeline_) {
                            description_text_->set_text_pipeline(pipeline_);
                        }
                    }
                    description_text_->set_text(description_);
                    if (description_text_->parent() == nullptr) {
                        add_child(description_text_);
                    }
                }
                else if (description_text_ != nullptr && description_text_->parent() == this) {
                    remove_and_delete(*description_text_);
                }
                apply_text_styles();
            }

            [[nodiscard]] auto measure_header(const float inner_w, const float max_inner_h)
                -> std::optional<foundation::NanSize> {
                if (auto custom = custom_header_.lock()) {
                    return custom->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = inner_w,
                            .min_height = 0.0F,
                            .max_height = max_inner_h,
                        }
                    );
                }
                if (title_.empty() && description_.empty()) {
                    return std::nullopt;
                }
                float width = 0.0F;
                float height = 0.0F;
                if (!title_.empty() && title_text_ != nullptr) {
                    const auto m = title_text_->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = inner_w,
                            .min_height = 0.0F,
                            .max_height = max_inner_h,
                        }
                    );
                    width = std::max(width, m.get_width());
                    height += m.get_height();
                }
                if (!description_.empty() && description_text_ != nullptr) {
                    if (!title_.empty()) {
                        height += kHeaderTextGap;
                    }
                    const auto m = description_text_->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = inner_w,
                            .min_height = 0.0F,
                            .max_height = max_inner_h,
                        }
                    );
                    width = std::max(width, m.get_width());
                    height += m.get_height();
                }
                return foundation::NanSize(width, height);
            }

            void apply_text_styles() {
                const auto& context = resolved_style_context();
                if (title_text_ != nullptr) {
                    text::TextStyle title_style {
                        .color = context.text_color_from_context ? context.text_color
                                                                 : style_.title.color,
                        .font_size = context.font_size_from_context ? context.font_size
                                                                    : style_.title.font_size,
                        .font = context.font_from_context ? context.font : title_text_->font(),
                        .overflow = text::TextOverflow::ellipsis,
                        .max_lines = 1,
                    };
                    apply_type_metrics(title_style, style_.title, context.font_from_context);
                    if (!title_text_->style().approx_equals(title_style)) {
                        title_text_->set_style(title_style);
                        mark_layout_dirty();
                    }
                }
                if (description_text_ != nullptr) {
                    text::TextStyle desc_style {
                        .color = context.text_color_from_context ? context.text_color
                                                                 : style_.description.color,
                        .font_size = context.font_size_from_context ? context.font_size
                                                                    : style_.description.font_size,
                        .font =
                            context.font_from_context ? context.font : description_text_->font(),
                        .overflow = text::TextOverflow::wrap,
                        .max_lines = kDescriptionMaxLines,
                    };
                    apply_type_metrics(desc_style, style_.description, context.font_from_context);
                    if (!description_text_->style().approx_equals(desc_style)) {
                        description_text_->set_style(desc_style);
                        mark_layout_dirty();
                    }
                }
            }

            SheetSide side_ = SheetSide::right;
            std::optional<float> explicit_extent_;
            std::string title_;
            std::string description_;
            std::shared_ptr<primitives::Text> title_text_;
            std::shared_ptr<primitives::Text> description_text_;
            std::weak_ptr<scene::NanControl> custom_header_;
            std::weak_ptr<scene::NanControl> content_;
            std::weak_ptr<scene::NanControl> footer_;
            theme::ResolvedDialogStyle style_;
            text::TextPipeline pipeline_;
            bool has_explicit_pipeline_ = false;
        };
    } // namespace internal

    Sheet::Sheet(const SheetSide side, theme::NanTheme theme): side_(side) {
        require_valid_side(side);
        system_ = theme::design_system_shared_from_theme(theme);
        theme_view_ = theme;

        panel_ = std::make_shared<internal::SheetPanel>();
        panel_->set_side(side_);
        focus_scope_ = std::make_shared<internal::FocusScope>();
        focus_scope_->set_content(panel_);
        dismiss_layer_ = std::make_shared<internal::DismissLayer>();
        dismiss_layer_->set_content(focus_scope_);
        dismiss_layer_->set_visible(false);
        install_origin_resolver();

        apply_style();
        set_visible(false);
    }

    Sheet::Sheet(std::string title, const SheetSide side, theme::NanTheme theme):
        Sheet(side, std::move(theme)) {
        if (!title.empty()) {
            set_title(std::move(title));
        }
    }

    Sheet::~Sheet() = default;

    auto Sheet::create(const SheetSide side, theme::NanTheme theme) -> std::shared_ptr<Sheet> {
        return std::make_shared<Sheet>(side, std::move(theme));
    }

    auto Sheet::create(std::string title, const SheetSide side, theme::NanTheme theme)
        -> std::shared_ptr<Sheet> {
        return std::make_shared<Sheet>(std::move(title), side, std::move(theme));
    }

    void Sheet::set_side(const SheetSide side) {
        require_valid_side(side);
        if (side_ == side) {
            return;
        }
        side_ = side;
        panel_->set_side(side);
        dismiss_layer_->mark_layout_dirty();
        mark_layout_dirty();
    }

    auto Sheet::side() const noexcept -> SheetSide {
        return side_;
    }

    void Sheet::set_extent(const float extent) {
        if (!std::isfinite(extent) || extent <= 0.0F) {
            throw std::invalid_argument("Sheet: extent must be finite and positive");
        }
        if (extent_.has_value() && *extent_ == extent) {
            return;
        }
        extent_ = extent;
        panel_->set_explicit_extent(extent_);
        apply_style();
        dismiss_layer_->mark_layout_dirty();
        mark_layout_dirty();
    }

    void Sheet::clear_extent() noexcept {
        if (!extent_.has_value()) {
            return;
        }
        extent_.reset();
        panel_->set_explicit_extent(std::nullopt);
        apply_style();
        dismiss_layer_->mark_layout_dirty();
        mark_layout_dirty();
    }

    auto Sheet::has_explicit_extent() const noexcept -> bool {
        return extent_.has_value();
    }

    auto Sheet::extent() const -> float {
        return resolved_style().metrics.panel_width;
    }

    void Sheet::set_title(std::string title) {
        panel_->set_title(std::move(title));
        mark_semantics_dirty();
    }

    auto Sheet::title() const noexcept -> std::string_view {
        return panel_->title();
    }

    void Sheet::set_description(std::string description) {
        panel_->set_description(std::move(description));
        mark_layout_dirty();
    }

    auto Sheet::description() const noexcept -> std::string_view {
        return panel_->description();
    }

    auto Sheet::set_header(std::shared_ptr<scene::NanControl> header) -> Sheet& {
        (void)panel_->set_header(*this, std::move(header));
        mark_layout_dirty();
        return *this;
    }

    auto Sheet::set_content(std::shared_ptr<scene::NanControl> content) -> Sheet& {
        (void)panel_->set_content(*this, std::move(content));
        mark_layout_dirty();
        return *this;
    }

    auto Sheet::set_footer(std::shared_ptr<scene::NanControl> footer) -> Sheet& {
        (void)panel_->set_footer(*this, std::move(footer));
        mark_layout_dirty();
        return *this;
    }

    auto Sheet::header() const -> scene::NanControl* {
        return panel_->header();
    }

    auto Sheet::content() const -> scene::NanControl* {
        return panel_->content();
    }

    auto Sheet::footer() const -> scene::NanControl* {
        return panel_->footer();
    }

    void Sheet::open() {
        if (phase_ == SheetPhase::opening || phase_ == SheetPhase::opened) {
            return;
        }
        if (mount_mode_ == MountMode::unmounted) {
            mount_mode_ = resolve_overlay_host() != nullptr ? MountMode::overlay : MountMode::tree;
        }
        set_visible(mount_mode_ == MountMode::tree);
        dismiss_layer_->set_visible(true);
        if (auto* tree = get_tree(); tree != nullptr) {
            auto* focused = tree->focused_node();
            if (focused != nullptr && focused != this && !focus_scope_->is_ancestor_of(*focused)) {
                previous_focus_ = focused->weak_from_this();
            }
        }
        if (!mount()) {
            dismiss_layer_->set_visible(false);
            set_visible(false);
            return;
        }
        phase_ = SheetPhase::opening;
        apply_style();

        if (auto* tree = get_tree(); tree != nullptr) {
            auto* focused = tree->focused_node();
            if (focused == nullptr || !focus_scope_->is_ancestor_of(*focused)) {
                if (focused != nullptr && focused != this) {
                    previous_focus_ = focused->weak_from_this();
                }
                (void)tree->focus_first_within(
                    *focus_scope_,
                    scene::FocusRequestContext {.reason = scene::FocusRequestReason::popup}
                );
            }
        }

        auto& fade = dismiss_layer_->fade();
        fade.clear_behavior();
        fade.set_target(0.0F);
        fade.set_behavior(fade_behavior(*system_, true));
        start_fade(1.0F);

        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    void Sheet::close() {
        if (!active() || phase_ == SheetPhase::closing) {
            return;
        }
        phase_ = SheetPhase::closing;
        dismiss_layer_->fade().set_behavior(fade_behavior(*system_, false));
        start_fade(0.0F);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Sheet::set_open(const bool open) {
        if (open) {
            this->open();
        }
        else {
            close();
        }
    }

    auto Sheet::is_open() const noexcept -> bool {
        return phase_ == SheetPhase::opening || phase_ == SheetPhase::opened;
    }

    void Sheet::set_dismissible(const bool dismissible) noexcept {
        dismissible_ = dismissible;
    }

    auto Sheet::dismissible() const noexcept -> bool {
        return dismissible_;
    }

    void Sheet::set_on_close(std::function<void()> callback) {
        user_close_sub_.disconnect();
        if (callback) {
            user_close_sub_ = closed_.subscribe(std::move(callback));
        }
    }

    auto Sheet::closed() const noexcept -> const reactive::Event<>& {
        return closed_;
    }

    void Sheet::set_theme(theme::NanTheme theme) {
        system_ = theme::design_system_shared_from_theme(theme);
        system_explicit_ = true;
        theme_view_ = theme;
        apply_style();
    }

    auto Sheet::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Sheet::set_override(theme::DialogRecipeRule rule) {
        override_ = std::move(rule);
        apply_style();
    }

    auto Sheet::resolved_style() const -> theme::ResolvedDialogStyle {
        auto style = theme::resolve_dialog(*system_, appearance_);
        // 贴边抽屉默认直角贴齐视口边缘；若作者通过 set_override 显式指定了 panel_radius 则尊重覆盖值。
        style.panel.radius = 0.0F;
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        if (extent_.has_value()) {
            style.metrics.panel_width = *extent_;
        }
        return style;
    }

    void Sheet::set_text_pipeline(text::TextPipeline pipeline) {
        panel_->set_text_pipeline(std::move(pipeline));
    }

    void Sheet::apply_default_text_pipeline(const text::TextPipeline& pipeline) {
        panel_->apply_default_text_pipeline(pipeline);
    }

    void Sheet::apply_font_context(text::FontPipelineCache& context) {
        panel_->apply_font_context(context);
    }

    void Sheet::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_style();
    }

    void Sheet::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_style();
    }

    void Sheet::apply_style() {
        const auto style = resolved_style();
        panel_->set_style(style);
        dismiss_layer_->set_scrim(scrim_style(style));
    }

    auto Sheet::z_index_hint() const -> int {
        return active() && mount_mode_ != MountMode::overlay ? 1 : 0;
    }

    auto Sheet::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        if (!active() || mount_mode_ == MountMode::overlay) {
            return constraints.constrain(foundation::NanSize {0.0F, 0.0F});
        }
        return constraints.constrain(
            foundation::NanSize(constraints.max_width, constraints.max_height)
        );
    }

    void Sheet::on_layout() {
        if (!active() || dismiss_layer_->parent() != this) {
            return;
        }
        dismiss_layer_->layout_to(local_rect());
    }

    void Sheet::on_process(const float /*dt*/) {
        if (!active()) {
            return;
        }
        auto& fade = dismiss_layer_->fade();
        if (phase_ == SheetPhase::opening && !fade.is_animating()) {
            phase_ = SheetPhase::opened;
        }
        else if (phase_ == SheetPhase::closing && !fade.is_animating()) {
            const auto keep_alive = weak_from_this().lock();
            phase_ = SheetPhase::closed;
            dismiss_layer_->set_visible(false);
            unmount();
            set_visible(false);
            restore_previous_focus();
            closed_.emit();
            mark_dirty(
                scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
            );
        }
    }

    void Sheet::on_exit_tree() {
        phase_ = SheetPhase::closed;
        unmount();
        set_visible(false);
        scene::NanControl::on_exit_tree();
    }

    void Sheet::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
    }

    auto Sheet::resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost> {
        if (auto injected = overlay_service_.lock()) {
            return injected;
        }
        for (auto* node = parent(); node != nullptr; node = node->parent()) {
            auto* node_2d = node->as_node2d();
            auto* stack = node_2d != nullptr ? node_2d->as_layer_stack() : nullptr;
            if (stack == nullptr) {
                continue;
            }
            if (auto* host = stack->as_overlay_host(); host != nullptr) {
                return host->weak_self().lock();
            }
        }
        return nullptr;
    }

    auto Sheet::mount() -> bool {
        if (mount_mode_ == MountMode::overlay) {
            if (portal_handle_ != nullptr) {
                return true;
            }
            auto host = resolve_overlay_host();
            if (host == nullptr) {
                return false;
            }
            install_dismiss_callback();
            portal_handle_ = std::make_unique<scene::OverlayHandle>(host->present(
                dismiss_layer_,
                scene::OverlayOptions {
                    .level = scene::OverlayLevel::modal,
                    .block_below = true,
                }
            ));
            return true;
        }

        if (dismiss_layer_->parent() == this) {
            return true;
        }
        install_dismiss_callback();
        add_child(dismiss_layer_);
        mark_layout_dirty();
        return true;
    }

    void Sheet::install_dismiss_callback() {
        auto weak = std::weak_ptr<Sheet>(std::static_pointer_cast<Sheet>(shared_from_this()));
        dismiss_layer_->set_callback([weak](const internal::DismissReason) {
            if (auto sheet = weak.lock(); sheet != nullptr && sheet->dismissible_) {
                sheet->close();
            }
        });
    }

    void Sheet::install_origin_resolver() {
        auto* panel = panel_.get();
        dismiss_layer_->set_origin_resolver(
            [panel](const foundation::NanSize viewport, const foundation::NanSize content) {
                switch (panel->side()) {
                    case SheetSide::left:
                    case SheetSide::top:
                        return foundation::NanPoint {0.0F, 0.0F};
                    case SheetSide::right:
                        return foundation::NanPoint {
                            std::max(0.0F, viewport.get_width() - content.get_width()),
                            0.0F,
                        };
                    case SheetSide::bottom:
                        return foundation::NanPoint {
                            0.0F,
                            std::max(0.0F, viewport.get_height() - content.get_height()),
                        };
                }
                return foundation::NanPoint {0.0F, 0.0F};
            }
        );
    }

    void Sheet::unmount() {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
    }

    void Sheet::restore_previous_focus() {
        auto previous = previous_focus_.lock();
        previous_focus_.reset();
        if (previous == nullptr) {
            return;
        }
        auto* tree = get_tree();
        auto* target = previous->as_node2d();
        if (tree != nullptr && target != nullptr) {
            tree->set_focus(
                target,
                scene::FocusRequestContext {.reason = scene::FocusRequestReason::restore}
            );
        }
    }

    void Sheet::start_fade(const float target) {
        if (auto* tree = dismiss_layer_->get_tree(); tree != nullptr) {
            tree->animation_host().set_target(
                *dismiss_layer_,
                dismiss_layer_->fade(),
                target,
                scene::DirtyFlags::paint
            );
            return;
        }
        dismiss_layer_->fade().clear_behavior();
        dismiss_layer_->fade().set_target(target);
    }
} // namespace nandina::widget
