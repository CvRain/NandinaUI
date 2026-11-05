// widget/collapsible — disclosure button and a retained single content slot.
#ifndef NANDINA_EXPERIMENT_WIDGET_COLLAPSIBLE_HPP
#define NANDINA_EXPERIMENT_WIDGET_COLLAPSIBLE_HPP

#include "../reactive/event.hpp"
#include "button.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nandina::widget
{
    /// A disclosure reuses Button input/style and hides its retained content host.
    /// Programmatic set_open is silent; user toggle emits open_changed and on_change.
    class Collapsible final: public scene::NanControl {
    public:
        explicit Collapsible(std::string text, theme::NanTheme theme = theme::default_theme());
        ~Collapsible() override;
        [[nodiscard]] static auto
        create(std::string text, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Collapsible>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Collapsible&;
        void clear_child();
        [[nodiscard]] auto child() const -> scene::NanControl*;
        void set_open(bool open);
        [[nodiscard]] auto open() const noexcept -> bool;
        void toggle();
        void set_on_change(std::function<void(bool)> callback);
        [[nodiscard]] auto open_changed() const noexcept -> const reactive::Event<bool>&;
        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;
        auto set_gap(float gap) -> Collapsible&;
        [[nodiscard]] auto gap() const noexcept -> float;

        [[nodiscard]] auto trigger() const -> const Button&;
        void set_trigger_override(theme::ButtonRecipeRule rule);
        [[nodiscard]] auto visual_part(visual::container_t) noexcept
            -> primitives::BoxPresentation&;
        [[nodiscard]] auto visual_part(visual::label_t) noexcept -> primitives::TextPresentation&;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        struct State;
        class Trigger;
        struct Geometry {
            float width;
            float trigger_height;
            float content_height;
            float gap;
        };
        [[nodiscard]] auto geometry(float width) -> Geometry;
        void release_content_input();

        std::shared_ptr<State> state_;
        std::shared_ptr<Trigger> trigger_;
        std::shared_ptr<scene::NanControl> body_;
        std::weak_ptr<scene::NanControl> child_;
        float gap_ = 0.0F;
        std::size_t content_generation_ = 0;
        std::function<void(bool)> on_change_;
        reactive::Event<bool> open_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_COLLAPSIBLE_HPP
