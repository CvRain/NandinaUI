//
// widget/layout — minimal widget-style layout containers.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_LAYOUT_HPP
#define NANDINA_EXPERIMENT_WIDGET_LAYOUT_HPP

#include "../foundation/geometry.hpp"
#include "../scene/control.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace nandina::widget
{

    enum class LayoutAlignment {
        start,
        center,
        end,
        stretch,
        space_between,
    };

    enum class LayoutAxis {
        horizontal,
        vertical,
    };

    class Flex: public scene::NanControl {
    public:
        explicit Flex(LayoutAxis axis = LayoutAxis::horizontal);

        [[nodiscard]] static auto create(LayoutAxis axis = LayoutAxis::horizontal)
            -> std::shared_ptr<Flex>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Flex&;
        auto set_axis(LayoutAxis axis) -> Flex&;
        auto set_gap(float gap) -> Flex&;
        auto set_main_alignment(LayoutAlignment alignment) -> Flex&;
        auto set_cross_alignment(LayoutAlignment alignment) -> Flex&;
        [[nodiscard]] auto axis() const -> LayoutAxis;
        [[nodiscard]] auto gap() const -> float;
        [[nodiscard]] auto main_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        LayoutAxis axis_ = LayoutAxis::horizontal;
        float gap_ = 0.0F;
        LayoutAlignment main_alignment_ = LayoutAlignment::start;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
    };

    class Wrap: public scene::NanControl {
    public:
        explicit Wrap(LayoutAxis axis = LayoutAxis::horizontal);

        [[nodiscard]] static auto create(LayoutAxis axis = LayoutAxis::horizontal)
            -> std::shared_ptr<Wrap>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Wrap&;
        auto
        add(std::shared_ptr<scene::NanControl> child,
            std::optional<LayoutAlignment> cross_alignment) -> Wrap&;
        auto set_child_cross_alignment(
            scene::NanControl& child,
            std::optional<LayoutAlignment> alignment
        ) -> Wrap&;
        auto set_axis(LayoutAxis axis) -> Wrap&;
        auto set_gap(float gap) -> Wrap&;
        auto set_run_gap(float gap) -> Wrap&;
        auto set_main_alignment(LayoutAlignment alignment) -> Wrap&;
        auto set_cross_alignment(LayoutAlignment alignment) -> Wrap&;
        auto set_run_alignment(LayoutAlignment alignment) -> Wrap&;
        [[nodiscard]] auto axis() const -> LayoutAxis;
        [[nodiscard]] auto gap() const -> float;
        [[nodiscard]] auto run_gap() const -> float;
        [[nodiscard]] auto main_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto run_alignment() const -> LayoutAlignment;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        LayoutAxis axis_ = LayoutAxis::horizontal;
        float gap_ = 0.0F;
        float run_gap_ = 0.0F;
        LayoutAlignment main_alignment_ = LayoutAlignment::start;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
        LayoutAlignment run_alignment_ = LayoutAlignment::start;
        std::vector<std::pair<std::weak_ptr<scene::NanControl>, LayoutAlignment>>
            child_cross_alignments_;
    };

    using Flow = Wrap;

    class Column: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Column>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Column&;
        auto set_gap(float gap) -> Column&;
        auto set_main_alignment(LayoutAlignment alignment) -> Column&;
        auto set_cross_alignment(LayoutAlignment alignment) -> Column&;
        [[nodiscard]] auto gap() const -> float;
        [[nodiscard]] auto main_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        float gap_ = 0.0F;
        LayoutAlignment main_alignment_ = LayoutAlignment::start;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
    };

    class Row: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Row>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Row&;
        auto set_gap(float gap) -> Row&;
        auto set_main_alignment(LayoutAlignment alignment) -> Row&;
        auto set_cross_alignment(LayoutAlignment alignment) -> Row&;
        [[nodiscard]] auto gap() const -> float;
        [[nodiscard]] auto main_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        float gap_ = 0.0F;
        LayoutAlignment main_alignment_ = LayoutAlignment::start;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
    };

    class Stack: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Stack>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Stack&;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;
    };

    class Padding: public scene::NanControl {
    public:
        explicit Padding(foundation::NanInsets insets = foundation::NanInsets::all(0.0F));

        [[nodiscard]] static auto
        create(foundation::NanInsets insets = foundation::NanInsets::all(0.0F))
            -> std::shared_ptr<Padding>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Padding&;
        auto set_padding(foundation::NanInsets insets) -> Padding&;
        [[nodiscard]] auto padding() const -> foundation::NanInsets;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        foundation::NanInsets insets_;
        std::weak_ptr<scene::NanControl> child_;
    };

    class Center: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Center>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Center&;
        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        std::weak_ptr<scene::NanControl> child_;
    };

    class Expanded: public scene::NanControl {
    public:
        explicit Expanded(int flex = 1);

        [[nodiscard]] static auto create(int flex = 1) -> std::shared_ptr<Expanded>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Expanded&;
        auto set_flex(int flex) -> Expanded&;
        [[nodiscard]] auto flex() const -> int;
        [[nodiscard]] auto layout_flex_factor() const -> int override;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        int flex_ = 1;
        std::weak_ptr<scene::NanControl> child_;
    };

    class FlexItem: public scene::NanControl {
    public:
        explicit FlexItem(scene::LayoutFlexPolicy policy = {});
        [[nodiscard]] static auto create(scene::LayoutFlexPolicy policy = {})
            -> std::shared_ptr<FlexItem>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> FlexItem&;
        auto set_policy(scene::LayoutFlexPolicy policy) -> FlexItem&;
        [[nodiscard]] auto policy() const -> scene::LayoutFlexPolicy;
        [[nodiscard]] auto layout_flex_policy() const -> scene::LayoutFlexPolicy override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;

    private:
        scene::LayoutFlexPolicy policy_;
        std::weak_ptr<scene::NanControl> child_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LAYOUT_HPP
