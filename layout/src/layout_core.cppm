module;

#include <algorithm>
#include <cstdint>
#include <vector>

export module nandina.layout.core;

export import nandina.foundation.nan_rect;
export import nandina.foundation.nan_size;
export import nandina.foundation.nan_constraints;
export import nandina.foundation.nan_types;

export namespace nandina::layout {

    // ── 布局方向 ─────────────────────────────────────────────
    enum class LayoutAxis : std::uint8_t {
        column, // 主轴垂直
        row,    // 主轴水平
        stack,  // 重叠（Z 轴堆叠）
        flow,   // 流式换行（主轴水平，超宽自动折行）
    };

    // ── 对齐方式 ─────────────────────────────────────────────
    enum class LayoutAlignment : std::uint8_t {
        start,
        center,
        end,
        stretch,
        space_between,
        space_around,
    };

    // ── 子节点规格 ──────────────────────────────────────────
    struct LayoutChildSpec {
        geometry::NanSize preferred_size{};
        geometry::NanSize min_size{};   ///< 最小尺寸约束
        geometry::NanSize max_size{geometry::NanConstraints::k_infinity, geometry::NanConstraints::k_infinity};
                                     ///< 最大尺寸约束
        int flex_factor = 0;
        bool can_shrink = true;         ///< 空间不足时是否允许压缩
        types::SizeValue width_mode;           ///< 宽度尺寸模式
        types::SizeValue height_mode;          ///< 高度尺寸模式
    };

    // ── 内边距 ──────────────────────────────────────────────
    struct LayoutInsets {
        float left   = 0.0f;
        float top    = 0.0f;
        float right  = 0.0f;
        float bottom = 0.0f;

        [[nodiscard]] auto horizontal() const noexcept -> float {
            return left + right;
        }

        [[nodiscard]] auto vertical() const noexcept -> float {
            return top + bottom;
        }
    };

    // ── 布局请求 ─────────────────────────────────────────────
    struct LayoutRequest {
        LayoutAxis axis = LayoutAxis::column;
        geometry::NanRect container_bounds{};
        geometry::NanConstraints constraints = geometry::NanConstraints::expand();
        LayoutInsets padding{};
        float gap = 0.0f;
        float line_gap = 0.0f;   ///< 流式布局行间距
        LayoutAlignment cross_alignment = LayoutAlignment::start;
        LayoutAlignment main_alignment  = LayoutAlignment::start;
        std::vector<LayoutChildSpec> children;

        [[nodiscard]] auto content_bounds() const noexcept -> geometry::NanRect {
            return geometry::NanRect{
                container_bounds.x() + padding.left,
                container_bounds.y() + padding.top,
                container_bounds.x() + padding.left + std::max(0.0f, container_bounds.width() - padding.left - padding.right),
                container_bounds.y() + padding.top + std::max(0.0f, container_bounds.height() - padding.top - padding.bottom),
            };
        }
    };

    namespace detail {

        [[nodiscard]] inline auto clamp_non_negative(const float value) noexcept -> float {
            return std::max(0.0f, value);
        }

        [[nodiscard]] inline auto resolve_cross_extent(
            const LayoutAlignment align,
            const float available,
            const float desired,
            const float min_extent,
            const float max_extent) noexcept -> float {
            const float unclamped = align == LayoutAlignment::stretch
                ? clamp_non_negative(available)
                : std::min(clamp_non_negative(desired), clamp_non_negative(available));
            return std::clamp(unclamped, clamp_non_negative(min_extent), max_extent);
        }

        [[nodiscard]] inline auto resolve_main_extent(
            const float desired,
            const float min_extent,
            const float max_extent) noexcept -> float {
            return std::clamp(clamp_non_negative(desired), clamp_non_negative(min_extent), max_extent);
        }

        [[nodiscard]] inline auto resolve_cross_position(
            const float origin, const float available, const float extent, const LayoutAlignment align) noexcept
            -> float {
            switch (align) {
            case LayoutAlignment::center:
            case LayoutAlignment::space_between:
            case LayoutAlignment::space_around:
                return origin + (available - extent) * 0.5f;
            case LayoutAlignment::end:
                return origin + (available - extent);
            case LayoutAlignment::start:
            case LayoutAlignment::stretch:
            default:
                return origin;
            }
        }

