module;

#include <algorithm>
#include <memory>

export module nandina.layout.flex_widgets;

export import nandina.foundation.nan_constraints;
export import nandina.foundation.nan_size;
export import nandina.runtime.nan_widget;

export namespace nandina::layout {

    namespace detail {
        [[nodiscard]] inline auto measured_or_preferred_size(const runtime::NanWidget& child) noexcept -> geometry::NanSize {
            const auto measured = child.measured_size();
            const auto preferred = child.preferred_size();
            return {
                measured.width() > 0.0f ? measured.width() : preferred.width(),
                measured.height() > 0.0f ? measured.height() : preferred.height(),
            };
        }

        [[nodiscard]] inline auto deflate_constraints(
            const geometry::NanConstraints& constraints,
            const float horizontal,
            const float vertical) noexcept -> geometry::NanConstraints {
            const auto deflate_min = [](const float value, const float delta) {
                return std::max(0.0f, value - delta);
            };
            const auto deflate_max = [](const float value, const float delta) {
                if (value == geometry::NanConstraints::k_infinity) {
                    return value;
                }
                return std::max(0.0f, value - delta);
            };

            return geometry::NanConstraints{
                deflate_min(constraints.min_width(), horizontal),
                deflate_max(constraints.max_width(), horizontal),
                deflate_min(constraints.min_height(), vertical),
                deflate_max(constraints.max_height(), vertical),
            };
        }
    }

    // ── Spacer ───────────────────────────────────────────────
    // 弹性空白，在 Row/Column 中占据剩余空间。
    // flex_factor() 决定占据多少份剩余空间。
    class Spacer final : public runtime::NanWidget {
    public:
        static auto Create(const int flex = 1) -> std::unique_ptr<Spacer> {
            return std::unique_ptr<Spacer>(new Spacer(flex));
        }

        [[nodiscard]] auto flex_factor() const noexcept -> int override {
            return flex_;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return geometry::NanSize{0.0f, 0.0f};
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            set_measured_layout_state(
                constraints,
                constraints.constrain(geometry::NanSize{0.0f, 0.0f}));
        }

    private:
        explicit Spacer(const int flex) noexcept
            : flex_(flex) {
        }

        int flex_ = 1;
    };

    // ── Expanded ─────────────────────────────────────────────
    // 包裹一个子 widget，使其在布局中占据弹性空间。
    // 用法：
    //   auto exp = Expanded::Create(2);
    //   exp->child(std::move(myWidget));
    class Expanded final : public runtime::NanWidget {
    public:
        static auto Create(const int flex = 1) -> std::unique_ptr<Expanded> {
            return std::unique_ptr<Expanded>(new Expanded(flex));
        }

        auto child(std::unique_ptr<runtime::NanWidget> w) -> Expanded& {
            add_child(std::move(w));
            return *this;
        }

