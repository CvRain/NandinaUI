module;

#include <algorithm>
#include <memory>
#include <vector>

export module nandina.layout.container;

export import nandina.foundation.nan_rect;
export import nandina.foundation.nan_size;
export import nandina.runtime.nan_widget;
export import nandina.layout.core;

export namespace nandina::layout {

    namespace detail {
        [[nodiscard]] inline auto deflate_constraint_min(const float value, const float delta) noexcept -> float {
            return std::max(0.0f, value - delta);
        }

        [[nodiscard]] inline auto deflate_constraint_max(const float value, const float delta) noexcept -> float {
            if (value == geometry::NanConstraints::k_infinity) {
                return value;
            }
            return std::max(0.0f, value - delta);
        }

        [[nodiscard]] inline auto measured_or_preferred_size(const runtime::NanWidget& child) noexcept -> geometry::NanSize {
            const auto measured = child.measured_size();
            const auto preferred = child.preferred_size();
            return {
                measured.width() > 0.0f ? measured.width() : preferred.width(),
                measured.height() > 0.0f ? measured.height() : preferred.height(),
            };
        }

        [[nodiscard]] inline auto derive_child_max_size(const runtime::NanWidget& child) noexcept -> geometry::NanSize {
            const auto constraints = child.measured_constraints();
            const auto preferred = child.preferred_size();

            const float max_width = (constraints.max_width() == 0.0f && preferred.width() > 0.0f)
                ? geometry::NanConstraints::k_infinity
                : constraints.max_width();
            const float max_height = (constraints.max_height() == 0.0f && preferred.height() > 0.0f)
                ? geometry::NanConstraints::k_infinity
                : constraints.max_height();

            return {max_width, max_height};
        }
    }

    // ── LayoutContainer ──────────────────────────────────────
    // 抽象基类，提供 gap/padding/align/justify 等布局属性设置。
    // Column/Row/Stack 继承此类并重写 layout() 方法。
    class LayoutContainer : public runtime::NanWidget {
    public:
        // ── 诊断计数器 ──
        inline static thread_local int s_measure_count{0};

        auto gap(const float value) -> LayoutContainer& {
            gap_ = value;
            request_layout();
            return *this;
        }

        auto padding(const float value) -> LayoutContainer& {
            padding_top_ = padding_right_ = padding_bottom_ = padding_left_ = value;
            request_layout();
            return *this;
        }

        auto padding(const float h, const float v) -> LayoutContainer& {
            padding_top_ = padding_bottom_ = v;
            padding_left_ = padding_right_ = h;
            request_layout();
            return *this;
        }

        auto padding(const float left, const float top, const float right, const float bottom) -> LayoutContainer& {
            padding_left_   = left;
            padding_top_    = top;
            padding_right_  = right;
            padding_bottom_ = bottom;
            request_layout();
            return *this;
        }

        auto padding_left(const float value) -> LayoutContainer& {
            padding_left_ = value;
            request_layout();
            return *this;
        }

        auto padding_top(const float value) -> LayoutContainer& {
            padding_top_ = value;
            request_layout();
            return *this;
        }

        auto padding_right(const float value) -> LayoutContainer& {
            padding_right_ = value;
            request_layout();
            return *this;
        }

        auto padding_bottom(const float value) -> LayoutContainer& {
            padding_bottom_ = value;
            request_layout();
            return *this;
        }

        auto align_items(const LayoutAlignment value) -> LayoutContainer& {
            align_items_ = value;
            request_layout();
            return *this;
        }

        auto justify_content(const LayoutAlignment value) -> LayoutContainer& {
            justify_content_ = value;
            request_layout();
            return *this;
        }

        auto add(std::unique_ptr<runtime::NanWidget> child) -> LayoutContainer& {
            add_child(std::move(child));
            request_layout();
            return *this;
        }

        [[nodiscard]] auto gap_value() const noexcept -> float {
            return gap_;
        }

        [[nodiscard]] auto padding_left_value() const noexcept -> float {
            return padding_left_;
        }

