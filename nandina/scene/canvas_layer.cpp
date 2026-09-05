#include "canvas_layer.hpp"

#include "control.hpp"
#include "scene_tree.hpp"
#include "../render/draw_context.hpp"

#include <algorithm>
#include <stdexcept>

namespace nandina::scene
{

    CanvasLayer::CanvasLayer(const CanvasSpace space, const int order):
        space_(space), order_(order) {
        NanNode2D::set_z_index(order);
    }

    auto CanvasLayer::create(const CanvasSpace space, const int order)
        -> std::shared_ptr<CanvasLayer> {
        return std::make_shared<CanvasLayer>(space, order);
    }

    auto CanvasLayer::space() const -> CanvasSpace { return space_; }
    void CanvasLayer::set_space(const CanvasSpace space) {
        if (space == CanvasSpace::world && layout_root() != nullptr) {
            throw std::logic_error("a layer with a layout root must remain screen-space");
        }
        space_ = space;
    }
    auto CanvasLayer::order() const -> int { return order_; }
    void CanvasLayer::set_order(const int order) {
        order_ = order;
        NanNode2D::set_z_index(order);
    }
    auto CanvasLayer::input_mode() const -> LayerInputMode { return input_mode_; }
    void CanvasLayer::set_input_mode(const LayerInputMode mode) { input_mode_ = mode; }

    auto CanvasLayer::canvas_transform() const -> const foundation::NanTransform2D& {
        return transform();
    }

    void CanvasLayer::set_canvas_transform(const foundation::NanTransform2D& transform) {
        NanNode2D::set_transform(transform);
    }

    auto CanvasLayer::screen_to_canvas(const foundation::NanPoint point) const
        -> foundation::NanPoint {
        return transform().inverse_transform_point(point);
    }

    auto CanvasLayer::canvas_to_screen(const foundation::NanPoint point) const
        -> foundation::NanPoint {
        return transform().transform_point(point);
    }

    auto CanvasLayer::layout_root() const -> NanControl* {
        auto current = layout_root_.lock();
        return current != nullptr && current->parent() == this ? current.get() : nullptr;
    }

    auto CanvasLayer::set_layout_root(std::shared_ptr<NanControl> root) -> NanControl& {
        if (!root) {
            throw std::invalid_argument("CanvasLayer::set_layout_root: root is null");
        }
        if (space_ != CanvasSpace::screen) {
            throw std::logic_error("only screen-space layers may own a layout root");
        }
        auto current = layout_root_.lock();
        if (auto* tree = get_tree(); tree != nullptr && tree->defers_tree_mutation()) {
            auto self = std::static_pointer_cast<CanvasLayer>(shared_from_this());
            auto* raw = root.get();
            tree->defer_tree_mutation([self = std::move(self), root = std::move(root)]() mutable {
                self->set_layout_root(std::move(root));
            });
            return *raw;
        }
        layout_root_ = root;
        return static_cast<NanControl&>(replace_child(current.get(), std::move(root)));
    }

    auto CanvasLayer::_push_draw_transform(render::DrawContext& ctx)
        -> foundation::NanTransform2D {
        auto saved = ctx.world_;
        ctx.world_ = ctx.root_transform_.compose(transform());
        return saved;
    }

    void CanvasLayer::_pop_draw_transform(
        render::DrawContext& ctx,
        const foundation::NanTransform2D& saved
    ) {
        ctx.world_ = saved;
    }

    auto LayerStack::create() -> std::shared_ptr<LayerStack> {
        return std::make_shared<LayerStack>();
    }

    auto LayerStack::add_layer(std::shared_ptr<CanvasLayer> layer) -> CanvasLayer& {
        if (!layer) {
            throw std::invalid_argument("LayerStack::add_layer: layer is null");
        }
        return static_cast<CanvasLayer&>(add_child(std::move(layer)));
    }

    auto LayerStack::layer_count() const -> std::size_t {
        return layers_in_order().size();
    }

    auto LayerStack::layer_at(const std::size_t index) const -> CanvasLayer* {
        auto* child = get_child(index);
        return child != nullptr ? child->as_canvas_layer() : nullptr;
    }

    auto LayerStack::layers_in_order(const bool front_to_back) const
        -> std::vector<CanvasLayer*> {
        std::vector<CanvasLayer*> result;
        result.reserve(child_count());
        for (std::size_t i = 0; i < child_count(); ++i) {
            if (auto* layer = layer_at(i); layer != nullptr) {
                result.push_back(layer);
            }
        }
        std::ranges::stable_sort(result, [](const CanvasLayer* lhs, const CanvasLayer* rhs) {
            return lhs->order() < rhs->order();
        });
        if (front_to_back) {
            std::ranges::reverse(result);
        }
        return result;
    }

} // namespace nandina::scene
