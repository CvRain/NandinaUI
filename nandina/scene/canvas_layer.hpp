#ifndef NANDINA_EXPERIMENT_CANVAS_LAYER_HPP
#define NANDINA_EXPERIMENT_CANVAS_LAYER_HPP

#include "node2d.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace nandina::scene
{

    enum class CanvasSpace {
        world,
        screen,
    };

    enum class LayerInputMode {
        pass,
        block_below,
        disabled,
    };

    class CanvasLayer final: public NanNode2D {
    public:
        explicit CanvasLayer(CanvasSpace space = CanvasSpace::screen, int order = 0);

        [[nodiscard]] static auto create(CanvasSpace space = CanvasSpace::screen, int order = 0)
            -> std::shared_ptr<CanvasLayer>;

        [[nodiscard]] auto space() const -> CanvasSpace;
        void set_space(CanvasSpace space);
        [[nodiscard]] auto order() const -> int;
        void set_order(int order);
        [[nodiscard]] auto z_index_hint() const -> int override { return order_; }
        [[nodiscard]] auto input_mode() const -> LayerInputMode;
        void set_input_mode(LayerInputMode mode);

        [[nodiscard]] auto canvas_transform() const -> const foundation::NanTransform2D&;
        void set_canvas_transform(const foundation::NanTransform2D& transform);
        [[nodiscard]] auto screen_to_canvas(foundation::NanPoint point) const
            -> foundation::NanPoint;
        [[nodiscard]] auto canvas_to_screen(foundation::NanPoint point) const
            -> foundation::NanPoint;

        [[nodiscard]] auto layout_root() const -> NanControl*;
        auto set_layout_root(std::shared_ptr<NanControl> root) -> NanControl&;

        [[nodiscard]] auto as_canvas_layer() -> CanvasLayer* override { return this; }
        [[nodiscard]] auto as_canvas_layer() const -> const CanvasLayer* override { return this; }

    protected:
        [[nodiscard]] auto _push_draw_transform(render::DrawContext& ctx)
            -> foundation::NanTransform2D override;
        void _pop_draw_transform(
            render::DrawContext& ctx,
            const foundation::NanTransform2D& saved
        ) override;

    private:
        CanvasSpace space_;
        int order_;
        LayerInputMode input_mode_ = LayerInputMode::pass;
        std::weak_ptr<NanControl> layout_root_;
    };

    class LayerStack: public NanNode2D {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<LayerStack>;

        auto add_layer(std::shared_ptr<CanvasLayer> layer) -> CanvasLayer&;
        [[nodiscard]] auto layer_count() const -> std::size_t;
        [[nodiscard]] auto layer_at(std::size_t index) const -> CanvasLayer*;
        [[nodiscard]] auto layers_in_order(bool front_to_back = false) const
            -> std::vector<CanvasLayer*>;

        [[nodiscard]] auto as_layer_stack() -> LayerStack* override { return this; }
        [[nodiscard]] auto as_layer_stack() const -> const LayerStack* override { return this; }
        [[nodiscard]] auto accepts_child(const NanNode& child) const -> bool override {
            return child.as_canvas_layer() != nullptr;
        }
    };

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_CANVAS_LAYER_HPP