        [[nodiscard]] auto padding_top_value() const noexcept -> float {
            return padding_top_;
        }

        [[nodiscard]] auto padding_right_value() const noexcept -> float {
            return padding_right_;
        }

        [[nodiscard]] auto padding_bottom_value() const noexcept -> float {
            return padding_bottom_;
        }

        [[nodiscard]] auto align_items_value() const noexcept -> LayoutAlignment {
            return align_items_;
        }

        [[nodiscard]] auto justify_content_value() const noexcept -> LayoutAlignment {
            return justify_content_;
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const float pad_h = padding_left_ + padding_right_;
            const float pad_v = padding_top_ + padding_bottom_;

            const geometry::NanConstraints child_constraints{
                detail::deflate_constraint_min(constraints.min_width(), pad_h),
                detail::deflate_constraint_max(constraints.max_width(), pad_h),
                detail::deflate_constraint_min(constraints.min_height(), pad_v),
                detail::deflate_constraint_max(constraints.max_height(), pad_v),
            };
            const auto child_measure_constraints = child_constraints.loosen();

            m_child_specs.clear();
            for_each_child([&](runtime::NanWidget& child) {
                child.measure(child_constraints_from(child_measure_constraints, child));
                const auto measured = detail::measured_or_preferred_size(child);
                m_child_specs.push_back({
                    .preferred_size = measured,
                    .min_size = child.measured_constraints().min_size(),
                    .max_size = detail::derive_child_max_size(child),
                    .flex_factor = child.flex_factor(),
                });
            });

            // 缓存 preferred_size（用 expand 无约束计算），避免后续 derive_child_max_size 重算
            m_cached_preferred_size = compute_container_size(m_child_specs, geometry::NanConstraints::expand());
            m_pref_size_valid = true;
            set_measured_layout_state(constraints, compute_container_size(m_child_specs, constraints));

            s_measure_count += 1;
        }

        // 子类必须实现此方法
        virtual auto layout() -> void = 0;

        /// 子类返回自身布局轴，用于 preferred_size 计算
        [[nodiscard]] virtual auto layout_axis() const noexcept -> LayoutAxis = 0;

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            if (m_pref_size_valid && !is_layout_dirty()) {
                return m_cached_preferred_size;
            }
            return compute_container_size(collect_child_specs(), geometry::NanConstraints::expand());
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            layout();
            return *this;
        }

    protected:
        auto request_layout() -> void {
            mark_layout_dirty();
        }

        // ── 子类辅助方法（通过 this 访问 NanWidget 的 protected 成员）────

        [[nodiscard]] auto collect_child_specs() const -> std::vector<LayoutChildSpec> {
            std::vector<LayoutChildSpec> specs;
            for_each_child([&](const runtime::NanWidget& child) {
                const auto preferred = detail::measured_or_preferred_size(child);
                specs.push_back({
                    .preferred_size = preferred,
                    .min_size = child.measured_constraints().min_size(),
                    .max_size = detail::derive_child_max_size(child),
                    .flex_factor    = child.flex_factor(),
                });
            });
            return specs;
        }

        [[nodiscard]] auto collect_child_targets() -> std::vector<runtime::NanWidget*> {
            std::vector<runtime::NanWidget*> targets;
            for_each_child([&](runtime::NanWidget& child) {
                targets.push_back(&child);
            });
            return targets;
        }

        [[nodiscard]] auto build_layout_request(const LayoutAxis axis) const -> LayoutRequest {
            LayoutRequest request;
            request.axis              = axis;
            request.container_bounds  = {x(), y(), x() + width(), y() + height()};
            request.constraints       = measured_constraints();
            request.padding           = {padding_left_, padding_top_, padding_right_, padding_bottom_};
            request.gap               = gap_;
            request.cross_alignment   = align_items_;
            request.main_alignment    = justify_content_;
            request.children          = m_child_specs.empty() ? collect_child_specs() : m_child_specs;
            return request;
        }

