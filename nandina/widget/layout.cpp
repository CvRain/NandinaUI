//
// widget/layout — minimal widget-style layout containers.
//

#include "layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace nandina::widget
{
    namespace
    {
        template<typename List>
        void prune_expired(List& items) {
            std::erase_if(items, [](const auto& item) { return item.expired(); });
        }

        [[nodiscard]] auto control_children(scene::NanNode& parent)
            -> std::vector<std::weak_ptr<scene::NanControl>> {
            std::vector<std::weak_ptr<scene::NanControl>> result;
            result.reserve(parent.child_count());
            for (std::size_t index = 0; index < parent.child_count(); ++index) {
                auto* node = parent.get_child(index);
                auto* control = node != nullptr ? node->as_control() : nullptr;
                if (control != nullptr) {
                    result.push_back(
                        std::static_pointer_cast<scene::NanControl>(node->shared_from_this())
                    );
                }
            }
            return result;
        }

        [[nodiscard]] auto child_constraints(scene::LayoutConstraints constraints)
            -> scene::LayoutConstraints {
            return {
                .min_width = 0.0F,
                .max_width = constraints.max_width,
                .min_height = 0.0F,
                .max_height = constraints.max_height,
            };
        }

        [[nodiscard]] auto alignment_offset(LayoutAlignment alignment, float available, float used)
            -> float {
            const auto remaining = std::max(0.0F, available - used);
            switch (alignment) {
                case LayoutAlignment::center:
                    return remaining * 0.5F;
                case LayoutAlignment::end:
                    return remaining;
                case LayoutAlignment::space_between:
                    return 0.0F;
                case LayoutAlignment::start:
                case LayoutAlignment::stretch:
                default:
                    return 0.0F;
            }
        }

        [[nodiscard]] auto distributed_gap(
            LayoutAlignment alignment,
            float base_gap,
            float available,
            float used,
            std::size_t count
        ) -> float {
            if (alignment != LayoutAlignment::space_between || count < 2) {
                return base_gap;
            }
            return base_gap + std::max(0.0F, available - used) / static_cast<float>(count - 1);
        }

        [[nodiscard]] auto
        stretched_extent(LayoutAlignment alignment, float available, float measured) -> float {
            if (alignment == LayoutAlignment::stretch) {
                return std::max(0.0F, available);
            }
            return measured;
        }

        [[nodiscard]] auto main_extent(foundation::NanSize size, LayoutAxis axis) -> float {
            return axis == LayoutAxis::horizontal ? size.get_width() : size.get_height();
        }

        [[nodiscard]] auto cross_extent(foundation::NanSize size, LayoutAxis axis) -> float {
            return axis == LayoutAxis::horizontal ? size.get_height() : size.get_width();
        }

        [[nodiscard]] auto max_main(scene::LayoutConstraints constraints, LayoutAxis axis)
            -> float {
            return axis == LayoutAxis::horizontal ? constraints.max_width : constraints.max_height;
        }

        [[nodiscard]] auto size_from_extents(float main, float cross, LayoutAxis axis)
            -> foundation::NanSize {
            if (axis == LayoutAxis::horizontal) {
                return foundation::NanSize(main, cross);
            }
            return foundation::NanSize(cross, main);
        }

        [[nodiscard]] auto rect_from_extents(
            float main_pos,
            float cross_pos,
            float main_size,
            float cross_size,
            LayoutAxis axis
        ) -> foundation::NanRect {
            if (axis == LayoutAxis::horizontal) {
                return foundation::NanRect::from_xywh(main_pos, cross_pos, main_size, cross_size);
            }
            return foundation::NanRect::from_xywh(cross_pos, main_pos, cross_size, main_size);
        }

        [[nodiscard]] auto effective_limit(float limit) -> float {
            return std::isfinite(limit) ? limit : std::numeric_limits<float>::infinity();
        }

        struct LinearItem {
            std::shared_ptr<scene::NanControl> child;
            scene::LayoutFlexPolicy policy;
            foundation::NanSize measured;
            float base = 0.0F;
            float target = 0.0F;
            float min_main = 0.0F;
            float max_main = std::numeric_limits<float>::infinity();
        };

        [[nodiscard]] auto
        policy_limit(const scene::LayoutConstraints& limits, LayoutAxis axis, bool minimum)
            -> float {
            if (axis == LayoutAxis::horizontal) {
                return minimum ? limits.min_width : effective_limit(limits.max_width);
            }
            return minimum ? limits.min_height : effective_limit(limits.max_height);
        }

        void distribute_linear(std::vector<LinearItem>& items, float item_space) {
            constexpr float epsilon = 0.001F;
            float total = 0.0F;
            for (const auto& item: items) {
                total += item.target;
            }
            if (item_space > total + epsilon) {
                float free = item_space - total;
                for (std::size_t iteration = 0; iteration < items.size() && free > epsilon;
                     ++iteration)
                {
                    float weight = 0.0F;
                    for (const auto& item: items) {
                        if (item.policy.grow > 0.0F && item.target < item.max_main - epsilon) {
                            weight += item.policy.grow;
                        }
                    }
                    if (weight <= 0.0F) {
                        break;
                    }
                    const float before = free;
                    for (auto& item: items) {
                        if (item.policy.grow <= 0.0F || item.target >= item.max_main - epsilon) {
                            continue;
                        }
                        const float add = before * item.policy.grow / weight;
                        const float next = std::min(item.max_main, item.target + add);
                        free -= next - item.target;
                        item.target = next;
                    }
                }
            }
            else if (item_space + epsilon < total) {
                float deficit = total - item_space;
                for (std::size_t iteration = 0; iteration < items.size() && deficit > epsilon;
                     ++iteration)
                {
                    float weight = 0.0F;
                    for (const auto& item: items) {
                        if (item.policy.shrink > 0.0F && item.target > item.min_main + epsilon) {
                            weight += item.policy.shrink * item.base;
                        }
                    }
                    if (weight <= 0.0F) {
                        break;
                    }
                    const float before = deficit;
                    for (auto& item: items) {
                        const float item_weight = item.policy.shrink * item.base;
                        if (item_weight <= 0.0F || item.target <= item.min_main + epsilon) {
                            continue;
                        }
                        const float remove = before * item_weight / weight;
                        const float next = std::max(item.min_main, item.target - remove);
                        deficit -= item.target - next;
                        item.target = next;
                    }
                }
            }
        }

        void layout_linear_items(
            std::vector<std::weak_ptr<scene::NanControl>>& refs,
            LayoutAxis axis,
            float gap,
            LayoutAlignment main_alignment,
            LayoutAlignment cross_alignment,
            foundation::NanSize container_size
        ) {
            prune_expired(refs);
            std::vector<LinearItem> items;
            for (auto& ref: refs) {
                auto child = ref.lock();
                if (!child || !child->visible()) {
                    continue;
                }
                auto policy = child->layout_flex_policy();
                const auto measured = child->measured_size();
                const float min_main = std::max(0.0F, policy_limit(policy.limits, axis, true));
                const float max_main = std::max(min_main, policy_limit(policy.limits, axis, false));
                const float raw_base = policy.basis.value_or(main_extent(measured, axis));
                const float base = std::clamp(raw_base, min_main, max_main);
                items.push_back(
                    LinearItem {
                        .child = std::move(child),
                        .policy = policy,
                        .measured = measured,
                        .base = base,
                        .target = base,
                        .min_main = min_main,
                        .max_main = max_main,
                    }
                );
            }
            if (items.empty()) {
                return;
            }
            const float available_main = main_extent(container_size, axis);
            const float available_cross = cross_extent(container_size, axis);
            const float base_gaps = gap * static_cast<float>(items.size() - 1);
            distribute_linear(items, std::max(0.0F, available_main - base_gaps));
            float used_main = base_gaps;
            for (const auto& item: items) {
                used_main += item.target;
            }
            const auto effective_gap =
                distributed_gap(main_alignment, gap, available_main, used_main, items.size());
            float main_pos = alignment_offset(main_alignment, available_main, used_main);
            for (auto& item: items) {
                const float min_cross = std::max(
                    0.0F,
                    policy_limit(
                        item.policy.limits,
                        axis == LayoutAxis::horizontal ? LayoutAxis::vertical
                                                       : LayoutAxis::horizontal,
                        true
                    )
                );
                const float max_cross = std::max(
                    min_cross,
                    policy_limit(
                        item.policy.limits,
                        axis == LayoutAxis::horizontal ? LayoutAxis::vertical
                                                       : LayoutAxis::horizontal,
                        false
                    )
                );
                const float desired_cross = stretched_extent(
                    cross_alignment,
                    available_cross,
                    cross_extent(item.measured, axis)
                );
                const float child_cross = std::clamp(desired_cross, min_cross, max_cross);
                const float cross_pos =
                    alignment_offset(cross_alignment, available_cross, child_cross);
                const auto rect =
                    rect_from_extents(main_pos, cross_pos, item.target, child_cross, axis);
                (void)item.child->measure_layout(scene::LayoutConstraints::tight(rect.get_size()));
                item.child->layout_to(rect);
                main_pos += item.target + effective_gap;
            }
        }

        struct WrapRun {
            struct Child {
                std::shared_ptr<scene::NanControl> control;
                std::optional<LayoutAlignment> cross_alignment;
            };
            std::vector<Child> children;
            float main = 0.0F;
            float cross = 0.0F;
        };

        [[nodiscard]] auto collect_wrap_runs(
            std::vector<std::weak_ptr<scene::NanControl>>& items,
            scene::LayoutConstraints constraints,
            LayoutAxis axis,
            float gap,
            std::vector<std::pair<std::weak_ptr<scene::NanControl>, LayoutAlignment>>& overrides
        ) -> std::vector<WrapRun> {
            prune_expired(items);
            std::erase_if(overrides, [](const auto& entry) { return entry.first.expired(); });

            std::vector<WrapRun> runs;
            WrapRun current;
            const float limit = effective_limit(max_main(constraints, axis));
            const bool can_wrap = std::isfinite(limit) && limit >= 0.0F;

            for (auto& item: items) {
                auto child = item.lock();
                if (!child || !child->visible()) {
                    continue;
                }

                const auto measured = child->measure_layout(child_constraints(constraints));
                const float child_main = main_extent(measured, axis);
                const float child_cross = cross_extent(measured, axis);
                const float next_main =
                    current.children.empty() ? child_main : current.main + gap + child_main;

                if (can_wrap && !current.children.empty() && next_main > limit) {
                    runs.push_back(std::move(current));
                    current = WrapRun {};
                }

                if (!current.children.empty()) {
                    current.main += gap;
                }
                current.main += child_main;
                current.cross = std::max(current.cross, child_cross);
                const auto override = std::ranges::find_if(overrides, [&child](const auto& entry) {
                    return entry.first.lock() == child;
                });
                current.children.push_back(
                    WrapRun::Child {
                        .control = std::move(child),
                        .cross_alignment = override == overrides.end()
                            ? std::nullopt
                            : std::optional(override->second),
                    }
                );
            }

            if (!current.children.empty()) {
                runs.push_back(std::move(current));
            }
            return runs;
        }
    } // namespace

    Flex::Flex(LayoutAxis axis): axis_(axis) {}

    auto Flex::create(LayoutAxis axis) -> std::shared_ptr<Flex> {
        return std::make_shared<Flex>(axis);
    }

    auto Flex::add(std::shared_ptr<scene::NanControl> child) -> Flex& {
        if (!child) {
            throw std::runtime_error("Flex::add: child is null");
        }
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_axis(LayoutAxis axis) -> Flex& {
        axis_ = axis;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_gap(float gap) -> Flex& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_main_alignment(LayoutAlignment alignment) -> Flex& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_cross_alignment(LayoutAlignment alignment) -> Flex& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::axis() const -> LayoutAxis {
        return axis_;
    }

    auto Flex::gap() const -> float {
        return gap_;
    }

    auto Flex::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Flex::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Flex::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Flex::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto items = control_children(*this);

        float used_main = 0.0F;
        float used_cross = 0.0F;
        std::size_t visible_count = 0;
        for (auto& item: items) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                used_main += gap_;
            }
            const auto measured = child->measure_layout(child_constraints(constraints));
            used_main += main_extent(measured, axis_);
            used_cross = std::max(used_cross, cross_extent(measured, axis_));
            ++visible_count;
        }

        return constraints.constrain(size_from_extents(used_main, used_cross, axis_));
    }

    auto Flex::on_layout() -> void {
        auto items = control_children(*this);
        layout_linear_items(items, axis_, gap_, main_alignment_, cross_alignment_, size());
    }

    void Flex::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    Wrap::Wrap(LayoutAxis axis): axis_(axis) {}

    auto Wrap::create(LayoutAxis axis) -> std::shared_ptr<Wrap> {
        return std::make_shared<Wrap>(axis);
    }

    auto Wrap::add(std::shared_ptr<scene::NanControl> child) -> Wrap& {
        return add(std::move(child), std::nullopt);
    }

    auto Wrap::add(
        std::shared_ptr<scene::NanControl> child,
        std::optional<LayoutAlignment> cross_alignment
    ) -> Wrap& {
        if (!child) {
            throw std::runtime_error("Wrap::add: child is null");
        }
        if (cross_alignment == LayoutAlignment::space_between) {
            throw std::invalid_argument("Wrap child alignment cannot be space_between");
        }
        if (cross_alignment.has_value()) {
            child_cross_alignments_.emplace_back(child, *cross_alignment);
        }
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_child_cross_alignment(
        scene::NanControl& child,
        std::optional<LayoutAlignment> alignment
    ) -> Wrap& {
        if (alignment == LayoutAlignment::space_between) {
            throw std::invalid_argument("Wrap child alignment cannot be space_between");
        }
        std::erase_if(child_cross_alignments_, [&child](const auto& entry) {
            const auto current = entry.first.lock();
            return current == nullptr || current.get() == &child;
        });
        if (alignment.has_value()) {
            child_cross_alignments_.emplace_back(
                std::static_pointer_cast<scene::NanControl>(child.shared_from_this()),
                *alignment
            );
        }
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_axis(LayoutAxis axis) -> Wrap& {
        axis_ = axis;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_gap(float gap) -> Wrap& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_run_gap(float gap) -> Wrap& {
        run_gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_main_alignment(LayoutAlignment alignment) -> Wrap& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_cross_alignment(LayoutAlignment alignment) -> Wrap& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_run_alignment(LayoutAlignment alignment) -> Wrap& {
        run_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::axis() const -> LayoutAxis {
        return axis_;
    }

    auto Wrap::gap() const -> float {
        return gap_;
    }

    auto Wrap::run_gap() const -> float {
        return run_gap_;
    }

    auto Wrap::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Wrap::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    auto Wrap::run_alignment() const -> LayoutAlignment {
        return run_alignment_;
    }

    void Wrap::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Wrap::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto items = control_children(*this);
        const auto runs =
            collect_wrap_runs(items, constraints, axis_, gap_, child_cross_alignments_);
        float used_main = 0.0F;
        float used_cross = 0.0F;
        for (std::size_t i = 0; i < runs.size(); ++i) {
            used_main = std::max(used_main, runs[i].main);
            if (i > 0) {
                used_cross += run_gap_;
            }
            used_cross += runs[i].cross;
        }
        return constraints.constrain(size_from_extents(used_main, used_cross, axis_));
    }

    auto Wrap::on_layout() -> void {
        auto items = control_children(*this);
        auto runs = collect_wrap_runs(
            items,
            scene::LayoutConstraints::tight(size()),
            axis_,
            gap_,
            child_cross_alignments_
        );
        const float available_main = main_extent(size(), axis_);
        const float available_cross = cross_extent(size(), axis_);
        float used_cross = 0.0F;
        for (std::size_t i = 0; i < runs.size(); ++i) {
            if (i > 0) {
                used_cross += run_gap_;
            }
            used_cross += runs[i].cross;
        }
        if (run_alignment_ == LayoutAlignment::stretch && !runs.empty()) {
            const float extra =
                std::max(0.0F, available_cross - used_cross) / static_cast<float>(runs.size());
            for (auto& run: runs) {
                run.cross += extra;
            }
            used_cross = available_cross;
        }

        const auto effective_run_gap =
            distributed_gap(run_alignment_, run_gap_, available_cross, used_cross, runs.size());
        float run_pos = alignment_offset(run_alignment_, available_cross, used_cross);
        for (const auto& run: runs) {
            const auto effective_gap = distributed_gap(
                main_alignment_,
                gap_,
                available_main,
                run.main,
                run.children.size()
            );
            float child_pos = alignment_offset(main_alignment_, available_main, run.main);
            for (const auto& item: run.children) {
                const auto& child = item.control;
                const auto child_size = child->measured_size();
                const auto child_main = main_extent(child_size, axis_);
                const auto alignment = item.cross_alignment.value_or(cross_alignment_);
                const auto child_cross =
                    stretched_extent(alignment, run.cross, cross_extent(child_size, axis_));
                const auto child_cross_pos =
                    run_pos + alignment_offset(alignment, run.cross, child_cross);
                child->layout_to(
                    rect_from_extents(child_pos, child_cross_pos, child_main, child_cross, axis_)
                );
                child_pos += child_main + effective_gap;
            }
            run_pos += run.cross + effective_run_gap;
        }
    }

    void Wrap::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Column::create() -> std::shared_ptr<Column> {
        return std::make_shared<Column>();
    }

    auto Column::add(std::shared_ptr<scene::NanControl> child) -> Column& {
        if (!child) {
            throw std::runtime_error("Column::add: child is null");
        }
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::set_gap(float gap) -> Column& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::set_main_alignment(LayoutAlignment alignment) -> Column& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::set_cross_alignment(LayoutAlignment alignment) -> Column& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::gap() const -> float {
        return gap_;
    }

    auto Column::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Column::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Column::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Column::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto items = control_children(*this);

        float y = 0.0F;
        float max_width = 0.0F;
        std::size_t visible_count = 0;
        for (auto& item: items) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                y += gap_;
            }
            const auto measured = child->measure_layout(child_constraints(constraints));
            y += measured.get_height();
            max_width = std::max(max_width, measured.get_width());
            ++visible_count;
        }

        return constraints.constrain(foundation::NanSize(max_width, y));
    }

    auto Column::on_layout() -> void {
        auto items = control_children(*this);
        layout_linear_items(
            items,
            LayoutAxis::vertical,
            gap_,
            main_alignment_,
            cross_alignment_,
            size()
        );
    }

    void Column::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Row::create() -> std::shared_ptr<Row> {
        return std::make_shared<Row>();
    }

    auto Row::add(std::shared_ptr<scene::NanControl> child) -> Row& {
        if (!child) {
            throw std::runtime_error("Row::add: child is null");
        }
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::set_gap(float gap) -> Row& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::set_main_alignment(LayoutAlignment alignment) -> Row& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::set_cross_alignment(LayoutAlignment alignment) -> Row& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::gap() const -> float {
        return gap_;
    }

    auto Row::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Row::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Row::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Row::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto items = control_children(*this);

        float x = 0.0F;
        float max_height = 0.0F;
        std::size_t visible_count = 0;
        for (auto& item: items) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                x += gap_;
            }
            const auto measured = child->measure_layout(child_constraints(constraints));
            x += measured.get_width();
            max_height = std::max(max_height, measured.get_height());
            ++visible_count;
        }

        return constraints.constrain(foundation::NanSize(x, max_height));
    }

    auto Row::on_layout() -> void {
        auto items = control_children(*this);
        layout_linear_items(
            items,
            LayoutAxis::horizontal,
            gap_,
            main_alignment_,
            cross_alignment_,
            size()
        );
    }

    void Row::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Stack::create() -> std::shared_ptr<Stack> {
        return std::make_shared<Stack>();
    }

    auto Stack::add(std::shared_ptr<scene::NanControl> child) -> Stack& {
        if (!child) {
            throw std::runtime_error("Stack::add: child is null");
        }
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    void Stack::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Stack::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        float max_width = 0.0F;
        float max_height = 0.0F;
        for (std::size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
            if (!child || !child->visible()) {
                continue;
            }
            const auto measured = child->measure_layout(scene::LayoutConstraints::loose());
            max_width = std::max(max_width, measured.get_width());
            max_height = std::max(max_height, measured.get_height());
        }
        return constraints.constrain(foundation::NanSize(max_width, max_height));
    }

    auto Stack::on_layout() -> void {
        for (std::size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
            if (!child || !child->visible()) {
                continue;
            }
            (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
            child->layout_to(local_rect());
        }
    }

    void Stack::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    Padding::Padding(foundation::NanInsets insets): insets_(insets) {}

    auto Padding::create(foundation::NanInsets insets) -> std::shared_ptr<Padding> {
        return std::make_shared<Padding>(insets);
    }

    auto Padding::set_child(std::shared_ptr<scene::NanControl> child) -> Padding& {
        if (!child) {
            throw std::runtime_error("Padding::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Padding::set_padding(foundation::NanInsets insets) -> Padding& {
        insets_ = insets;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Padding::padding() const -> foundation::NanInsets {
        return insets_;
    }

    void Padding::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Padding::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(
                foundation::NanSize(insets_.horizontal_sum(), insets_.vertical_sum())
            );
        }

        const auto measured = child->measure_layout(constraints.deflated(insets_));
        return constraints.constrain(
            foundation::NanSize(
                measured.get_width() + insets_.horizontal_sum(),
                measured.get_height() + insets_.vertical_sum()
            )
        );
    }

    auto Padding::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }

        const auto content_width = std::max(0.0F, width() - insets_.horizontal_sum());
        const auto content_height = std::max(0.0F, height() - insets_.vertical_sum());
        const auto child_size = child->measured_size();
        child->layout_to(
            foundation::NanRect::from_xywh(
                insets_.get_left(),
                insets_.get_top(),
                std::min(child_size.get_width(), content_width),
                std::min(child_size.get_height(), content_height)
            )
        );
    }

    void Padding::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Center::create() -> std::shared_ptr<Center> {
        return std::make_shared<Center>();
    }

    auto Center::set_child(std::shared_ptr<scene::NanControl> child) -> Center& {
        if (!child) {
            throw std::runtime_error("Center::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    void Center::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Center::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(foundation::NanSize::zero());
        }

        const auto measured = child->measure_layout(child_constraints(constraints));
        return constraints.constrain(measured);
    }

    auto Center::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }

        const auto child_size = child->measured_size();
        const auto child_width = std::min(child_size.get_width(), width());
        const auto child_height = std::min(child_size.get_height(), height());
        child->layout_to(
            foundation::NanRect::from_xywh(
                (width() - child_width) * 0.5F,
                (height() - child_height) * 0.5F,
                child_width,
                child_height
            )
        );
    }

    void Center::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    Expanded::Expanded(int flex): flex_(std::max(1, flex)) {}

    auto Expanded::create(int flex) -> std::shared_ptr<Expanded> {
        return std::make_shared<Expanded>(flex);
    }

    auto Expanded::set_child(std::shared_ptr<scene::NanControl> child) -> Expanded& {
        if (!child) {
            throw std::runtime_error("Expanded::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Expanded::set_flex(int flex) -> Expanded& {
        flex_ = std::max(1, flex);
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Expanded::flex() const -> int {
        return flex_;
    }

    auto Expanded::layout_flex_factor() const -> int {
        return flex_;
    }

    void Expanded::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Expanded::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(foundation::NanSize::zero());
        }
        return constraints.constrain(child->measure_layout(child_constraints(constraints)));
    }

    auto Expanded::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }
        (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
        child->layout_to(local_rect());
    }

    void Expanded::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    FlexItem::FlexItem(scene::LayoutFlexPolicy policy) {
        set_policy(policy);
    }

    auto FlexItem::create(scene::LayoutFlexPolicy policy) -> std::shared_ptr<FlexItem> {
        return std::make_shared<FlexItem>(policy);
    }

    auto FlexItem::set_child(std::shared_ptr<scene::NanControl> child) -> FlexItem& {
        if (!child) {
            throw std::runtime_error("FlexItem::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        return *this;
    }

    auto FlexItem::set_policy(scene::LayoutFlexPolicy policy) -> FlexItem& {
        if ((policy.basis.has_value() && (!std::isfinite(*policy.basis) || *policy.basis < 0.0F))
            || !std::isfinite(policy.grow) || policy.grow < 0.0F || !std::isfinite(policy.shrink)
            || policy.shrink < 0.0F)
        {
            throw std::invalid_argument("FlexItem policy requires finite non-negative values");
        }
        policy.limits.min_width = std::max(0.0F, policy.limits.min_width);
        policy.limits.min_height = std::max(0.0F, policy.limits.min_height);
        policy.limits.max_width = std::max(policy.limits.min_width, policy.limits.max_width);
        policy.limits.max_height = std::max(policy.limits.min_height, policy.limits.max_height);
        policy_ = policy;
        mark_layout_dirty();
        return *this;
    }

    auto FlexItem::policy() const -> scene::LayoutFlexPolicy {
        return policy_;
    }
    auto FlexItem::layout_flex_policy() const -> scene::LayoutFlexPolicy {
        return policy_;
    }

    auto FlexItem::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        return child ? child->measure_layout(constraints)
                     : constraints.constrain(foundation::NanSize {});
    }

    auto FlexItem::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }
        (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
        child->layout_to(local_rect());
    }

} // namespace nandina::widget