        [[nodiscard]] auto flex_factor() const noexcept -> int override {
            return flex_;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize preferred{0.0f, 0.0f};
            for_each_child([&](runtime::NanWidget& c) {
                preferred = c.preferred_size();
            });
            return preferred;
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            geometry::NanSize measured{};
            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.measure(constraints);
                measured = detail::measured_or_preferred_size(child_widget);
            });
            set_measured_layout_state(constraints, constraints.constrain(measured));
        }

        auto layout() -> void override {
            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.set_bounds(
                    x(),
                    y(),
                    runtime::NanWidget::width(),
                    runtime::NanWidget::height());
            });
            clear_layout_dirty();
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            layout();
            return *this;
        }

    private:
        explicit Expanded(const int flex) noexcept
            : flex_(flex) {
        }

        int flex_ = 1;
    };

    // ── SizedBox ─────────────────────────────────────────────
    // 固定尺寸容器。如果 fixed_w_/fixed_h_ > 0 则覆盖父布局分配的尺寸。
    class SizedBox final : public runtime::NanWidget {
    public:
        static auto Create() -> std::unique_ptr<SizedBox> {
            return std::unique_ptr<SizedBox>(new SizedBox());
        }

        auto width(const float w) -> SizedBox& {
            fixed_w_ = w;
            mark_layout_dirty();
            return *this;
        }

        auto height(const float h) -> SizedBox& {
            fixed_h_ = h;
            mark_layout_dirty();
            return *this;
        }

        auto size(const geometry::NanSize& s) -> SizedBox& {
            fixed_w_ = s.width();
            fixed_h_ = s.height();
            mark_layout_dirty();
            return *this;
        }

        auto child(std::unique_ptr<runtime::NanWidget> w) -> SizedBox& {
            add_child(std::move(w));
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize preferred{0.0f, 0.0f};
            for_each_child([&](const runtime::NanWidget& c) {
                preferred = c.preferred_size();
            });
            return {
                fixed_w_ > 0.0f ? fixed_w_ : preferred.width(),
                fixed_h_ > 0.0f ? fixed_h_ : preferred.height(),
            };
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const float min_width = fixed_w_ > 0.0f ? fixed_w_ : constraints.min_width();
            const float max_width = fixed_w_ > 0.0f ? fixed_w_ : constraints.max_width();
            const float min_height = fixed_h_ > 0.0f ? fixed_h_ : constraints.min_height();
            const float max_height = fixed_h_ > 0.0f ? fixed_h_ : constraints.max_height();

            const geometry::NanConstraints child_constraints{min_width, max_width, min_height, max_height};

            geometry::NanSize measured{
                fixed_w_ > 0.0f ? fixed_w_ : 0.0f,
                fixed_h_ > 0.0f ? fixed_h_ : 0.0f,
            };

            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.measure(child_constraints);
                const auto child_size = detail::measured_or_preferred_size(child_widget);
                if (fixed_w_ <= 0.0f) {
                    measured = {child_size.width(), measured.height()};
                }
                if (fixed_h_ <= 0.0f) {
                    measured = {measured.width(), child_size.height()};
                }
            });

            set_measured_layout_state(constraints, constraints.constrain(measured));
        }

        auto layout() -> void override {
            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.set_bounds(
                    x(),
                    y(),
                    runtime::NanWidget::width(),
                    runtime::NanWidget::height());
            });
            clear_layout_dirty();
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            const float actual_w = fixed_w_ > 0.0f ? fixed_w_ : w;
            const float actual_h = fixed_h_ > 0.0f ? fixed_h_ : h;
            NanWidget::set_bounds(x, y, actual_w, actual_h);
            layout();
            return *this;
        }

    private:
        SizedBox() noexcept = default;

        float fixed_w_ = 0.0f;
        float fixed_h_ = 0.0f;
    };

    // ── Center ───────────────────────────────────────────────
    // 在自身范围内居中唯一子 widget（子 widget 保持自身大小）。
    class Center final : public runtime::NanWidget {
    public:
        static auto Create() -> std::unique_ptr<Center> {
            return std::unique_ptr<Center>(new Center());
        }

        auto child(std::unique_ptr<runtime::NanWidget> w) -> Center& {
            add_child(std::move(w));
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize preferred{0.0f, 0.0f};
            for_each_child([&](const runtime::NanWidget& c) {
                preferred = c.preferred_size();
            });
            return preferred;
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto child_constraints = constraints.loosen();
            geometry::NanSize measured{};
            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.measure(child_constraints);
                measured = detail::measured_or_preferred_size(child_widget);
            });
            set_measured_layout_state(constraints, constraints.constrain(measured));
        }

        auto layout() -> void override {
            for_each_child([&](runtime::NanWidget& child_widget) {
                const auto child_size = detail::measured_or_preferred_size(child_widget);
                const float child_w = std::min(child_size.width(), width());
                const float child_h = std::min(child_size.height(), height());
                const float cx = x() + (width() - child_w) * 0.5f;
                const float cy = y() + (height() - child_h) * 0.5f;
                child_widget.set_bounds(cx, cy, child_w, child_h);
            });
            clear_layout_dirty();
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            layout();
            return *this;
        }

    private:
        Center() noexcept = default;
    };

    // ── Padding ──────────────────────────────────────────────
    // 在子 widget 周围添加内边距。
    class Padding final : public runtime::NanWidget {
    public:
        static auto Create() -> std::unique_ptr<Padding> {
            return std::unique_ptr<Padding>(new Padding());
        }

        auto all(const float v) -> Padding& {
            top_ = right_ = bottom_ = left_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto horizontal(const float v) -> Padding& {
            left_ = right_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto vertical(const float v) -> Padding& {
            top_ = bottom_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto top(const float v) -> Padding& {
            top_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto right(const float v) -> Padding& {
            right_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto bottom(const float v) -> Padding& {
            bottom_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto left(const float v) -> Padding& {
            left_ = v;
            mark_layout_dirty();
            return *this;
        }

        auto padding(const float v) -> Padding& {
            return all(v);
        }

        auto padding(const float horizontal_v, const float vertical_v) -> Padding& {
            left_ = right_ = horizontal_v;
            top_ = bottom_ = vertical_v;
            return *this;
        }

        auto padding(const float l, const float t, const float r, const float b) -> Padding& {
            left_   = l;
            top_    = t;
            right_  = r;
            bottom_ = b;
            return *this;
        }

        auto child(std::unique_ptr<runtime::NanWidget> w) -> Padding& {
            add_child(std::move(w));
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize preferred{0.0f, 0.0f};
            for_each_child([&](const runtime::NanWidget& c) {
                preferred = c.preferred_size();
            });
            return {
                preferred.width() + left_ + right_,
                preferred.height() + top_ + bottom_,
            };
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto child_constraints = detail::deflate_constraints(constraints, left_ + right_, top_ + bottom_);
            geometry::NanSize child_size{};
            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.measure(child_constraints);
                child_size = detail::measured_or_preferred_size(child_widget);
            });

            set_measured_layout_state(
                constraints,
                constraints.constrain(geometry::NanSize{
                    child_size.width() + left_ + right_,
                    child_size.height() + top_ + bottom_,
                }));
        }

        auto layout() -> void override {
            const float child_w = std::max(0.0f, width() - left_ - right_);
            const float child_h = std::max(0.0f, height() - top_ - bottom_);
            for_each_child([&](runtime::NanWidget& child_widget) {
                child_widget.set_bounds(
                    x() + left_,
                    y() + top_,
                    child_w,
                    child_h);
            });
            clear_layout_dirty();
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            layout();
            return *this;
        }

    private:
        Padding() noexcept = default;

        float top_    = 0.0f;
        float right_  = 0.0f;
        float bottom_ = 0.0f;
        float left_   = 0.0f;
    };

} // namespace nandina::layout