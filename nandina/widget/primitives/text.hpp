//
// widget/primitives/text — minimal text drawing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP

#include "../../reactive/property.hpp"
#include "../../scene/control.hpp"
#include "../../scene/property_endpoint.hpp"
#include "../../text/text_layout_backend.hpp"
#include "../visual_property.hpp"

#include <array>
#include <memory>
#include <optional>
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
        void set_behavior(motion::Behavior<foundation::NanColor> behavior);
        [[nodiscard]] auto value() const noexcept -> const foundation::NanColor*;
        [[nodiscard]] auto target() const noexcept -> const foundation::NanColor*;

    private:
        Text* text_;
    };

    class TextFontSizeProperty {
    public:
        explicit TextFontSizeProperty(Text& text) noexcept: text_(&text) {}
        void set(float size);
        void set_behavior(motion::Behavior<float> behavior);
        void set_spring(motion::SpringSpec spec);
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
            const text::ITextLayoutBackend& backend = text::deterministic_text_layout_backend()
        );

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        [[nodiscard]] auto text_property() -> reactive::Property<std::string>&;
        [[nodiscard]] auto text_property() const -> reactive::ReadProperty<std::string>;

        void set_style(text::TextStyle style);
        /// Apply a derived style baseline for `primitives::Text` parts without marking fields as
        /// explicit instance overrides. Active per-field instance overrides and inherited
        /// `StyleContext` fields still take precedence until cleared (`widget::Label` resolves its
        /// own semantic theme/role baseline over `derived_style_`).
        void apply_derived_style(text::TextStyle style);
        void clear_derived_style();
        [[nodiscard]] auto has_derived_style() const noexcept -> bool;
        [[nodiscard]] auto style() const -> const text::TextStyle&;

        void set_color(foundation::NanColor color);
        void clear_color();
        [[nodiscard]] auto has_color_override() const noexcept -> bool;
        [[nodiscard]] auto color() const -> foundation::NanColor;

        void set_font_size(float size);
        void clear_font_size();
        [[nodiscard]] auto has_font_size_override() const noexcept -> bool;
        [[nodiscard]] auto font_size() const -> float;
        /// Line-height multiplier of `font_size`; `0` clears the numeric override and restores
        /// the component/derived/backend fallback. A positive value overrides it.
        void set_line_height(float factor);
        void clear_line_height();
        [[nodiscard]] auto has_line_height_override() const noexcept -> bool;
        [[nodiscard]] auto line_height() const -> float;
        [[nodiscard]] auto visual_part(visual::label_t) noexcept -> Text&;
        [[nodiscard]] auto property(visual::color_t) noexcept -> TextColorProperty;
        [[nodiscard]] auto property(visual::font_size_t) noexcept -> TextFontSizeProperty;
        void set_font(theme::FontRequest request);
        void clear_font();
        void set_font_family(resource::ResourceKey family);
        void clear_font_family();
        void set_font_weight(int weight);
        void clear_font_weight();
        void set_font_slant(theme::FontSlant slant);
        void clear_font_slant();
        [[nodiscard]] auto has_font_override() const noexcept -> bool;
        [[nodiscard]] auto has_font_family_override() const noexcept -> bool;
        [[nodiscard]] auto has_font_weight_override() const noexcept -> bool;
        [[nodiscard]] auto has_font_slant_override() const noexcept -> bool;
        [[nodiscard]] auto font() const -> const theme::FontRequest&;

        /// Clear all instance explicit style overrides (`color`, `font_size`, `line_height`,
        /// `font` family/weight/slant) and re-resolve against `StyleContext` and component
        /// fallbacks in a single pass. Box configuration (`overflow`, `max_lines`, `align`,
        /// `vertical_align`) and semantic component tokens are preserved.
        void clear_style_overrides();

        void set_overflow(text::TextOverflow overflow);
        void clear_overflow();
        [[nodiscard]] auto has_overflow_override() const noexcept -> bool;
        [[nodiscard]] auto overflow() const -> text::TextOverflow;

        void set_max_lines(int lines);
        void clear_max_lines();
        [[nodiscard]] auto has_max_lines_override() const noexcept -> bool;
        [[nodiscard]] auto max_lines() const -> int;

        /// 设置文本在其**被指派矩形**内的水平对齐；见 `text::text_align_offset()`。
        ///
        /// 只有盒子比文本宽时才有视觉效果 —— 收缩包裹的文本三种取值结果相同。
        /// 绘制入口是 `draw_in()`；`draw_at()` 没有盒子，因此不对齐。
        void set_align(theme::TextAlign align);
        void clear_align();
        [[nodiscard]] auto has_align_override() const noexcept -> bool;
        [[nodiscard]] auto align() const -> theme::TextAlign;

        /// Place the natural text block inside its assigned height. Paint-only; draw_at ignores it.
        void set_vertical_align(text::TextVerticalAlign align);
        void clear_vertical_align();
        [[nodiscard]] auto has_vertical_align_override() const noexcept -> bool;
        [[nodiscard]] auto vertical_align() const -> text::TextVerticalAlign;

        [[nodiscard]] auto measured_text_width() const -> float;
        [[nodiscard]] auto measured_text_height() const -> float;
        [[nodiscard]] auto laid_out_font_size() const -> float;
        [[nodiscard]] auto layout_result() const -> const text::TextLayoutResult&;
        [[nodiscard]] auto line_count() const noexcept -> std::size_t;
        [[nodiscard]] auto overflowed() const noexcept -> bool;

        void set_text_pipeline(text::TextPipeline pipeline);

        /// update_metrics() 记忆化的命中/失配计数（诊断与测试用）。
        [[nodiscard]] auto metrics_cache_hits() const noexcept -> long;
        [[nodiscard]] auto metrics_cache_misses() const noexcept -> long;
        [[nodiscard]] auto text_pipeline() const -> text::TextPipeline;
        void apply_default_text_pipeline(const text::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;

        /// The referenced backend must outlive this Text instance.
        void set_layout_backend(const text::ITextLayoutBackend& backend);
        [[nodiscard]] auto layout_backend() const -> const text::ITextLayoutBackend&;
        void set_layout_renderer(text::ITextLayoutRenderer* renderer);
        [[nodiscard]] auto layout_renderer() const -> text::ITextLayoutRenderer*;

        /// 在 screen-space `position` 处绘制（自然尺寸盒子，两轴均不对齐）。
        void draw_at(render::DrawContext& ctx, foundation::NanPoint position);

        /// 在 screen-space `rect` 内按 align/vertical_align 摆放每行与整个文本块。
        ///
        /// @param ctx  绘制上下文。
        /// @param rect 已包含 viewport 变换的目标矩形；不要再次乘 DPI 或 viewport scale。
        ///
        /// 偏移是**绘制期**计算的，写进布局结果的 `text::TextLayoutLine::origin_x`；布局本身
        /// （`size` / `caret_stops`）不受影响，所以命中测试仍在未偏移的空间里。
        void draw_in(render::DrawContext& ctx, foundation::NanRect rect);
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        virtual void resolve_component_style_fallbacks(
            const theme::ResolvedStyleContext& context,
            text::TextStyle& fallback
        ) const;
        void reconcile_style(const theme::ResolvedStyleContext& context);
        /// True when `set_font_size()` was called; component defaults must not override it.
        [[nodiscard]] auto font_size_is_explicit() const noexcept -> bool;
        void clear_explicit_color() noexcept;
        void clear_explicit_line_height() noexcept;

    private:
        struct OverrideStateSnapshot {
            std::optional<text::TextStyle> derived_style;
            bool color = false;
            bool font_size = false;
            bool line_height = false;
            bool font_family = false;
            bool font_weight = false;
            bool font_slant = false;
            bool overflow = false;
            bool max_lines = false;
            bool align = false;
            bool vertical_align = false;
        };

        [[nodiscard]] auto capture_override_state() const -> OverrideStateSnapshot;
        void restore_override_state(OverrideStateSnapshot snapshot) noexcept;
        void commit_reconciled_style(
            const text::TextStyle& before,
            text::TextStyle next,
            bool force_color_presentation = false,
            bool force_font_size_presentation = false
        );
        void draw_text_box(render::DrawContext& ctx, foundation::NanRect rect, bool align_to_box);
        void apply_text(const std::string& text);
        void update_metrics(
            foundation::NanLayoutConstraints constraints = foundation::NanLayoutConstraints::loose()
        );
        void resolve_font();

        /// update_metrics() 的记忆化条目。
        ///
        /// 键是**完整的排版输入**（文本 + 所有影响排版的样式字段 + 约束）：命中就代表输入
        /// 逐字段相同，结果必然相同，因此这里没有「忘了失效」这类静默错误。颜色不进键 ——
        /// 颜色不影响排版，TextLayoutResult 里也不含颜色。
        ///
        /// 为什么需要：容器每加一个子项就 `relayout()` 一次，整棵子树被反复测量 ——
        /// showcase 的 Label 页因此在构建期产生约 1.4 万次测量（见
        /// docs/references/text_shaping_cost.md §5.1）。
        ///
        /// 为什么是 8 条：同一个 Text 在一个周期里会被交替用多种约束测量/摆放
        /// （父级 measure 用 loose，arrange 用 tight，横向拉伸后 tight 的宽度还会变），
        /// 再加上构建过程中样式是分步应用的。实测命中率：
        /// 2 条 8%、4 条 45%、8 条 74%（Label 页构建 378 → 154 ms）。
        ///
        /// 为什么还要字节预算：条目里存的是完整排版结果。短标签一条几百字节，但一个
        /// 5000 字的 Text 一条就是几百 KB —— 8 条会让这个节点多占几 MB。所以超过预算的
        /// 结果只留一条（`kMetricsCacheByteBudget`），把「按条数」和「按字节」两个上限
        /// 一起用上。淘汰永远是安全的：最坏情况只是少一次命中。
        struct MetricsCacheEntry {
            std::string text;
            text::TextStyle style;
            foundation::NanLayoutConstraints constraints;
            text::TextLayoutResult result;
            float natural_width = 0.0F;
            float natural_height = 0.0F;
            bool unwrapped = false;
            std::uint64_t generation = 0;
        };
        static constexpr std::size_t kMetricsCacheEntries = 8;
        /// 单个 Text 节点允许的缓存字节上限；超过它的结果只保留一条。
        static constexpr std::size_t kMetricsCacheByteBudget = 64 * 1024;

        [[nodiscard]] auto find_metrics_cache(const text::TextLayoutInput& input) const
            -> std::optional<std::size_t>;
        auto store_metrics_cache(const text::TextLayoutInput& input, text::TextLayoutResult result)
            -> std::size_t;
        /// 后端换了（resolve_font / set_text_pipeline）就必须清：后端不在键里。
        void clear_metrics_cache() noexcept;

        [[nodiscard]] static auto
        metrics_entry_bytes(std::string_view text, const text::TextLayoutResult& result)
            -> std::size_t;

        std::array<MetricsCacheEntry, kMetricsCacheEntries> metrics_cache_ {};
        std::size_t metrics_cache_used_ = 0;
        std::size_t metrics_cache_next_ = 0;
        std::size_t metrics_cache_bytes_ = 0;
        std::size_t active_cache_slot_ = kMetricsCacheEntries;
        std::uint64_t active_cache_generation_ = 0;
        std::uint64_t metrics_cache_generation_ = 0;
        long metrics_cache_hits_ = 0;
        long metrics_cache_misses_ = 0;

        reactive::Property<std::string> text_;
        text::TextStyle style_ {};
        std::optional<text::TextStyle> derived_style_;
        scene::PropertyEndpoint<foundation::NanColor> color_presentation_;
        scene::PropertyEndpoint<float> font_size_presentation_;
        text::TextLayoutResult layout_ {};
        const text::ITextLayoutBackend* backend_ = nullptr;
        text::ITextLayoutRenderer* renderer_ = nullptr;
        bool pipeline_explicit_ = false;
        bool color_explicit_ = false;
        bool font_size_explicit_ = false;
        bool line_height_explicit_ = false;
        bool font_family_explicit_ = false;
        bool font_weight_explicit_ = false;
        bool font_slant_explicit_ = false;
        bool overflow_explicit_ = false;
        bool max_lines_explicit_ = false;
        bool align_explicit_ = false;
        bool vertical_align_explicit_ = false;
        text::FontPipelineCache* font_context_ = nullptr;
        std::shared_ptr<text::FontPipeline> resolved_font_pipeline_;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