        struct MainAxisPlacement {
            float start_offset = 0.0f;
            float gap          = 0.0f;
        };

        [[nodiscard]] inline auto resolve_main_axis_placement(
            const LayoutAlignment justify, const int child_count, const float used_extent,
            const float available_extent, const float base_gap) noexcept -> MainAxisPlacement {
            const float free_space = clamp_non_negative(available_extent - used_extent);

            switch (justify) {
            case LayoutAlignment::end:
                return {.start_offset = free_space, .gap = base_gap};
            case LayoutAlignment::center:
                return {.start_offset = free_space * 0.5f, .gap = base_gap};
            case LayoutAlignment::space_between:
                if (child_count > 1) {
                    return {.start_offset = 0.0f, .gap = base_gap + free_space / static_cast<float>(child_count - 1)};
                }
                return {.start_offset = free_space * 0.5f, .gap = base_gap};
            case LayoutAlignment::space_around:
                if (child_count > 0) {
                    const float extra = free_space / static_cast<float>(child_count);
                    return {.start_offset = extra * 0.5f, .gap = base_gap + extra};
                }
                return {.start_offset = 0.0f, .gap = base_gap};
            case LayoutAlignment::start:
            case LayoutAlignment::stretch:
            default:
                return {.start_offset = 0.0f, .gap = base_gap};
            }
        }

    } // namespace detail

    // ── BasicLayoutBackend ───────────────────────────────────
    // 轻量级 Flex 布局引擎，支持 column/row/stack 三种模式。
    // reference: Flutter RenderFlex, archive LayoutBackend
    class BasicLayoutBackend {
    public:
        [[nodiscard]] auto compute(const LayoutRequest& request) const -> std::vector<geometry::NanRect> {
            switch (request.axis) {
            case LayoutAxis::column:
                return compute_column(request);
            case LayoutAxis::row:
                return compute_row(request);
            case LayoutAxis::stack:
                return compute_stack(request);
            case LayoutAxis::flow:
                return compute_flow(request);
            }
            return {};
        }

    private:
        [[nodiscard]] static auto compute_column(const LayoutRequest& request) -> std::vector<geometry::NanRect> {
            using namespace detail;

            const auto content_bounds = request.content_bounds();
            const float avail_w = clamp_non_negative(content_bounds.width());
            const float avail_h = clamp_non_negative(content_bounds.height());

            float fixed_total = 0.0f;
            int flex_total    = 0;

            for (const auto& child : request.children) {
                if (child.flex_factor > 0) {
                    flex_total += child.flex_factor;
                } else {
                    fixed_total += resolve_main_extent(
                        child.preferred_size.height(),
                        child.min_size.height(),
                        child.max_size.height());
                }
            }

            const int child_count  = static_cast<int>(request.children.size());
            const float gap_total  = child_count > 1 ? request.gap * static_cast<float>(child_count - 1) : 0.0f;
            const float remaining  = avail_h - fixed_total - gap_total;
            const float flex_inv   = flex_total > 0 ? 1.0f / static_cast<float>(flex_total) : 0.0f;
            std::vector<float> child_heights;
            child_heights.reserve(request.children.size());

            float used_children_h = 0.0f;
            for (const auto& child : request.children) {
                const float desired_height = (child.flex_factor > 0 && flex_total > 0)
                    ? (remaining > 0.0f
                        ? remaining * (static_cast<float>(child.flex_factor) * flex_inv)
                        : (child.can_shrink ? 0.0f : clamp_non_negative(child.preferred_size.height())))
                    : clamp_non_negative(child.preferred_size.height());
                const float child_height = resolve_main_extent(
                    desired_height,
                    child.min_size.height(),
                    child.max_size.height());
                child_heights.push_back(child_height);
                used_children_h += child_height;
            }

            const float used_h = used_children_h + gap_total;
            const auto placement   = resolve_main_axis_placement(
                request.main_alignment, child_count, used_h, avail_h, request.gap);

            std::vector<geometry::NanRect> frames;
            frames.reserve(request.children.size());

            float cursor_y = content_bounds.y() + placement.start_offset;
            bool first     = true;

            for (std::size_t index = 0; index < request.children.size(); ++index) {
                const auto& child = request.children[index];
                if (!first) {
                    cursor_y += placement.gap;
                }
                first = false;

                const float child_h = child_heights[index];
                const float child_w = resolve_cross_extent(
                    request.cross_alignment,
                    avail_w,
                    clamp_non_negative(child.preferred_size.width()),
                    child.min_size.width(),
                    child.max_size.width());
                const float child_x = resolve_cross_position(
                    content_bounds.x(), avail_w, child_w, request.cross_alignment);

                frames.emplace_back(child_x, cursor_y, child_x + child_w, cursor_y + child_h);
                cursor_y += child_h;
            }

            return frames;
        }

