#include "anchor_canvas.hpp"

#include "scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace nandina::scene
{
    namespace
    {
        [[nodiscard]] auto label(const NanNode& node) -> std::string {
            std::ostringstream out;
            out << "'" << node.name() << "' @" << &node;
            return out.str();
        }

        [[noreturn]] void fail(const NanNode& source, const std::string& reason) {
            throw std::logic_error("anchors: " + label(source) + ": " + reason);
        }

        [[nodiscard]] auto uses_percent(const LayoutLength& length) -> bool {
            return std::holds_alternative<PercentLength>(length);
        }

        [[nodiscard]] auto uses_percent(const std::optional<LayoutLength>& length) -> bool {
            return length.has_value() && uses_percent(*length);
        }

        [[nodiscard]] auto is_tight_axis(float minimum, float maximum) -> bool {
            return std::isfinite(maximum) && maximum >= 0.0F
                && std::abs(minimum - maximum) <= foundation::nan_epsilon;
        }

        [[nodiscard]] auto has_definite_self_extent(const NanControl& node, bool horizontal)
            -> bool {
            const auto constraints = node.last_layout_constraints();
            const float lo = horizontal ? constraints.min_width : constraints.min_height;
            const float hi = horizontal ? constraints.max_width : constraints.max_height;
            if (is_tight_axis(lo, hi) && (node.parent() == nullptr || lo > 0.0F)) {
                return true;
            }
            const auto& length = horizontal ? node.size_spec().width : node.size_spec().height;
            if (std::holds_alternative<LogicalLength>(length)) {
                return true;
            }
            if (node.size_spec().aspect_ratio.has_value()) {
                const auto& other = horizontal ? node.size_spec().height : node.size_spec().width;
                const float other_lo = horizontal ? constraints.min_height : constraints.min_width;
                const float other_hi = horizontal ? constraints.max_height : constraints.max_width;
                if (std::holds_alternative<LogicalLength>(other)
                    || (is_tight_axis(other_lo, other_hi)
                        && (node.parent() == nullptr || other_lo > 0.0F)))
                {
                    return true;
                }
            }
            if ((std::holds_alternative<FillLength>(length)
                 || std::holds_alternative<PercentLength>(length))
                && std::isfinite(hi) && hi >= 0.0F)
            {
                return true;
            }
            return false;
        }

        [[nodiscard]] auto
        will_receive_assigned_extent(const NanControl& node, const bool horizontal) -> bool {
            const NanControl* current = &node;
            while (const auto* parent_node = current->parent()) {
                const auto* parent_ctrl = parent_node->as_control();
                if (parent_ctrl == nullptr) {
                    return false;
                }
                const auto mode = parent_ctrl->child_axis_assignment(*current, horizontal);
                if (mode == ChildAxisAssignment::assigned) {
                    return true;
                }
                if (mode == ChildAxisAssignment::none) {
                    return false;
                }
                if (has_definite_self_extent(*parent_ctrl, horizontal)) {
                    return true;
                }
                current = parent_ctrl;
            }
            return false;
        }

        [[nodiscard]] auto coordinate(const foundation::NanRect& rect, AnchorLine line) -> float {
            switch (line) {
                case AnchorLine::left:
                    return rect.get_x();
                case AnchorLine::right:
                    return rect.get_x() + rect.get_width();
                case AnchorLine::top:
                    return rect.get_y();
                case AnchorLine::bottom:
                    return rect.get_y() + rect.get_height();
                case AnchorLine::horizontal_center:
                    return rect.get_x() + rect.get_width() * 0.5F;
                case AnchorLine::vertical_center:
                    return rect.get_y() + rect.get_height() * 0.5F;
            }
            throw std::logic_error("anchors: invalid line");
        }

        void validate_axis(
            const NanControl& node,
            const LayoutLength& length,
            const std::optional<LayoutLength>& minimum,
            const std::optional<LayoutLength>& maximum,
            float basis,
            std::optional<float> span
        ) {
            if (std::holds_alternative<FillLength>(length)) {
                fail(node, "fill is a flow size; use two edge anchors instead");
            }
            if (span && !std::holds_alternative<ContentLength>(length)) {
                fail(node, "two edge anchors conflict with explicit or percentage size");
            }
            const auto low = minimum ? resolve_layout_length(*minimum, basis).value_or(0.0F) : 0.0F;
            const auto high = maximum ? resolve_layout_length(*maximum, basis).value_or(basis)
                                      : std::numeric_limits<float>::infinity();
            if (!std::isfinite(low) || std::isnan(high) || low > high) {
                fail(node, "conflicting or non-finite min/max size");
            }
            if (span && (!std::isfinite(*span) || *span < low || *span > high)) {
                fail(node, "edge span is negative, non-finite or violates min/max size");
            }
        }
    } // namespace

    auto AnchorCanvas::accepts_child(const NanNode& child) const -> bool {
        return child.as_control() != nullptr;
    }

    auto AnchorCanvas::child_axis_assignment(const NanControl& child, const bool horizontal) const
        -> ChildAxisAssignment {
        if (child.parent() != this) {
            return ChildAxisAssignment::none;
        }
        const auto& spec = child.anchors();
        const bool two_edges = horizontal ? (spec.left && spec.right) : (spec.top && spec.bottom);
        if (!two_edges) {
            return ChildAxisAssignment::none;
        }
        const auto constraints = last_layout_constraints();
        const float hi = horizontal ? constraints.max_width : constraints.max_height;
        return (std::isfinite(hi) && hi >= 0.0F) ? ChildAxisAssignment::assigned
                                                 : ChildAxisAssignment::pass_through;
    }

    auto AnchorCanvas::has_definite_canvas_axis(const bool horizontal) const -> bool {
        const auto raw = last_layout_constraints();
        const float lo = horizontal ? raw.min_width : raw.min_height;
        const float hi = horizontal ? raw.max_width : raw.max_height;
        const auto& spec_len = horizontal ? size_spec().width : size_spec().height;
        if (std::holds_alternative<LogicalLength>(spec_len)) {
            return true;
        }
        if ((std::holds_alternative<FillLength>(spec_len)
             || std::holds_alternative<PercentLength>(spec_len))
            && std::isfinite(hi) && hi >= 0.0F)
        {
            return true;
        }
        if (is_tight_axis(lo, hi)
            && (parent() == nullptr || lo > 0.0F
                || will_receive_assigned_extent(*this, horizontal)))
        {
            return true;
        }
        if (size_spec().aspect_ratio.has_value()) {
            const auto& other_len = horizontal ? size_spec().height : size_spec().width;
            const float other_lo = horizontal ? raw.min_height : raw.min_width;
            const float other_hi = horizontal ? raw.max_height : raw.max_width;
            if (std::holds_alternative<LogicalLength>(other_len)) {
                return true;
            }
            if ((std::holds_alternative<FillLength>(other_len)
                 || std::holds_alternative<PercentLength>(other_len))
                && std::isfinite(other_hi) && other_hi >= 0.0F)
            {
                return true;
            }
            if (is_tight_axis(other_lo, other_hi)
                && (parent() == nullptr || other_lo > 0.0F
                    || will_receive_assigned_extent(*this, !horizontal)))
            {
                return true;
            }
        }
        return false;
    }

    auto AnchorCanvas::children_need_canvas_extent(const bool horizontal) const -> bool {
        for (std::size_t i = 0; i < child_count(); ++i) {
            const auto* node = get_child(i)->as_control();
            if (node == nullptr) {
                continue;
            }
            const auto& spec = node->anchors();
            const auto& sizing = node->size_spec();
            if (horizontal) {
                if ((spec.right && spec.right->parent)
                    || (spec.horizontal_center && spec.horizontal_center->parent)
                    || uses_percent(sizing.width) || uses_percent(sizing.min_width)
                    || uses_percent(sizing.max_width))
                {
                    return true;
                }
            }
            else {
                if ((spec.bottom && spec.bottom->parent)
                    || (spec.vertical_center && spec.vertical_center->parent)
                    || uses_percent(sizing.height) || uses_percent(sizing.min_height)
                    || uses_percent(sizing.max_height))
                {
                    return true;
                }
            }
        }
        return false;
    }

    auto AnchorCanvas::rest_solve_axis(const bool horizontal) const -> SolveAxis {
        const float current_extent = horizontal ? size().get_width() : size().get_height();
        const float saved_limit = horizontal ? last_content_max_width_ : last_content_max_height_;
        const bool deferred = horizontal ? deferred_width_ : deferred_height_;
        const auto& spec_len = horizontal ? size_spec().width : size_spec().height;

        if (const auto* logical = std::get_if<LogicalLength>(&spec_len)) {
            const float extent = has_measure_state_ && !deferred ? current_extent : logical->value;
            return SolveAxis {
                .mode = AxisMode::definite,
                .extent = extent,
                .max_limit = extent,
            };
        }
        if (std::holds_alternative<FillLength>(spec_len)
            || std::holds_alternative<PercentLength>(spec_len))
        {
            if (has_measure_state_ && !deferred && parent() == measured_parent_) {
                return SolveAxis {
                    .mode = AxisMode::definite,
                    .extent = current_extent,
                    .max_limit = current_extent,
                };
            }
            return SolveAxis {
                .mode = AxisMode::content,
                .extent = 0.0F,
                .max_limit = saved_limit,
            };
        }

        // ContentLength: only definite if the most recent valid measure pass for this exact parent
        // and content spec solved this axis as definite (and is not currently deferred).
        const bool had_content_spec = horizontal ? had_content_spec_x_ : had_content_spec_y_;
        const bool was_definite = has_measure_state_ && !deferred && parent() == measured_parent_
            && had_content_spec && !(horizontal ? content_sized_x_ : content_sized_y_);
        if (was_definite) {
            return SolveAxis {
                .mode = AxisMode::definite,
                .extent = current_extent,
                .max_limit = current_extent,
            };
        }
        return SolveAxis {
            .mode = AxisMode::content,
            .extent = 0.0F,
            .max_limit = saved_limit,
        };
    }

    auto AnchorCanvas::on_measure(foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        if (provisional_relayout_active()) {
            if (has_committed_layout_) {
                return constraints.constrain(committed_size_);
            }
            const bool tight_x = is_tight_axis(constraints.min_width, constraints.max_width)
                && constraints.min_width > 0.0F;
            const bool tight_y = is_tight_axis(constraints.min_height, constraints.max_height)
                && constraints.min_height > 0.0F;
            deferred_width_ = !tight_x;
            deferred_height_ = !tight_y;
            return foundation::NanSize(
                tight_x ? constraints.max_width : std::max(0.0F, constraints.min_width),
                tight_y ? constraints.max_height : std::max(0.0F, constraints.min_height)
            );
        }

        const bool definite_x = has_definite_canvas_axis(true);
        const bool definite_y = has_definite_canvas_axis(false);

        if (definite_x && definite_y) {
            // Preserve content-axis classification when a linear parent echoes our own
            // measured content size back as a tight rect in layout_linear_items().
            const bool echoed_content_x = content_sized_x_ && parent() != nullptr
                && parent() == measured_parent_
                && std::holds_alternative<ContentLength>(size_spec().width)
                && !children_need_canvas_extent(true) && !will_receive_assigned_extent(*this, true)
                && std::abs(constraints.max_width - measured_size().get_width())
                    <= foundation::nan_epsilon;
            const bool echoed_content_y = content_sized_y_ && parent() != nullptr
                && parent() == measured_parent_
                && std::holds_alternative<ContentLength>(size_spec().height)
                && !children_need_canvas_extent(false)
                && !will_receive_assigned_extent(*this, false)
                && std::abs(constraints.max_height - measured_size().get_height())
                    <= foundation::nan_epsilon;

            measured_parent_ = parent();
            had_content_spec_x_ = std::holds_alternative<ContentLength>(size_spec().width);
            had_content_spec_y_ = std::holds_alternative<ContentLength>(size_spec().height);
            content_sized_x_ = echoed_content_x;
            content_sized_y_ = echoed_content_y;
            deferred_width_ = false;
            deferred_height_ = false;
            has_measure_state_ = true;
            return foundation::NanSize(constraints.max_width, constraints.max_height);
        }

        const bool needs_definite_x = !std::holds_alternative<ContentLength>(size_spec().width)
            || children_need_canvas_extent(true);
        const bool needs_definite_y = !std::holds_alternative<ContentLength>(size_spec().height)
            || children_need_canvas_extent(false);

        bool defer_x = false;
        bool defer_y = false;
        if (!definite_x && needs_definite_x) {
            if (will_receive_assigned_extent(*this, true)) {
                defer_x = true;
            }
            else {
                fail(
                    *this,
                    "AnchorCanvas width requires a definite parent bound (avoid fill/percent on unbounded axes, or wrap in Expanded / set explicit width when children use parent right/center or percentage width)"
                );
            }
        }
        if (!definite_y && needs_definite_y) {
            if (will_receive_assigned_extent(*this, false)) {
                defer_y = true;
            }
            else {
                fail(
                    *this,
                    "AnchorCanvas height requires a definite parent bound (avoid fill/percent on unbounded axes, or wrap in Expanded / set explicit height when children use parent bottom/center or percentage height)"
                );
            }
        }

        deferred_width_ = defer_x;
        deferred_height_ = defer_y;
        measured_parent_ = parent();
        had_content_spec_x_ = std::holds_alternative<ContentLength>(size_spec().width);
        had_content_spec_y_ = std::holds_alternative<ContentLength>(size_spec().height);
        has_measure_state_ = true;
        if (defer_x || defer_y) {
            return foundation::NanSize(
                definite_x ? constraints.max_width : std::max(0.0F, constraints.min_width),
                definite_y ? constraints.max_height : std::max(0.0F, constraints.min_height)
            );
        }

        if (!definite_x) {
            last_content_max_width_ = constraints.max_width;
        }
        if (!definite_y) {
            last_content_max_height_ = constraints.max_height;
        }
        content_sized_x_ = !definite_x;
        content_sized_y_ = !definite_y;
        const auto solve_x = definite_x ? SolveAxis {
            .mode = AxisMode::definite,
            .extent = constraints.max_width,
            .max_limit = constraints.max_width,
        }
                                        : SolveAxis {
                                              .mode = AxisMode::content,
                                              .extent = 0.0F,
                                              .max_limit = constraints.max_width,
                                          };
        const auto solve_y = definite_y ? SolveAxis {
            .mode = AxisMode::definite,
            .extent = constraints.max_height,
            .max_limit = constraints.max_height,
        }
                                        : SolveAxis {
                                              .mode = AxisMode::content,
                                              .extent = 0.0F,
                                              .max_limit = constraints.max_height,
                                          };
        const auto outcome = solve(solve_x, solve_y);
        return constraints.constrain(
            foundation::NanSize(
                definite_x ? constraints.max_width : outcome.content_bounds.get_width(),
                definite_y ? constraints.max_height : outcome.content_bounds.get_height()
            )
        );
    }

    auto AnchorCanvas::solve(
        const SolveAxis axis_x,
        const SolveAxis axis_y,
        std::span<const Update> updates
    ) -> SolveOutcome {
        if (axis_x.mode == AxisMode::definite
            && (!std::isfinite(axis_x.extent) || axis_x.extent < 0.0F))
        {
            fail(*this, "AnchorCanvas requires a definite, non-negative width");
        }
        if (axis_y.mode == AxisMode::definite
            && (!std::isfinite(axis_y.extent) || axis_y.extent < 0.0F))
        {
            fail(*this, "AnchorCanvas requires a definite, non-negative height");
        }
        if ((axis_x.mode == AxisMode::content
             && (std::isnan(axis_x.max_limit) || axis_x.max_limit < 0.0F))
            || (axis_y.mode == AxisMode::content
                && (std::isnan(axis_y.max_limit) || axis_y.max_limit < 0.0F)))
        {
            fail(*this, "AnchorCanvas received invalid content measurement bounds");
        }
        std::vector<NanControl*> nodes;
        std::vector<const AnchorSpec*> specs;
        std::unordered_map<const NanNode*, std::size_t> indices;
        for (std::size_t i = 0; i < child_count(); ++i) {
            auto* node = get_child(i)->as_control();
            indices.emplace(node, i);
            nodes.push_back(node);
            specs.push_back(&node->anchors());
        }
        std::unordered_set<const NanControl*> updated;
        for (const auto& update: updates) {
            if (!update.node || !indices.contains(update.node.get())) {
                fail(*this, "batch update must refer to a direct child");
            }
            if (!updated.insert(update.node.get()).second) {
                fail(*update.node, "duplicate batch update");
            }
            specs[indices.at(update.node.get())] = &update.anchors;
        }

        // Resolve every weak target before sorting. A parent target is the source's
        // direct parent, never a search for an ancestor canvas.
        const auto resolve_target = [&](const NanControl& source,
                                        const AnchorTarget& target) -> const NanNode* {
            std::shared_ptr<NanNode> ref;
            try {
                ref = target.identity->require();
            }
            catch (const std::logic_error& error) {
                fail(source, error.what());
            }
            if (target.parent) {
                if (ref.get() != &source) {
                    fail(source, "parent expression belongs to another node " + label(*ref));
                }
                return this;
            }
            if (ref.get() == &source) {
                fail(source, "self reference");
            }
            if (ref.get() != this && (!indices.contains(ref.get()) || ref->parent() != this)) {
                fail(source, "target " + label(*ref) + " is detached or outside this canvas");
            }
            return ref.get();
        };

        std::vector<std::vector<std::size_t>> dependents(nodes.size());
        std::vector<std::size_t> pending(nodes.size(), 0);
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            try {
                specs[i]->validate();
            }
            catch (const std::invalid_argument& error) {
                fail(*nodes[i], error.what());
            }
            const auto flex = nodes[i]->layout_flex_policy();
            if (flex.basis || flex.grow != 0.0F || flex.shrink != 0.0F) {
                fail(*nodes[i], "flex/Expanded is not supported by AnchorCanvas");
            }
            std::unordered_set<std::size_t> dependencies;
            for (const auto& target: specs[i]->targets()) {
                if (!target) {
                    continue;
                }
                const auto* resolved = resolve_target(*nodes[i], *target);
                if (resolved != this) {
                    dependencies.insert(indices.at(resolved));
                }
            }
            pending[i] = dependencies.size();
            for (const auto dependency: dependencies) {
                dependents[dependency].push_back(i);
            }
        }
        std::vector<std::size_t> order;
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (pending[i] == 0) {
                order.push_back(i);
            }
        }
        for (std::size_t i = 0; i < order.size(); ++i) {
            for (const auto dependent: dependents[order[i]]) {
                if (--pending[dependent] == 0) {
                    order.push_back(dependent);
                }
            }
        }
        if (order.size() != nodes.size()) {
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                if (pending[i] != 0) {
                    fail(*nodes[i], "sibling dependency cycle (including cross-axis cycles)");
                }
            }
        }

        const auto canvas_rect = foundation::NanRect::from_xywh(
            0.0F,
            0.0F,
            axis_x.mode == AxisMode::definite ? axis_x.extent : 0.0F,
            axis_y.mode == AxisMode::definite ? axis_y.extent : 0.0F
        );
        const float basis_x = axis_x.mode == AxisMode::definite
            ? axis_x.extent
            : std::numeric_limits<float>::infinity();
        const float basis_y = axis_y.mode == AxisMode::definite
            ? axis_y.extent
            : std::numeric_limits<float>::infinity();
        const foundation::NanSize percentage_basis(basis_x, basis_y);

        std::vector<foundation::NanRect> rects(nodes.size());
        std::vector<Placement> result;
        float max_right = 0.0F;
        float max_bottom = 0.0F;
        for (const auto i: order) {
            auto& node = *nodes[i];
            const auto& spec = *specs[i];
            const auto& sizing = node.size_spec();
            if (axis_x.mode == AxisMode::content) {
                if ((spec.right && resolve_target(node, *spec.right) == this)
                    || (spec.horizontal_center
                        && resolve_target(node, *spec.horizontal_center) == this))
                {
                    fail(
                        node,
                        "parent right or horizontal_center anchor requires a definite canvas width"
                    );
                }
                if (uses_percent(sizing.width) || uses_percent(sizing.min_width)
                    || uses_percent(sizing.max_width))
                {
                    fail(node, "percentage width/min/max requires a definite canvas width");
                }
            }
            if (axis_y.mode == AxisMode::content) {
                if ((spec.bottom && resolve_target(node, *spec.bottom) == this)
                    || (spec.vertical_center
                        && resolve_target(node, *spec.vertical_center) == this))
                {
                    fail(
                        node,
                        "parent bottom or vertical_center anchor requires a definite canvas height"
                    );
                }
                if (uses_percent(sizing.height) || uses_percent(sizing.min_height)
                    || uses_percent(sizing.max_height))
                {
                    fail(node, "percentage height/min/max requires a definite canvas height");
                }
            }
            const auto value =
                [&](const std::optional<AnchorTarget>& target) -> std::optional<float> {
                if (!target) {
                    return std::nullopt;
                }
                const auto* resolved = resolve_target(node, *target);
                return coordinate(
                           resolved == this ? canvas_rect : rects[indices.at(resolved)],
                           target->line
                       )
                    + target->offset;
            };
            const auto left = value(spec.left), right = value(spec.right);
            const auto top = value(spec.top), bottom = value(spec.bottom);
            const auto center_x = value(spec.horizontal_center),
                       center_y = value(spec.vertical_center);
            const auto width = left && right ? std::optional(*right - *left) : std::nullopt;
            const auto height = top && bottom ? std::optional(*bottom - *top) : std::nullopt;
            validate_axis(node, sizing.width, sizing.min_width, sizing.max_width, basis_x, width);
            validate_axis(
                node,
                sizing.height,
                sizing.min_height,
                sizing.max_height,
                basis_y,
                height
            );
            const float measure_max_w = width.value_or(
                axis_x.mode == AxisMode::definite ? axis_x.extent : axis_x.max_limit
            );
            const float measure_max_h = height.value_or(
                axis_y.mode == AxisMode::definite ? axis_y.extent : axis_y.max_limit
            );
            const auto measured = node.measure_layout_with_basis(
                foundation::NanLayoutConstraints {
                    .min_width = width.value_or(0.0F),
                    .max_width = measure_max_w,
                    .min_height = height.value_or(0.0F),
                    .max_height = measure_max_h
                },
                percentage_basis
            );
            const auto x = left.value_or(
                right          ? *right - measured.get_width()
                    : center_x ? *center_x - measured.get_width() * 0.5F
                               : 0.0F
            );
            const auto y = top.value_or(
                bottom         ? *bottom - measured.get_height()
                    : center_y ? *center_y - measured.get_height() * 0.5F
                               : 0.0F
            );
            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(measured.get_width())
                || !std::isfinite(measured.get_height()))
            {
                fail(node, "non-finite solved rectangle");
            }
            rects[i] =
                foundation::NanRect::from_xywh(x, y, measured.get_width(), measured.get_height());
            max_right = std::max(max_right, x + measured.get_width());
            max_bottom = std::max(max_bottom, y + measured.get_height());
            result.push_back({&node, rects[i]});
        }
        return SolveOutcome {
            .placements = std::move(result),
            .content_bounds = foundation::NanSize(max_right, max_bottom),
        };
    }

    void AnchorCanvas::on_layout() {
        if (provisional_relayout_active()) {
            mark_layout_dirty();
            return;
        }
        try {
            if (deferred_width_ || deferred_height_) {
                fail(*this, "AnchorCanvas requires a definite parent bound before layout");
            }
            const auto lx = rest_solve_axis(true);
            const auto ly = rest_solve_axis(false);
            const auto outcome = solve(lx, ly);
            for (const auto& placement: outcome.placements) {
                placement.node->layout_to(placement.rect);
            }
            committed_size_ = size();
            has_committed_layout_ = true;
        }
        catch (...) {
            // layout_to clears layout flags before entering this callback. A failed
            // solve must remain pending, e.g. while a forward reference is unbound.
            mark_layout_dirty();
            throw;
        }
    }

    void AnchorCanvas::set_child_anchors(std::span<const Update> updates) {
        (void)solve(rest_solve_axis(true), rest_solve_axis(false), updates);
        std::vector<std::unique_ptr<AnchorSpec>> prepared;
        prepared.reserve(updates.size());
        for (const auto& update: updates) {
            prepared.push_back(
                update.anchors.empty() ? nullptr : std::make_unique<AnchorSpec>(update.anchors)
            );
        }
        for (std::size_t i = 0; i < updates.size(); ++i) {
            updates[i].node->anchors_.swap(prepared[i]);
        }
        mark_layout_dirty();
    }
} // namespace nandina::scene
