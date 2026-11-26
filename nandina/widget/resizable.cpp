//
// widget/resizable — two-pane split container with draggable & keyboard-accessible divider.
//

#include "resizable.hpp"

#include "../foundation/input.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/focus_ring_painter.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto numeric_text(const float value) -> std::string {
            char buffer[32] {};
            const auto [end, error] = std::to_chars(
                std::begin(buffer),
                std::end(buffer),
                value,
                std::chars_format::general,
                4
            );
            return error == std::errc {} ? std::string(buffer, end) : std::string {};
        }

        void validate_axis(const LayoutAxis axis) {
            switch (axis) {
                case LayoutAxis::horizontal:
                case LayoutAxis::vertical:
                    return;
            }
            throw std::invalid_argument("Resizable: invalid LayoutAxis");
        }

        void validate_finite_ratio(const float ratio, const char* what) {
            if (!std::isfinite(ratio)) {
                throw std::invalid_argument(what);
            }
        }

        void validate_min_extent(const float extent, const char* what) {
            if (!std::isfinite(extent) || extent < 0.0F) {
                throw std::invalid_argument(what);
            }
        }
    } // namespace

    class Resizable::Handle final: public scene::NanControl {
    public:
        explicit Handle(Resizable& owner): owner_(&owner) {}

        void detach_owner() noexcept {
            owner_ = nullptr;
        }

        [[nodiscard]] auto hovered() const noexcept -> bool {
            return hovered_;
        }

        [[nodiscard]] auto dragging() const noexcept -> bool {
            return dragging_;
        }

        [[nodiscard]] auto focused() const noexcept -> bool {
            return focused_;
        }

        void clear_interaction_state() {
            hovered_ = false;
            dragging_ = false;
            focused_ = false;
            has_last_click_ = false;
            if (is_inside_tree()) {
                get_tree()->release_pointer_capture(this);
                if (get_tree()->focused_node() == this) {
                    get_tree()->set_focus(nullptr);
                }
            }
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        }

        [[nodiscard]] auto z_index_hint() const -> int override {
            return 1;
        }

        [[nodiscard]] auto is_focusable() const -> bool override {
            return owner_ != nullptr && !owner_->disabled_;
        }

        void on_process(const float dt) override {
            if (has_last_click_) {
                last_click_elapsed_ += std::max(0.0F, dt);
                if (last_click_elapsed_ > kDoubleClickInterval) {
                    has_last_click_ = false;
                }
            }
            scene::NanControl::on_process(dt);
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (owner_ == nullptr) {
                return false;
            }
            const auto keep_owner = owner_->weak_from_this().lock();

            if (event.type() == scene::EventType::focus_enter) {
                focused_ = !owner_->disabled_;
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                return false;
            }
            if (event.type() == scene::EventType::focus_leave) {
                focused_ = false;
                if (dragging_) {
                    dragging_ = false;
                    if (is_inside_tree()) {
                        get_tree()->release_pointer_capture(this);
                    }
                }
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                return false;
            }
            if (owner_->disabled_) {
                return false;
            }

            switch (event.type()) {
                case scene::EventType::mouse_enter:
                    hovered_ = true;
                    mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                    return false;
                case scene::EventType::mouse_leave:
                    hovered_ = false;
                    mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                    return false;
                case scene::EventType::mouse_button: {
                    auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                    if (mouse.button() != scene::MouseButtonEvent::Button::left) {
                        return false;
                    }
                    if (mouse.is_pressed()) {
                        if (owner_->reset_on_double_click_ && has_last_click_
                            && last_click_elapsed_ <= kDoubleClickInterval
                            && last_click_screen_pos_.distance(mouse.screen_pos())
                                <= kDoubleClickDistance)
                        {
                            has_last_click_ = false;
                            dragging_ = false;
                            if (is_inside_tree()) {
                                get_tree()->release_pointer_capture(this);
                            }
                            owner_->apply_user_ratio(owner_->default_ratio_);
                            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                            event.accept();
                            return true;
                        }

                        has_last_click_ = true;
                        last_click_elapsed_ = 0.0F;
                        last_click_screen_pos_ = mouse.screen_pos();

                        const auto owner_local = owner_->to_local(mouse.screen_pos());
                        const float pointer_main = owner_->axis_ == LayoutAxis::horizontal
                            ? owner_local.get_x()
                            : owner_local.get_y();
                        const float handle_start = owner_->axis_ == LayoutAxis::horizontal
                            ? owner_->last_geometry_.handle_rect.get_x()
                            : owner_->last_geometry_.handle_rect.get_y();
                        drag_grab_offset_ = pointer_main - handle_start;
                        dragging_ = true;
                        if (is_inside_tree()) {
                            get_tree()->set_pointer_capture(this);
                        }
                        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                        event.accept();
                        return true;
                    }

                    if (dragging_) {
                        dragging_ = false;
                        if (is_inside_tree()) {
                            get_tree()->release_pointer_capture(this);
                        }
                        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                        event.accept();
                        return true;
                    }
                    return false;
                }
                case scene::EventType::mouse_move: {
                    if (!dragging_) {
                        return false;
                    }
                    const auto& move = static_cast<scene::MouseMoveEvent&>(event);
                    if (has_last_click_
                        && last_click_screen_pos_.distance(move.screen_pos())
                            > kDragCancelClickDist)
                    {
                        has_last_click_ = false;
                    }
                    const auto owner_local = owner_->to_local(move.screen_pos());
                    const float pointer_main = owner_->axis_ == LayoutAxis::horizontal
                        ? owner_local.get_x()
                        : owner_local.get_y();
                    const float desired_first = pointer_main - drag_grab_offset_;
                    const float available = owner_->last_geometry_.available_main;
                    if (available > foundation::nan_epsilon) {
                        const float next_ratio = std::clamp(desired_first / available, 0.0F, 1.0F);
                        owner_->apply_user_ratio(next_ratio);
                    }
                    event.accept();
                    return true;
                }
                case scene::EventType::key: {
                    const auto& key = static_cast<scene::KeyEvent&>(event);
                    if (!key.is_press_or_repeat()) {
                        return false;
                    }
                    const float current = owner_->effective_ratio();
                    const bool horizontal = owner_->axis_ == LayoutAxis::horizontal;
                    if ((horizontal && key.logical_key() == foundation::LogicalKey::left)
                        || (!horizontal && key.logical_key() == foundation::LogicalKey::up))
                    {
                        owner_->apply_user_ratio(current - owner_->keyboard_step_);
                        event.accept();
                        return true;
                    }
                    if ((horizontal && key.logical_key() == foundation::LogicalKey::right)
                        || (!horizontal && key.logical_key() == foundation::LogicalKey::down))
                    {
                        owner_->apply_user_ratio(current + owner_->keyboard_step_);
                        event.accept();
                        return true;
                    }
                    if (key.logical_key() == foundation::LogicalKey::home) {
                        owner_->apply_user_ratio(owner_->last_geometry_.min_ratio);
                        event.accept();
                        return true;
                    }
                    if (key.logical_key() == foundation::LogicalKey::end) {
                        owner_->apply_user_ratio(owner_->last_geometry_.max_ratio);
                        event.accept();
                        return true;
                    }
                    return false;
                }
                case scene::EventType::mouse_wheel:
                case scene::EventType::text_input:
                case scene::EventType::focus_enter:
                case scene::EventType::focus_leave:
                    return false;
            }
            return false;
        }

        void on_draw(render::DrawContext& context) override {
            if (owner_ == nullptr) {
                return;
            }
            const auto world =
                render::world_bounds_from_local(context.world_transform(), local_rect());
            if (world.get_width() <= 0.0F || world.get_height() <= 0.0F) {
                return;
            }

            const auto& palette = owner_->system_->palette(owner_->appearance_);
            const float opacity = context.opacity() * (owner_->disabled_ ? 0.5F : 1.0F);
            const bool horizontal = owner_->axis_ == LayoutAxis::horizontal;

            // 1. Track highlight when hovered or dragging.
            if (!owner_->disabled_ && (dragging_ || hovered_)) {
                const auto track_color = (dragging_ ? palette.accent : palette.surface_variant)
                                             .with_alpha((dragging_ ? 0.85F : 0.60F) * opacity);
                context.device().draw_rect(world, track_color);
            }

            // 2. Center 1 DIP separator rule.
            const float line_thickness = std::max(1.0F, context.logical_to_screen(1.0F));
            const auto line_color =
                (dragging_ ? palette.primary
                           : ((hovered_ || focused_) ? palette.ring : palette.border))
                    .with_alpha(opacity);
            const auto line_rect = horizontal
                ? foundation::NanRect::from_xywh(
                      world.get_x() + (world.get_width() - line_thickness) * 0.5F,
                      world.get_y(),
                      line_thickness,
                      world.get_height()
                  )
                : foundation::NanRect::from_xywh(
                      world.get_x(),
                      world.get_y() + (world.get_height() - line_thickness) * 0.5F,
                      world.get_width(),
                      line_thickness
                  );
            context.device().draw_rect(line_rect, line_color);

            // 3. Center grip pill indicator.
            const float pill_short = std::min(
                context.logical_to_screen(4.0F),
                std::min(world.get_width(), world.get_height())
            );
            const float pill_long = std::min(
                context.logical_to_screen(24.0F),
                (horizontal ? world.get_height() : world.get_width()) * 0.6F
            );
            if (pill_short > 0.0F && pill_long > 0.0F) {
                const float pill_w = horizontal ? pill_short : pill_long;
                const float pill_h = horizontal ? pill_long : pill_short;
                const auto pill_rect = foundation::NanRect::from_xywh(
                    world.get_x() + (world.get_width() - pill_w) * 0.5F,
                    world.get_y() + (world.get_height() - pill_h) * 0.5F,
                    pill_w,
                    pill_h
                );
                const auto pill_color =
                    (dragging_ || focused_ ? palette.primary : palette.on_surface_variant)
                        .with_alpha(opacity);
                context.device()
                    .draw_rounded_rect(pill_rect, std::min(pill_w, pill_h) * 0.5F, pill_color);
            }

            // 4. Focus ring when keyboard focus hint is visible.
            if (focused_ && !owner_->disabled_ && primitives::focus_hint_visible(*this)) {
                const theme::ResolvedFocusRing focus_style {
                    .color = palette.ring,
                    .width = 2.0F,
                };
                primitives::FocusRingPainter::paint(
                    context,
                    world,
                    focus_style,
                    context.opacity(),
                    2.0F
                );
            }
        }

    protected:
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
            if (owner_ == nullptr) {
                return {};
            }
            auto actions = semantics::Action::none;
            if (!owner_->disabled_) {
                actions = semantics::Action::focus | semantics::Action::set_value
                    | semantics::Action::increment | semantics::Action::decrement;
            }
            return {
                .role = semantics::Role::slider,
                .label = owner_->label_,
                .value = numeric_text(owner_->effective_ratio()),
                .hint = numeric_text(owner_->last_geometry_.min_ratio) + " to "
                    + numeric_text(owner_->last_geometry_.max_ratio),
                .state =
                    {
                        .focusable = !owner_->disabled_,
                        .focused = focused_,
                        .disabled = owner_->disabled_,
                    },
                .actions = actions,
            };
        }

        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override {
            if (owner_ == nullptr || owner_->disabled_) {
                return false;
            }
            const auto keep_owner = owner_->weak_from_this().lock();
            const float current = owner_->effective_ratio();
            if (request.action == semantics::Action::increment) {
                owner_->apply_user_ratio(current + owner_->keyboard_step_);
                return true;
            }
            if (request.action == semantics::Action::decrement) {
                owner_->apply_user_ratio(current - owner_->keyboard_step_);
                return true;
            }
            if (request.action != semantics::Action::set_value) {
                return false;
            }
            float requested = 0.0F;
            const char* begin = request.value.data();
            const char* end = begin + request.value.size();
            const auto [parsed, error] = std::from_chars(begin, end, requested);
            if (error != std::errc {} || parsed != end || !std::isfinite(requested)) {
                return false;
            }
            owner_->apply_user_ratio(requested);
            return true;
        }

    private:
        static constexpr float kDoubleClickInterval = 0.30F;
        static constexpr float kDoubleClickDistance = 5.0F;
        static constexpr float kDragCancelClickDist = 3.0F;

        Resizable* owner_ = nullptr;
        bool hovered_ = false;
        bool dragging_ = false;
        bool focused_ = false;
        bool has_last_click_ = false;
        float last_click_elapsed_ = 0.0F;
        foundation::NanPoint last_click_screen_pos_ {};
        float drag_grab_offset_ = 0.0F;
    };

    Resizable::Resizable(const LayoutAxis axis, theme::NanTheme theme):
        axis_(axis),
        system_(theme::design_system_shared_from_theme(theme)),
        theme_view_(std::move(theme)),
        handle_(std::make_shared<Handle>(*this)) {
        validate_axis(axis);
        set_overflow(scene::ControlOverflow::clip);
        add_child(handle_);
    }

    Resizable::~Resizable() {
        if (handle_) {
            handle_->detach_owner();
        }
    }

    auto Resizable::create(const LayoutAxis axis, theme::NanTheme theme)
        -> std::shared_ptr<Resizable> {
        return std::make_shared<Resizable>(axis, std::move(theme));
    }

    auto Resizable::set_axis(const LayoutAxis axis) -> Resizable& {
        validate_axis(axis);
        if (axis_ == axis) {
            return *this;
        }
        axis_ = axis;
        if (width() > 0.0F || height() > 0.0F) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            layout_panes();
        }
        mark_layout_dirty();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        return *this;
    }

    auto Resizable::axis() const noexcept -> LayoutAxis {
        return axis_;
    }

    void Resizable::validate_pane_node(
        const std::shared_ptr<scene::NanControl>& next,
        const scene::NanControl* current_slot
    ) const {
        if (!next) {
            throw std::invalid_argument("Resizable: pane child must not be null");
        }
        if (next.get() == this || next.get() == handle_.get() || next->is_ancestor_of(*this)) {
            throw std::invalid_argument(
                "Resizable: pane child cannot be self, handle, or ancestor"
            );
        }
        const auto* first = first_child_.lock().get();
        const auto* second = second_child_.lock().get();
        if ((next.get() == first && first != current_slot)
            || (next.get() == second && second != current_slot))
        {
            throw std::invalid_argument(
                "Resizable: pane child is already mounted in the other Resizable slot"
            );
        }
        if (next->parent() != nullptr && next.get() != current_slot) {
            throw std::invalid_argument("Resizable: pane child already has a parent");
        }
        if (next->is_inside_tree() && next.get() != current_slot) {
            throw std::invalid_argument("Resizable: pane child is already inside a scene tree");
        }
        next->validate_anchor_parent(*this);
    }

    auto Resizable::set_first_child(std::shared_ptr<scene::NanControl> child) -> Resizable& {
        auto current = first_child_.lock();
        validate_pane_node(child, current.get());
        if (current == child) {
            return *this;
        }
        if (current) {
            release_pane_input(*current);
            replace_child(current.get(), child);
        }
        else {
            insert_child(0, child);
        }
        first_child_ = std::move(child);
        if (width() > 0.0F || height() > 0.0F) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            layout_panes();
        }
        mark_layout_dirty();
        mark_semantics_dirty();
        return *this;
    }

    auto Resizable::first_child() const noexcept -> scene::NanControl* {
        return first_child_.lock().get();
    }

    auto Resizable::set_second_child(std::shared_ptr<scene::NanControl> child) -> Resizable& {
        auto current = second_child_.lock();
        validate_pane_node(child, current.get());
        if (current == child) {
            return *this;
        }
        if (current) {
            release_pane_input(*current);
            replace_child(current.get(), child);
        }
        else {
            add_child(child);
        }
        second_child_ = std::move(child);
        if (width() > 0.0F || height() > 0.0F) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            layout_panes();
        }
        mark_layout_dirty();
        mark_semantics_dirty();
        return *this;
    }

    auto Resizable::second_child() const noexcept -> scene::NanControl* {
        return second_child_.lock().get();
    }

    auto Resizable::add(std::shared_ptr<scene::NanControl> child) -> Resizable& {
        if (!first_child_.lock()) {
            return set_first_child(std::move(child));
        }
        if (!second_child_.lock()) {
            return set_second_child(std::move(child));
        }
        throw std::logic_error(
            "Resizable::add: Resizable supports at most two child panes (nest Resizable for 3+ panes)"
        );
    }

    auto Resizable::clear_children() -> Resizable& {
        if (const auto first = first_child_.lock()) {
            release_pane_input(*first);
            remove_and_delete(*first);
            first_child_.reset();
        }
        if (const auto second = second_child_.lock()) {
            release_pane_input(*second);
            remove_and_delete(*second);
            second_child_.reset();
        }
        if (width() > 0.0F || height() > 0.0F) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            layout_panes();
        }
        else {
            last_geometry_ = {};
        }
        mark_layout_dirty();
        mark_semantics_dirty();
        return *this;
    }

    auto Resizable::set_ratio(const float ratio) -> Resizable& {
        validate_finite_ratio(ratio, "Resizable: ratio must be finite");
        const float clamped = std::clamp(ratio, 0.0F, 1.0F);
        if (std::abs(ratio_ - clamped) <= foundation::nan_epsilon) {
            return *this;
        }
        ratio_ = clamped;
        // 在窗口输入阶段只更新 O(1) 几何缓存并标脏，由本帧末尾的 layout_root() 统一排版一次；
        // 脱离窗口或空闲态（如单测直接调用）保持立即落位，不破坏既有即时几何断言。
        if (is_inside_tree() && get_tree()->phase() == scene::FramePhase::input) {
            if (width() > 0.0F || height() > 0.0F) {
                last_geometry_ = compute_split_geometry(size());
            }
            else {
                last_geometry_ = {};
            }
        }
        else {
            layout_panes();
        }
        mark_layout_dirty();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        handle_->mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        return *this;
    }

    auto Resizable::ratio() const noexcept -> float {
        return ratio_;
    }

    auto Resizable::effective_ratio() const noexcept -> float {
        if (width() > 0.0F || height() > 0.0F) {
            return compute_split_geometry(size()).effective_ratio;
        }
        return ratio_;
    }

    auto Resizable::set_default_ratio(const float ratio) -> Resizable& {
        validate_finite_ratio(ratio, "Resizable: default_ratio must be finite");
        default_ratio_ = std::clamp(ratio, 0.0F, 1.0F);
        return *this;
    }

    auto Resizable::default_ratio() const noexcept -> float {
        return default_ratio_;
    }

    auto Resizable::reset_ratio() -> Resizable& {
        return set_ratio(default_ratio_);
    }

    auto Resizable::set_reset_on_double_click(const bool enabled) -> Resizable& {
        reset_on_double_click_ = enabled;
        return *this;
    }

    auto Resizable::reset_on_double_click() const noexcept -> bool {
        return reset_on_double_click_;
    }

    auto Resizable::set_min_first_extent(const float extent) -> Resizable& {
        validate_min_extent(extent, "Resizable: min_first_extent must be finite and non-negative");
        if (std::abs(min_first_extent_ - extent) <= foundation::nan_epsilon) {
            return *this;
        }
        min_first_extent_ = extent;
        layout_panes();
        mark_layout_dirty();
        handle_->mark_semantics_dirty();
        return *this;
    }

    auto Resizable::min_first_extent() const noexcept -> float {
        return min_first_extent_;
    }

    auto Resizable::set_min_second_extent(const float extent) -> Resizable& {
        validate_min_extent(extent, "Resizable: min_second_extent must be finite and non-negative");
        if (std::abs(min_second_extent_ - extent) <= foundation::nan_epsilon) {
            return *this;
        }
        min_second_extent_ = extent;
        layout_panes();
        mark_layout_dirty();
        handle_->mark_semantics_dirty();
        return *this;
    }

    auto Resizable::min_second_extent() const noexcept -> float {
        return min_second_extent_;
    }

    auto Resizable::set_min_panes_extent(const float first_extent, const float second_extent)
        -> Resizable& {
        validate_min_extent(
            first_extent,
            "Resizable: min_first_extent must be finite and non-negative"
        );
        validate_min_extent(
            second_extent,
            "Resizable: min_second_extent must be finite and non-negative"
        );
        min_first_extent_ = first_extent;
        min_second_extent_ = second_extent;
        layout_panes();
        mark_layout_dirty();
        handle_->mark_semantics_dirty();
        return *this;
    }

    auto Resizable::set_handle_thickness(const float thickness) -> Resizable& {
        if (!std::isfinite(thickness) || thickness < 1.0F) {
            throw std::invalid_argument("Resizable: handle_thickness must be finite and >= 1.0");
        }
        if (std::abs(handle_thickness_ - thickness) <= foundation::nan_epsilon) {
            return *this;
        }
        handle_thickness_ = thickness;
        layout_panes();
        mark_layout_dirty();
        mark_dirty(scene::DirtyFlags::paint);
        return *this;
    }

    auto Resizable::handle_thickness() const noexcept -> float {
        return handle_thickness_;
    }

    auto Resizable::set_keyboard_step(const float step) -> Resizable& {
        if (!std::isfinite(step) || step <= 0.0F || step > 1.0F) {
            throw std::invalid_argument(
                "Resizable: keyboard_step must be finite and in (0.0, 1.0]"
            );
        }
        keyboard_step_ = step;
        return *this;
    }

    auto Resizable::keyboard_step() const noexcept -> float {
        return keyboard_step_;
    }

    auto Resizable::set_disabled(const bool disabled) -> Resizable& {
        if (disabled_ == disabled) {
            return *this;
        }
        disabled_ = disabled;
        if (disabled_) {
            handle_->clear_interaction_state();
        }
        else {
            handle_->mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        }
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        return *this;
    }

    auto Resizable::disabled() const noexcept -> bool {
        return disabled_;
    }

    auto Resizable::set_label(std::string label) -> Resizable& {
        label_ = std::move(label);
        mark_semantics_dirty();
        handle_->mark_semantics_dirty();
        return *this;
    }

    auto Resizable::label() const noexcept -> std::string_view {
        return label_;
    }

    auto Resizable::set_on_ratio_changed(std::function<void(float)> callback) -> Resizable& {
        on_ratio_changed_ = std::move(callback);
        return *this;
    }

    auto Resizable::ratio_changed() const noexcept -> const reactive::Event<float>& {
        return ratio_changed_;
    }

    auto Resizable::first_pane_rect() const noexcept -> foundation::NanRect {
        return last_geometry_.first_rect;
    }

    auto Resizable::handle_rect() const noexcept -> foundation::NanRect {
        return last_geometry_.handle_rect;
    }

    auto Resizable::second_pane_rect() const noexcept -> foundation::NanRect {
        return last_geometry_.second_rect;
    }

    auto Resizable::handle() noexcept -> scene::NanControl& {
        return *handle_;
    }

    auto Resizable::handle() const noexcept -> const scene::NanControl& {
        return *handle_;
    }

    auto Resizable::focus_handle() -> bool {
        if (disabled_ || !is_inside_tree() || !is_visible_in_tree()) {
            return false;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            return false;
        }
        tree->set_focus(handle_.get());
        return tree->focused_node() == handle_.get();
    }

    auto Resizable::is_dragging() const noexcept -> bool {
        return handle_->dragging();
    }

    auto Resizable::is_handle_hovered() const noexcept -> bool {
        return handle_->hovered();
    }

    auto Resizable::is_handle_focused() const noexcept -> bool {
        return handle_->focused();
    }

    void Resizable::set_theme(theme::NanTheme theme) {
        system_ = theme::design_system_shared_from_theme(theme);
        system_explicit_ = true;
        theme_view_ = std::move(theme);
        mark_dirty(scene::DirtyFlags::paint);
        handle_->mark_dirty(scene::DirtyFlags::paint);
    }

    auto Resizable::theme_ref() const noexcept -> const theme::NanTheme& {
        return theme_view_;
    }

    void Resizable::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_dirty(scene::DirtyFlags::paint);
        handle_->mark_dirty(scene::DirtyFlags::paint);
    }

    auto
    Resizable::child_axis_assignment(const scene::NanControl& child, const bool horizontal) const
        -> scene::ChildAxisAssignment {
        if (&child != first_child_.lock().get() && &child != second_child_.lock().get()) {
            return scene::ChildAxisAssignment::none;
        }
        const auto constraints = last_layout_constraints();
        const float hi = horizontal ? constraints.max_width : constraints.max_height;
        return (std::isfinite(hi) && hi >= 0.0F) ? scene::ChildAxisAssignment::assigned
                                                 : scene::ChildAxisAssignment::pass_through;
    }

    void Resizable::relayout() {
        scene::NanControl::ProvisionalRelayoutScope provisional;
        (void)measure_layout(foundation::NanLayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Resizable::compute_split_geometry(const foundation::NanSize container_size) const
        -> SplitGeometry {
        const bool horizontal = axis_ == LayoutAxis::horizontal;
        const float total_main =
            std::max(0.0F, horizontal ? container_size.get_width() : container_size.get_height());
        const float cross =
            std::max(0.0F, horizontal ? container_size.get_height() : container_size.get_width());
        const float handle_main = std::min(handle_thickness_, total_main);
        const float available = std::max(0.0F, total_main - handle_main);

        float min_ratio = 0.0F;
        float max_ratio = 1.0F;
        float first_main = 0.0F;

        if (available > foundation::nan_epsilon) {
            const float eff_min_first = std::min(min_first_extent_, available);
            const float eff_max_first = std::max(0.0F, available - min_second_extent_);
            if (eff_min_first <= eff_max_first) {
                min_ratio = std::clamp(eff_min_first / available, 0.0F, 1.0F);
                max_ratio = std::clamp(eff_max_first / available, 0.0F, 1.0F);
                first_main = std::clamp(available * ratio_, eff_min_first, eff_max_first);
            }
            else {
                const float min_sum = min_first_extent_ + min_second_extent_;
                const float fallback = min_sum > foundation::nan_epsilon
                    ? std::clamp(min_first_extent_ / min_sum, 0.0F, 1.0F)
                    : ratio_;
                min_ratio = fallback;
                max_ratio = fallback;
                first_main = std::clamp(available * fallback, 0.0F, available);
            }
        }

        const float second_main = std::max(0.0F, available - first_main);
        const float eff_ratio = available > foundation::nan_epsilon
            ? std::clamp(first_main / available, 0.0F, 1.0F)
            : ratio_;

        const auto make_rect = [horizontal, cross](const float main_pos, const float main_size) {
            return horizontal ? foundation::NanRect::from_xywh(main_pos, 0.0F, main_size, cross)
                              : foundation::NanRect::from_xywh(0.0F, main_pos, cross, main_size);
        };

        return SplitGeometry {
            .first_rect = make_rect(0.0F, first_main),
            .handle_rect = make_rect(first_main, handle_main),
            .second_rect = make_rect(first_main + handle_main, second_main),
            .available_main = available,
            .min_ratio = min_ratio,
            .max_ratio = max_ratio,
            .effective_ratio = eff_ratio,
        };
    }

    auto Resizable::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const bool horizontal = axis_ == LayoutAxis::horizontal;
        const auto first = first_child_.lock();
        const auto second = second_child_.lock();
        const bool finite_w = std::isfinite(constraints.max_width);
        const bool finite_h = std::isfinite(constraints.max_height);
        const bool tight_w = finite_w
            && std::abs(constraints.min_width - constraints.max_width) <= foundation::nan_epsilon;
        const bool tight_h = finite_h
            && std::abs(constraints.min_height - constraints.max_height) <= foundation::nan_epsilon;
        // 双轴紧约束下自身尺寸已完全确定，跳过测量期递归，避免嵌套 Resizable 在 measure+layout 间指数级重测。
        if (tight_w && tight_h) {
            return constraints.constrain(
                foundation::NanSize(
                    std::max(0.0F, constraints.max_width),
                    std::max(0.0F, constraints.max_height)
                )
            );
        }
        const bool finite_main = horizontal ? finite_w : finite_h;

        const auto measure_pane = [](const std::shared_ptr<scene::NanControl>& pane,
                                     const foundation::NanLayoutConstraints pane_constraints) {
            return pane && pane->visible() ? pane->measure_layout(pane_constraints)
                                           : foundation::NanSize::zero();
        };
        const auto pane_constraints_for =
            [&](const float main_extent,
                const bool tight_main) -> foundation::NanLayoutConstraints {
            if (horizontal) {
                return {
                    .min_width = tight_main ? main_extent : 0.0F,
                    .max_width = tight_main ? main_extent : constraints.max_width,
                    .min_height = constraints.min_height,
                    .max_height = constraints.max_height,
                };
            }
            return {
                .min_width = constraints.min_width,
                .max_width = constraints.max_width,
                .min_height = tight_main ? main_extent : 0.0F,
                .max_height = tight_main ? main_extent : constraints.max_height,
            };
        };
        const auto main_extent_of = [horizontal](const foundation::NanSize size) {
            return horizontal ? size.get_width() : size.get_height();
        };
        const auto cross_extent_of = [horizontal](const foundation::NanSize size) {
            return horizontal ? size.get_height() : size.get_width();
        };

        if (finite_main) {
            const float main_extent =
                std::max(0.0F, horizontal ? constraints.max_width : constraints.max_height);
            const foundation::NanSize geometry_size = horizontal
                ? foundation::NanSize(main_extent, finite_h ? constraints.max_height : 0.0F)
                : foundation::NanSize(finite_w ? constraints.max_width : 0.0F, main_extent);
            const auto geometry = compute_split_geometry(geometry_size);
            const float first_main =
                horizontal ? geometry.first_rect.get_width() : geometry.first_rect.get_height();
            const float second_main =
                horizontal ? geometry.second_rect.get_width() : geometry.second_rect.get_height();
            const auto first_size = measure_pane(first, pane_constraints_for(first_main, true));
            const auto second_size = measure_pane(second, pane_constraints_for(second_main, true));
            const float natural_cross =
                std::max(cross_extent_of(first_size), cross_extent_of(second_size));
            const float resolved_cross = horizontal
                ? (finite_h ? constraints.max_height : natural_cross)
                : (finite_w ? constraints.max_width : natural_cross);
            const auto measured = horizontal ? foundation::NanSize(main_extent, resolved_cross)
                                             : foundation::NanSize(resolved_cross, main_extent);
            return constraints.constrain(measured);
        }

        // Main axis is unbounded: measure panes naturally while honoring the finite cross axis.
        const auto first_size = measure_pane(first, pane_constraints_for(0.0F, false));
        const auto second_size = measure_pane(second, pane_constraints_for(0.0F, false));
        const float natural_main = std::max(min_first_extent_, main_extent_of(first_size))
            + handle_thickness_ + std::max(min_second_extent_, main_extent_of(second_size));
        const float natural_cross =
            std::max(cross_extent_of(first_size), cross_extent_of(second_size));
        const float resolved_cross = horizontal
            ? (finite_h ? constraints.max_height : natural_cross)
            : (finite_w ? constraints.max_width : natural_cross);
        const auto measured = horizontal ? foundation::NanSize(natural_main, resolved_cross)
                                         : foundation::NanSize(resolved_cross, natural_main);
        return constraints.constrain(measured);
    }

    void Resizable::layout_panes(const bool from_on_layout) {
        if (!from_on_layout && width() <= 0.0F && height() <= 0.0F) {
            last_geometry_ = {};
            return;
        }
        last_geometry_ = compute_split_geometry(size());
        if (const auto first = first_child_.lock(); first && first->visible()) {
            const auto measured = first->measure_layout(
                foundation::NanLayoutConstraints::tight(last_geometry_.first_rect.get_size())
            );
            first->layout_to(
                foundation::NanRect::from_origin_size(
                    last_geometry_.first_rect.get_top_left(),
                    measured
                )
            );
        }
        (void)handle_->measure_layout(
            foundation::NanLayoutConstraints::tight(last_geometry_.handle_rect.get_size())
        );
        handle_->layout_to(last_geometry_.handle_rect);
        if (const auto second = second_child_.lock(); second && second->visible()) {
            const auto measured = second->measure_layout(
                foundation::NanLayoutConstraints::tight(last_geometry_.second_rect.get_size())
            );
            second->layout_to(
                foundation::NanRect::from_origin_size(
                    last_geometry_.second_rect.get_top_left(),
                    measured
                )
            );
        }
    }

    auto Resizable::on_layout() -> void {
        layout_panes(true);
    }

    auto Resizable::on_input_capture(scene::InputEvent& event) -> bool {
        if (event.type() != scene::EventType::key) {
            return false;
        }
        auto& key = static_cast<scene::KeyEvent&>(event);
        if (!key.is_pressed() || key.logical_key() != foundation::LogicalKey::tab) {
            return false;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            return false;
        }
        auto* before = tree->focused_node();
        if (key.modifiers().shift) {
            tree->focus_previous();
        }
        else {
            tree->focus_next();
        }
        if (tree->focused_node() == before) {
            return false;
        }
        event.accept();
        return true;
    }

    void Resizable::on_ready() {
        scene::NanControl::on_ready();
    }

    auto Resizable::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::generic,
            .label = label_,
        };
    }

    void Resizable::apply_user_ratio(const float requested_ratio) {
        const auto keep_alive = weak_from_this().lock();
        const float before_effective = effective_ratio();
        const float clamped_target = (width() > 0.0F || height() > 0.0F)
            ? std::clamp(requested_ratio, last_geometry_.min_ratio, last_geometry_.max_ratio)
            : std::clamp(requested_ratio, 0.0F, 1.0F);
        set_ratio(clamped_target);
        const float after_effective = effective_ratio();
        if (std::abs(after_effective - before_effective) <= foundation::nan_epsilon) {
            return;
        }
        const auto callback = on_ratio_changed_;
        if (callback) {
            callback(after_effective);
        }
        ratio_changed_.emit(after_effective);
    }

    void Resizable::release_pane_input(const scene::NanControl& pane) {
        auto* tree = get_tree();
        if (tree == nullptr) {
            return;
        }
        if (auto* captured = tree->pointer_capture();
            captured != nullptr && (captured == &pane || pane.is_ancestor_of(*captured)))
        {
            tree->release_pointer_capture(captured);
        }
        if (auto* focused = tree->focused_node();
            focused != nullptr && (focused == &pane || pane.is_ancestor_of(*focused)))
        {
            tree->set_focus(nullptr);
        }
    }
} // namespace nandina::widget