        [[nodiscard]] static auto compute_row(const LayoutRequest& request) -> std::vector<geometry::NanRect> {
            using namespace detail;

            const auto content_bounds = request.content_bounds();
            const float avail_w = clamp_non_negative(content_bounds.width());
            const float avail_h = clamp_non_negative(content_bounds.height());

            float fixed_total = 0.0f;
            int flex_total    = 0;

            for (const auto& child : request.children) {
                if (child.flex_factor > 0) {
                    flex_total += child.flex_factor;
                } else {
                    fixed_total += resolve_main_extent(
                        child.preferred_size.width(),
                        child.min_size.width(),
                        child.max_size.width());
                }
            }

            const int child_count  = static_cast<int>(request.children.size());
            const float gap_total  = child_count > 1 ? request.gap * static_cast<float>(child_count - 1) : 0.0f;
            const float remaining  = avail_w - fixed_total - gap_total;
            const float flex_inv   = flex_total > 0 ? 1.0f / static_cast<float>(flex_total) : 0.0f;
            std::vector<float> child_widths;
            child_widths.reserve(request.children.size());

            float used_children_w = 0.0f;
            for (const auto& child : request.children) {
                const float desired_width = (child.flex_factor > 0 && flex_total > 0)
                    ? (remaining > 0.0f
                        ? remaining * (static_cast<float>(child.flex_factor) * flex_inv)
                        : (child.can_shrink ? 0.0f : clamp_non_negative(child.preferred_size.width())))
                    : clamp_non_negative(child.preferred_size.width());
                const float child_width = resolve_main_extent(
                    desired_width,
                    child.min_size.width(),
                    child.max_size.width());
                child_widths.push_back(child_width);
                used_children_w += child_width;
            }

            const float used_w = used_children_w + gap_total;
            const auto placement   = resolve_main_axis_placement(
                request.main_alignment, child_count, used_w, avail_w, request.gap);

            std::vector<geometry::NanRect> frames;
            frames.reserve(request.children.size());

            float cursor_x = content_bounds.x() + placement.start_offset;
            bool first     = true;

            for (std::size_t index = 0; index < request.children.size(); ++index) {
                const auto& child = request.children[index];
                if (!first) {
                    cursor_x += placement.gap;
                }
                first = false;

                const float child_w = child_widths[index];
                const float child_h = resolve_cross_extent(
                    request.cross_alignment,
                    avail_h,
                    clamp_non_negative(child.preferred_size.height()),
                    child.min_size.height(),
                    child.max_size.height());
                const float child_y = resolve_cross_position(
                    content_bounds.y(), avail_h, child_h, request.cross_alignment);

                frames.emplace_back(cursor_x, child_y, cursor_x + child_w, child_y + child_h);
                cursor_x += child_w;
            }

            return frames;
        }

        [[nodiscard]] static auto compute_stack(const LayoutRequest& request) -> std::vector<geometry::NanRect> {
            using namespace detail;

            const auto content_bounds = request.content_bounds();
            const float avail_w = clamp_non_negative(content_bounds.width());
            const float avail_h = clamp_non_negative(content_bounds.height());

            std::vector<geometry::NanRect> frames;
            frames.reserve(request.children.size());

            for (const auto& child : request.children) {
                const float child_w = resolve_cross_extent(
                    request.cross_alignment,
                    avail_w,
                    clamp_non_negative(child.preferred_size.width()),
                    child.min_size.width(),
                    child.max_size.width());
                const float child_h = resolve_cross_extent(
                    request.main_alignment,
                    avail_h,
                    clamp_non_negative(child.preferred_size.height()),
                    child.min_size.height(),
                    child.max_size.height());
                const float child_x = resolve_cross_position(
                    content_bounds.x(), avail_w, child_w, request.cross_alignment);
                const float child_y = resolve_cross_position(
                    content_bounds.y(), avail_h, child_h, request.main_alignment);

                frames.emplace_back(child_x, child_y, child_x + child_w, child_y + child_h);
            }

            return frames;
        }

