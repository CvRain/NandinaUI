module;

#include <memory>
#include <vector>

export module nandina.layout.container;

export import nandina.foundation.nan_rect;
export import nandina.foundation.nan_size;
export import nandina.runtime.nan_widget;
export import nandina.layout.core;

export namespace nandina::layout {

    // ── LayoutContainer ──────────────────────────────────────
    // 抽象基类，提供 gap/padding/align/justify 等布局属性设置。
    // Column/Row/Stack 继承此类并重写 layout() 方法。
    class LayoutContainer : public runtime::NanWidget {
    public:
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

        // 子类必须实现此方法
        virtual auto layout() -> void = 0;

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            layout();
            return *this;
        }

    protected:
        auto request_layout() -> void {
            mark_dirty();
            if (width() > 0.0f || height() > 0.0f) {
                layout();
            }
        }

        // ── 子类辅助方法（通过 this 访问 NanWidget 的 protected 成员）────

        [[nodiscard]] auto collect_child_specs() const -> std::vector<LayoutChildSpec> {
            std::vector<LayoutChildSpec> specs;
            for_each_child([&](const runtime::NanWidget& child) {
                const float pw = child.preferred_size().width();
                const float ph = child.preferred_size().height();
                specs.push_back({
                    .preferred_size = {pw > 0.0f ? pw : child.width(), ph > 0.0f ? ph : child.height()},
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
            request.container_bounds  = {x(), y(), width(), height()};
            request.padding           = {padding_left_, padding_top_, padding_right_, padding_bottom_};
            request.gap               = gap_;
            request.cross_alignment   = align_items_;
            request.main_alignment    = justify_content_;
            request.children          = collect_child_specs();
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

        float gap_                = 0.0f;
        float padding_top_       = 0.0f;
        float padding_right_     = 0.0f;
        float padding_bottom_    = 0.0f;
        float padding_left_      = 0.0f;
        LayoutAlignment align_items_     = LayoutAlignment::start;
        LayoutAlignment justify_content_ = LayoutAlignment::start;
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

        auto layout() -> void override {
            apply_backend(LayoutAxis::stack);
        }

    private:
        Stack() noexcept = default;
    };

} // namespace nandina::layout