#include "anchor_canvas.hpp"

#include <algorithm>
#include <cmath>
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

        void require_size(foundation::NanSize size) {
            if (!std::isfinite(size.get_width()) || !std::isfinite(size.get_height())
                || size.get_width() < 0.0F || size.get_height() < 0.0F)
            {
                throw std::logic_error(
                    "AnchorCanvas requires finite, non-negative width and height; set explicit dimensions on unbounded axes"
                );
            }
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

    auto AnchorCanvas::on_measure(foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const foundation::NanSize available(constraints.max_width, constraints.max_height);
        require_size(available);
        return available;
    }

    auto AnchorCanvas::solve(foundation::NanSize available, std::span<const Update> updates)
        -> std::vector<Placement> {
        require_size(available);
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
            available.get_width(),
            available.get_height()
        );
        std::vector<foundation::NanRect> rects(nodes.size());
        std::vector<Placement> result;
        for (const auto i: order) {
            auto& node = *nodes[i];
            const auto& spec = *specs[i];
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
            const auto& sizing = node.size_spec();
            validate_axis(
                node,
                sizing.width,
                sizing.min_width,
                sizing.max_width,
                available.get_width(),
                width
            );
            validate_axis(
                node,
                sizing.height,
                sizing.min_height,
                sizing.max_height,
                available.get_height(),
                height
            );
            const auto measured = node.measure_layout_with_basis(
                foundation::NanLayoutConstraints {
                    .min_width = width.value_or(0.0F),
                    .max_width = width.value_or(available.get_width()),
                    .min_height = height.value_or(0.0F),
                    .max_height = height.value_or(available.get_height())
                },
                available
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
            result.push_back({&node, rects[i]});
        }
        return result;
    }

    void AnchorCanvas::on_layout() {
        try {
            const auto placements = solve(size());
            for (const auto& placement: placements) {
                placement.node->layout_to(placement.rect);
            }
        }
        catch (...) {
            // layout_to clears layout flags before entering this callback. A failed
            // solve must remain pending, e.g. while a forward reference is unbound.
            mark_layout_dirty();
            throw;
        }
    }

    void AnchorCanvas::set_child_anchors(std::span<const Update> updates) {
        (void)solve(size(), updates);
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
