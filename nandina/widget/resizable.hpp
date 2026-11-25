//
// widget/resizable — two-pane split container with draggable & keyboard-accessible divider.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_RESIZABLE_HPP
#define NANDINA_EXPERIMENT_WIDGET_RESIZABLE_HPP

#include "../foundation/geometry.hpp"
#include "../reactive/event.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "layout.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nandina::widget
{
    /**
     * Two-pane split container (`first` and `second`) separated by an interactive
     * divider handle along `LayoutAxis::horizontal` (left / right) or
     * `LayoutAxis::vertical` (top / bottom).
     *
     * Sizing & interaction contract:
     * - Ratio-driven (`ratio() ∈ [0.0F, 1.0F]`, default `0.5F`), preserving pane
     *   proportions when the outer container resizes.
     * - Clamps effective pane extents to `min_first_extent()` and `min_second_extent()`
     *   (gracefully scaling down when the container is smaller than `min_first + min_second`).
     * - Supports pointer drag on the divider handle, double-click to reset to
     *   `default_ratio()` (when `reset_on_double_click()` is enabled), keyboard
     *   adjustment (`Left`/`Right` for horizontal, `Up`/`Down` for vertical, plus
     *   `Home`/`End`), and accessibility semantics (`Role::slider` on the handle).
     * - Three or more panes are composed by nesting `Resizable` instances.
     * - Programmatic `set_ratio()` / `reset_ratio()` are silent; user interactions emit
     *   `ratio_changed()` and `set_on_ratio_changed()` with owner-release safety.
     */
    class Resizable: public scene::NanControl {
    public:
        explicit Resizable(
            LayoutAxis axis = LayoutAxis::horizontal,
            theme::NanTheme theme = theme::default_theme()
        );
        ~Resizable() override;

        [[nodiscard]] static auto create(
            LayoutAxis axis = LayoutAxis::horizontal,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Resizable>;

        auto set_axis(LayoutAxis axis) -> Resizable&;
        [[nodiscard]] auto axis() const noexcept -> LayoutAxis;

        auto set_first_child(std::shared_ptr<scene::NanControl> child) -> Resizable&;
        [[nodiscard]] auto first_child() const noexcept -> scene::NanControl*;

        auto set_second_child(std::shared_ptr<scene::NanControl> child) -> Resizable&;
        [[nodiscard]] auto second_child() const noexcept -> scene::NanControl*;

        /// Populates `first_child` then `second_child` (enabling `.children(a, b)` and
        /// `ui.resizable(a, b)`). Throws `std::logic_error` if both slots are already occupied.
        auto add(std::shared_ptr<scene::NanControl> child) -> Resizable&;
        auto clear_children() -> Resizable&;

        /// Programmatic split ratio in `[0.0F, 1.0F]` (silent).
        auto set_ratio(float ratio) -> Resizable&;
        [[nodiscard]] auto ratio() const noexcept -> float;

        /// Effective ratio after clamping to `min_first_extent()` and `min_second_extent()`
        /// against the current container size.
        [[nodiscard]] auto effective_ratio() const noexcept -> float;

        auto set_default_ratio(float ratio) -> Resizable&;
        [[nodiscard]] auto default_ratio() const noexcept -> float;
        auto reset_ratio() -> Resizable&;

        auto set_reset_on_double_click(bool enabled) -> Resizable&;
        [[nodiscard]] auto reset_on_double_click() const noexcept -> bool;

        auto set_min_first_extent(float extent) -> Resizable&;
        [[nodiscard]] auto min_first_extent() const noexcept -> float;

        auto set_min_second_extent(float extent) -> Resizable&;
        [[nodiscard]] auto min_second_extent() const noexcept -> float;

        auto set_min_panes_extent(float first_extent, float second_extent) -> Resizable&;

        auto set_handle_thickness(float thickness) -> Resizable&;
        [[nodiscard]] auto handle_thickness() const noexcept -> float;

        auto set_keyboard_step(float step) -> Resizable&;
        [[nodiscard]] auto keyboard_step() const noexcept -> float;

        auto set_disabled(bool disabled) -> Resizable&;
        [[nodiscard]] auto disabled() const noexcept -> bool;

        auto set_label(std::string label) -> Resizable&;
        [[nodiscard]] auto label() const noexcept -> std::string_view;

        auto set_on_ratio_changed(std::function<void(float)> callback) -> Resizable&;
        [[nodiscard]] auto ratio_changed() const noexcept -> const reactive::Event<float>&;

        [[nodiscard]] auto first_pane_rect() const noexcept -> foundation::NanRect;
        [[nodiscard]] auto handle_rect() const noexcept -> foundation::NanRect;
        [[nodiscard]] auto second_pane_rect() const noexcept -> foundation::NanRect;

        [[nodiscard]] auto handle() noexcept -> scene::NanControl&;
        [[nodiscard]] auto handle() const noexcept -> const scene::NanControl&;
        auto focus_handle() -> bool;

        [[nodiscard]] auto is_dragging() const noexcept -> bool;
        [[nodiscard]] auto is_handle_hovered() const noexcept -> bool;
        [[nodiscard]] auto is_handle_focused() const noexcept -> bool;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const noexcept -> const theme::NanTheme&;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        [[nodiscard]] auto
        child_axis_assignment(const scene::NanControl& child, bool horizontal) const
            -> scene::ChildAxisAssignment override;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        auto on_input_capture(scene::InputEvent& event) -> bool override;
        void on_ready() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        class Handle;

        struct SplitGeometry {
            foundation::NanRect first_rect = foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
            foundation::NanRect handle_rect =
                foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
            foundation::NanRect second_rect =
                foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
            float available_main = 0.0F;
            float min_ratio = 0.0F;
            float max_ratio = 1.0F;
            float effective_ratio = 0.5F;
        };

        [[nodiscard]] auto compute_split_geometry(foundation::NanSize container_size) const
            -> SplitGeometry;
        void layout_panes(bool from_on_layout = false);
        void apply_user_ratio(float requested_ratio);
        void validate_pane_node(
            const std::shared_ptr<scene::NanControl>& next,
            const scene::NanControl* current_slot
        ) const;
        void release_pane_input(const scene::NanControl& pane);

        LayoutAxis axis_ = LayoutAxis::horizontal;
        float ratio_ = 0.5F;
        float default_ratio_ = 0.5F;
        float min_first_extent_ = 48.0F;
        float min_second_extent_ = 48.0F;
        float handle_thickness_ = 8.0F;
        float keyboard_step_ = 0.05F;
        bool reset_on_double_click_ = true;
        bool disabled_ = false;
        std::string label_ = "Resizable divider";

        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        bool system_explicit_ = false;

        std::shared_ptr<Handle> handle_;
        std::weak_ptr<scene::NanControl> first_child_;
        std::weak_ptr<scene::NanControl> second_child_;
        SplitGeometry last_geometry_ {};

        std::function<void(float)> on_ratio_changed_;
        reactive::Event<float> ratio_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_RESIZABLE_HPP
