module;

#include <memory>

export module nandina.layout.flex_widgets;

export import nandina.foundation.nan_size;
export import nandina.runtime.nan_widget;

export namespace nandina::layout {

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

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            for_each_child([&](runtime::NanWidget& c) {
                c.set_bounds(0.0f, 0.0f, w, h);
            });
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
            return *this;
        }

        auto height(const float h) -> SizedBox& {
            fixed_h_ = h;
            return *this;
        }

        auto size(const geometry::NanSize& s) -> SizedBox& {
            fixed_w_ = s.width();
            fixed_h_ = s.height();
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

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            const float actual_w = fixed_w_ > 0.0f ? fixed_w_ : w;
            const float actual_h = fixed_h_ > 0.0f ? fixed_h_ : h;
            NanWidget::set_bounds(x, y, actual_w, actual_h);
            for_each_child([&](runtime::NanWidget& c) {
                c.set_bounds(0.0f, 0.0f, actual_w, actual_h);
            });
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

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            for_each_child([&](runtime::NanWidget& c) {
                const auto pref     = c.preferred_size();
                const float child_w = pref.width();
                const float child_h = pref.height();
                const float cx      = x + (w - child_w) * 0.5f;
                const float cy      = y + (h - child_h) * 0.5f;
                c.set_bounds(cx, cy, child_w, child_h);
            });
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
            return *this;
        }

        auto horizontal(const float v) -> Padding& {
            left_ = right_ = v;
            return *this;
        }

        auto vertical(const float v) -> Padding& {
            top_ = bottom_ = v;
            return *this;
        }

        auto top(const float v) -> Padding& {
            top_ = v;
            return *this;
        }

        auto right(const float v) -> Padding& {
            right_ = v;
            return *this;
        }

        auto bottom(const float v) -> Padding& {
            bottom_ = v;
            return *this;
        }

        auto left(const float v) -> Padding& {
            left_ = v;
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

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            for_each_child([&](runtime::NanWidget& c) {
                c.set_bounds(
                    x + left_,
                    y + top_,
                    w - left_ - right_,
                    h - top_ - bottom_);
            });
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