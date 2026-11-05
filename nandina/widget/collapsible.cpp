// widget/collapsible — retained disclosure layout, focus restoration and activation.
#include "collapsible.hpp"

#include "../scene/scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    struct Collapsible::State {
        bool open = false;
    };

    class Collapsible::Trigger final: public Button {
    public:
        Trigger(std::string text, theme::NanTheme theme, const std::shared_ptr<State>& state):
            Button(std::move(text), std::move(theme)),
            state_(state) {}

    protected:
        auto semantics_properties() const -> semantics::Properties override {
            auto result = Button::semantics_properties();
            if (const auto state = state_.lock()) {
                result.state.expanded = state->open;
            }
            return result;
        }

    private:
        std::weak_ptr<State> state_;
    };

    namespace
    {
        auto finite_extent(const double value) -> float {
            if (!std::isfinite(value) || value < 0.0
                || value > static_cast<double>(std::numeric_limits<float>::max()))
            {
                throw std::invalid_argument("Collapsible layout extent must be finite");
            }
            return static_cast<float>(value);
        }
        void require_text(const std::string_view text) {
            if (text.empty()) {
                throw std::invalid_argument("Collapsible trigger text must not be empty");
            }
        }
    } // namespace

    Collapsible::Collapsible(std::string text, theme::NanTheme theme):
        state_(std::make_shared<State>()),
        body_(std::make_shared<scene::NanControl>()) {
        require_text(text);
        trigger_ = std::make_shared<Trigger>(std::move(text), std::move(theme), state_);
        trigger_->set_treatment(theme::ButtonTreatment::ghost);
        trigger_->set_label_align(theme::TextAlign::start);
        trigger_->set_on_click([this, lifetime = std::weak_ptr<State>(state_)] {
            if (!lifetime.expired()) {
                toggle();
            }
        });
        body_->set_visible(false);
        body_->set_overflow(scene::ControlOverflow::clip);
        set_overflow(scene::ControlOverflow::clip);
        add_child(trigger_);
        add_child(body_);
    }

    Collapsible::~Collapsible() {
        trigger_->set_on_click({});
    }

    auto Collapsible::create(std::string text, theme::NanTheme theme)
        -> std::shared_ptr<Collapsible> {
        return std::make_shared<Collapsible>(std::move(text), std::move(theme));
    }

    auto Collapsible::set_child(std::shared_ptr<scene::NanControl> child) -> Collapsible& {
        if (!child) {
            throw std::invalid_argument("Collapsible content must not be null");
        }
        auto current = child_.lock();
        if (current == child) {
            return *this;
        }
        if (child.get() == this || child == body_ || child == trigger_
            || child->is_ancestor_of(*this) || child->parent() || child->is_inside_tree())
        {
            throw std::logic_error("Collapsible content must be a detached control");
        }
        child->validate_anchor_parent(*body_);
        const auto keep_alive = weak_from_this().lock();
        const auto generation = ++content_generation_;
        release_content_input();
        if (generation != content_generation_) {
            return *this;
        }
        body_->replace_child(current.get(), child);
        child_ = std::move(child);
        mark_layout_dirty();
        mark_semantics_dirty();
        return *this;
    }

    void Collapsible::clear_child() {
        if (auto current = child_.lock()) {
            const auto keep_alive = weak_from_this().lock();
            const auto generation = ++content_generation_;
            release_content_input();
            if (generation != content_generation_) {
                return;
            }
            body_->remove_and_delete(*current);
            child_.reset();
            mark_layout_dirty();
            mark_semantics_dirty();
        }
    }

    auto Collapsible::child() const -> scene::NanControl* {
        return child_.lock().get();
    }

    void Collapsible::set_open(const bool open) {
        if (state_->open == open) {
            return;
        }
        // Focus callbacks may release the caller's last owning handle.
        const auto keep_alive = weak_from_this().lock();
        state_->open = open;
        body_->set_visible(open);
        mark_layout_dirty();
        mark_dirty(scene::DirtyFlags::paint);
        trigger_->mark_semantics_dirty();
        if (!open) {
            release_content_input();
        }
    }

    auto Collapsible::open() const noexcept -> bool {
        return state_->open;
    }

    void Collapsible::toggle() {
        if (disabled()) {
            return;
        }
        const auto keep_alive = weak_from_this().lock();
        const bool next = !open();
        set_open(next);
        // Keep the action payload stable if the callback performs a silent sync.
        const auto callback = on_change_;
        if (callback) {
            callback(next);
        }
        open_changed_.emit(next);
    }

    void Collapsible::set_on_change(std::function<void(bool)> callback) {
        on_change_ = std::move(callback);
    }

    auto Collapsible::open_changed() const noexcept -> const reactive::Event<bool>& {
        return open_changed_;
    }

    void Collapsible::set_text(std::string text) {
        require_text(text);
        trigger_->set_text(std::move(text));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Collapsible::text() const -> std::string_view {
        return trigger_->text();
    }

    void Collapsible::set_disabled(const bool disabled) {
        const auto keep_alive = weak_from_this().lock();
        trigger_->set_disabled(disabled);
    }

    auto Collapsible::disabled() const -> bool {
        return trigger_->disabled();
    }

    auto Collapsible::set_gap(const float gap) -> Collapsible& {
        if (!std::isfinite(gap) || gap < 0.0F) {
            throw std::invalid_argument("Collapsible gap must be finite and non-negative");
        }
        if (gap_ != gap) {
            gap_ = gap;
            mark_layout_dirty();
        }
        return *this;
    }

    auto Collapsible::gap() const noexcept -> float {
        return gap_;
    }

    auto Collapsible::trigger() const -> const Button& {
        return *trigger_;
    }

    void Collapsible::set_trigger_override(theme::ButtonRecipeRule rule) {
        trigger_->set_override(std::move(rule));
        mark_layout_dirty();
    }

    auto Collapsible::visual_part(visual::container_t part) noexcept
        -> primitives::BoxPresentation& {
        return trigger_->visual_part(part);
    }

    auto Collapsible::visual_part(visual::label_t part) noexcept -> primitives::TextPresentation& {
        return trigger_->visual_part(part);
    }

    void Collapsible::release_content_input() {
        auto* tree = get_tree();
        if (!tree) {
            return;
        }
        if (auto* captured = tree->pointer_capture();
            captured && (captured == body_.get() || body_->is_ancestor_of(*captured)))
        {
            tree->release_pointer_capture(captured);
        }
        if (auto* focused = tree->focused_node();
            focused && (focused == body_.get() || body_->is_ancestor_of(*focused)))
        {
            const bool usable = trigger_->get_tree() == tree && trigger_->is_visible_in_tree()
                && !trigger_->disabled();
            tree->set_focus(
                usable ? trigger_.get() : nullptr,
                scene::FocusRequestContext {.reason = scene::FocusRequestReason::restore}
            );
        }
    }

    auto Collapsible::geometry(const float available_width) -> Geometry {
        const auto content = child_.lock();
        const bool show_content = open() && content && content->visible();
        float width = available_width;
        if (!std::isfinite(width)) {
            width = trigger_->measure_layout(foundation::NanLayoutConstraints::loose()).get_width();
            if (show_content) {
                width = std::max(
                    width,
                    content->measure_layout(foundation::NanLayoutConstraints::loose()).get_width()
                );
            }
        }
        width = finite_extent(width);
        const foundation::NanLayoutConstraints limits {.min_width = width, .max_width = width};
        const float trigger_height = finite_extent(trigger_->measure_layout(limits).get_height());
        const float content_height =
            show_content ? finite_extent(content->measure_layout(limits).get_height()) : 0.0F;
        const float gap = show_content ? gap_ : 0.0F;
        (void)finite_extent(static_cast<double>(trigger_height) + gap + content_height);
        return {width, trigger_height, content_height, gap};
    }

    auto Collapsible::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const auto size = geometry(constraints.max_width);
        return constraints.constrain(
            foundation::NanSize(
                size.width,
                finite_extent(
                    static_cast<double>(size.trigger_height) + size.gap + size.content_height
                )
            )
        );
    }

    void Collapsible::on_layout() {
        const auto size = geometry(width());
        trigger_->layout_to(foundation::NanRect::from_xywh(0, 0, size.width, size.trigger_height));
        if (open()) {
            body_->layout_to(
                foundation::NanRect::from_xywh(
                    0,
                    finite_extent(static_cast<double>(size.trigger_height) + size.gap),
                    size.width,
                    size.content_height
                )
            );
        }
    }

    auto Collapsible::semantics_properties() const -> semantics::Properties {
        return {.role = semantics::Role::generic, .label = std::string(text())};
    }
} // namespace nandina::widget