        auto apply_backend(const LayoutAxis axis) -> void {
            auto targets  = collect_child_targets();
            auto request  = build_layout_request(axis);
            auto frames   = default_layout_backend().compute(request);

            const auto count = std::min(targets.size(), frames.size());
            for (std::size_t index = 0; index < count; ++index) {
                auto* widget = targets[index];
                const auto& frame = frames[index];
                if (widget) {
                    widget->set_bounds(frame.x(), frame.y(), frame.width(), frame.height());
                }
            }
        }

        [[nodiscard]] auto compute_container_size(
            const std::vector<LayoutChildSpec>& specs,
            const geometry::NanConstraints& constraints) const noexcept -> geometry::NanSize {
            float total_main = 0.0f;
            float max_cross  = 0.0f;

            for (const auto& child : specs) {
                const float child_w = child.preferred_size.width();
                const float child_h = child.preferred_size.height();

                if (layout_axis() == LayoutAxis::column) {
                    total_main += child_h;
                    max_cross   = std::max(max_cross, child_w);
                } else if (layout_axis() == LayoutAxis::row) {
                    total_main += child_w;
                    max_cross   = std::max(max_cross, child_h);
                } else {
                    max_cross   = std::max(max_cross, child_w);
                    total_main  = std::max(total_main, child_h);
                }
            }

            const float gap_total = specs.size() > 1 ? gap_ * static_cast<float>(specs.size() - 1) : 0.0f;

            geometry::NanSize size;
            if (layout_axis() == LayoutAxis::column) {
                size = {
                    max_cross + padding_left_ + padding_right_,
                    total_main + gap_total + padding_top_ + padding_bottom_
                };
            } else if (layout_axis() == LayoutAxis::row) {
                size = {
                    total_main + gap_total + padding_left_ + padding_right_,
                    max_cross + padding_top_ + padding_bottom_
                };
            } else {
                size = {
                    max_cross + padding_left_ + padding_right_,
                    total_main + padding_top_ + padding_bottom_
                };
            }

            return constraints.constrain(size);
        }

        float gap_                = 0.0f;
        float padding_top_       = 0.0f;
        float padding_right_     = 0.0f;
        float padding_bottom_    = 0.0f;
        float padding_left_      = 0.0f;
        LayoutAlignment align_items_     = LayoutAlignment::start;
        LayoutAlignment justify_content_ = LayoutAlignment::start;
        std::vector<LayoutChildSpec> m_child_specs;