        /// 流式换行布局：子项从左到右排列，超出行宽时自动折行。
        /// 主轴为水平方向，交叉轴为垂直方向。
        [[nodiscard]] static auto compute_flow(const LayoutRequest& request) -> std::vector<geometry::NanRect> {
            using namespace detail;

            const auto content_bounds  = request.content_bounds();
            const float avail_w        = clamp_non_negative(content_bounds.width());
            const float avail_h        = clamp_non_negative(content_bounds.height());
            const float inline_gap     = request.gap;
            const float line_gap       = request.line_gap;

            struct LineItems {
                std::vector<std::size_t> child_indices;
                float used_width = 0.0f;
                float max_height = 0.0f;
            };

            std::vector<LineItems> lines;
            lines.emplace_back();
            float cursor_x = 0.0f;

            for (std::size_t i = 0; i < request.children.size(); ++i) {
                const auto& child = request.children[i];
                const float child_w = clamp_non_negative(
                    resolve_main_extent(child.preferred_size.width(), child.min_size.width(), child.max_size.width()));
                const float child_h = clamp_non_negative(
                    resolve_main_extent(child.preferred_size.height(), child.min_size.height(), child.max_size.height()));

                // 是否需要折行（至少有一个子项在行内时才折行）
                if (!lines.back().child_indices.empty() && cursor_x + child_w > avail_w) {
                    // 新行
                    lines.emplace_back();
                    cursor_x = 0.0f;
                }

                if (!lines.back().child_indices.empty()) {
                    cursor_x += inline_gap;
                }

                lines.back().child_indices.push_back(i);
                lines.back().used_width = cursor_x + child_w;
                lines.back().max_height = std::max(lines.back().max_height, child_h);

                cursor_x += child_w;
            }

            // 生成 frames
            std::vector<geometry::NanRect> frames;
            frames.resize(request.children.size());

            float line_cursor_y = content_bounds.y();

            for (const auto& line : lines) {
                const float line_h = line.max_height;
                const float remaining_w = avail_w - (line.used_width + (line.child_indices.size() - 1) * inline_gap);
                const float extra_per_child = line.child_indices.size() > 0
                    ? std::max(0.0f, remaining_w / static_cast<float>(line.child_indices.size()))
                    : 0.0f;

                cursor_x = content_bounds.x();
                bool first = true;

                for (const auto idx : line.child_indices) {
                    const auto& child = request.children[idx];
                    const float child_pref_w = clamp_non_negative(
                        resolve_main_extent(child.preferred_size.width(), child.min_size.width(), child.max_size.width()));
                    const float child_h = clamp_non_negative(
                        resolve_main_extent(child.preferred_size.height(), child.min_size.height(), child.max_size.height()));
                    const float child_y = resolve_cross_position(
                        line_cursor_y, line_h, child_h, request.cross_alignment);

                    // 每行内等分剩余宽度，使卡片在行宽足够时填满整行
                    const float child_w = std::min(child_pref_w + extra_per_child,
                                                   clamp_non_negative(avail_w));

                    if (!first) cursor_x += inline_gap;
                    first = false;

                    frames[idx] = geometry::NanRect{cursor_x, child_y, cursor_x + child_w, child_y + child_h};
                    cursor_x += child_w;
                }

                line_cursor_y += line_h + line_gap;
            }

            return frames;
        }
    };

    /// 返回全局默认布局引擎实例
    [[nodiscard]] inline auto default_layout_backend() -> const BasicLayoutBackend& {
        static const BasicLayoutBackend backend{};
        return backend;
    }

} // namespace nandina::layout