        // ── preferred_size 缓存 ──────────────────────────────
        mutable geometry::NanSize m_cached_preferred_size{};
        mutable bool              m_pref_size_valid{false};
    };

    // ── Column ───────────────────────────────────────────────
    // 垂直布局容器，子 widget 沿 Y 轴排列。
    class Column final : public LayoutContainer {
    public:
        static auto Create() -> std::unique_ptr<Column> {
            return std::unique_ptr<Column>(new Column());
        }

        auto gap(const float value) -> Column& {
            LayoutContainer::gap(value);
            return *this;
        }

        auto padding(const float value) -> Column& {
            LayoutContainer::padding(value);
            return *this;
        }

        auto padding(const float h, const float v) -> Column& {
            LayoutContainer::padding(h, v);
            return *this;
        }

        auto padding(const float left, const float top, const float right, const float bottom) -> Column& {
            LayoutContainer::padding(left, top, right, bottom);
            return *this;
        }

        auto padding_left(const float value) -> Column& {
            LayoutContainer::padding_left(value);
            return *this;
        }

        auto padding_top(const float value) -> Column& {
            LayoutContainer::padding_top(value);
            return *this;
        }

        auto padding_right(const float value) -> Column& {
            LayoutContainer::padding_right(value);
            return *this;
        }

        auto padding_bottom(const float value) -> Column& {
            LayoutContainer::padding_bottom(value);
            return *this;
        }

        auto align_items(const LayoutAlignment value) -> Column& {
            LayoutContainer::align_items(value);
            return *this;
        }

        auto justify_content(const LayoutAlignment value) -> Column& {
            LayoutContainer::justify_content(value);
            return *this;
        }

        auto add(std::unique_ptr<runtime::NanWidget> child) -> Column& {
            LayoutContainer::add(std::move(child));
            return *this;
        }

        auto layout() -> void override {
            apply_backend(LayoutAxis::column);
        }

        [[nodiscard]] auto layout_axis() const noexcept -> LayoutAxis override {
            return LayoutAxis::column;
        }

    private:
        Column() noexcept = default;
    };

    // ── Row ──────────────────────────────────────────────────
    // 水平布局容器，子 widget 沿 X 轴排列。
    class Row final : public LayoutContainer {
    public:
        static auto Create() -> std::unique_ptr<Row> {
            return std::unique_ptr<Row>(new Row());
        }

        auto gap(const float value) -> Row& {
            LayoutContainer::gap(value);
            return *this;
        }

        auto padding(const float value) -> Row& {
            LayoutContainer::padding(value);
            return *this;
        }

        auto padding(const float h, const float v) -> Row& {
            LayoutContainer::padding(h, v);
            return *this;
        }

        auto padding(const float left, const float top, const float right, const float bottom) -> Row& {
            LayoutContainer::padding(left, top, right, bottom);
            return *this;
        }

        auto padding_left(const float value) -> Row& {
            LayoutContainer::padding_left(value);
            return *this;
        }

        auto padding_top(const float value) -> Row& {
            LayoutContainer::padding_top(value);
            return *this;
        }

        auto padding_right(const float value) -> Row& {
            LayoutContainer::padding_right(value);
            return *this;
        }

        auto padding_bottom(const float value) -> Row& {
            LayoutContainer::padding_bottom(value);
            return *this;
        }

        auto align_items(const LayoutAlignment value) -> Row& {
            LayoutContainer::align_items(value);
            return *this;
        }

        auto justify_content(const LayoutAlignment value) -> Row& {
            LayoutContainer::justify_content(value);
            return *this;
        }

        auto add(std::unique_ptr<runtime::NanWidget> child) -> Row& {
            LayoutContainer::add(std::move(child));
            return *this;
        }

        auto layout() -> void override {
            apply_backend(LayoutAxis::row);
        }

        [[nodiscard]] auto layout_axis() const noexcept -> LayoutAxis override {
            return LayoutAxis::row;
        }

    private:
        Row() noexcept = default;
    };

    // ── Stack ────────────────────────────────────────────────
    // 堆叠布局容器，子 widget 沿 Z 轴重叠排列。
    class Stack final : public LayoutContainer {
    public:
        static auto Create() -> std::unique_ptr<Stack> {
            return std::unique_ptr<Stack>(new Stack());
        }

        auto gap(const float value) -> Stack& {
            LayoutContainer::gap(value);
            return *this;
        }

        auto padding(const float value) -> Stack& {
            LayoutContainer::padding(value);
            return *this;
        }

        auto padding(const float h, const float v) -> Stack& {
            LayoutContainer::padding(h, v);
            return *this;
        }

        auto padding(const float left, const float top, const float right, const float bottom) -> Stack& {
            LayoutContainer::padding(left, top, right, bottom);
            return *this;
        }

        auto align_items(const LayoutAlignment value) -> Stack& {
            LayoutContainer::align_items(value);
            return *this;
        }

        auto justify_content(const LayoutAlignment value) -> Stack& {
            LayoutContainer::justify_content(value);
            return *this;
        }

        auto add(std::unique_ptr<runtime::NanWidget> child) -> Stack& {
            LayoutContainer::add(std::move(child));
            return *this;
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            measure(geometry::NanConstraints::tight(w, h));
            layout();
            return *this;
        }

        auto layout() -> void override {
            apply_backend(LayoutAxis::stack);
        }

        [[nodiscard]] auto layout_axis() const noexcept -> LayoutAxis override {
            return LayoutAxis::stack;
        }

    private:
        Stack() noexcept = default;
    };

} // namespace nandina::layout