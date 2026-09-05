//
// widget/authoring - thin composition helpers over concrete widgets.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP
#define NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP

#include "../animation/motion.hpp"
#include "../foundation/geometry.hpp"
#include "../reactive/graph.hpp"
#include "../reactive/scope.hpp"
#include "../scene/control.hpp"
#include "../theme/theme.hpp"
#include "grid.hpp"
#include "layout.hpp"
#include "list_view.hpp"
#include "scroll_view.hpp"
#include "visual_property.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::widget::authoring
{
    // Authoring 代码可直接写 percent(50) / fill / content，同时底层类型仍归 scene 所有。
    using scene::content;
    using scene::fill;
    using scene::percent;

    template<typename Node>
    class NodeBuilder;

    namespace detail
    {
        template<typename Node>
        [[nodiscard]] auto materialize(std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
            return node;
        }

        template<typename Node>
        [[nodiscard]] auto materialize(const NodeBuilder<Node>& builder) -> std::shared_ptr<Node> {
            return builder.build();
        }

        template<typename Parent, typename Child>
        concept AddableChild = requires(Parent& parent, Child&& child) {
            parent.add(materialize(std::forward<Child>(child)));
        };

        template<typename Parent, typename Child>
        concept SettableChild = requires(Parent& parent, Child&& child) {
            parent.set_child(materialize(std::forward<Child>(child)));
        };
    } // namespace detail

    template<typename Node>
    class NodeBuilder {
    public:
        explicit NodeBuilder(std::shared_ptr<Node> node): node_(std::move(node)) {
            if (!node_) {
                throw std::invalid_argument("NodeBuilder node cannot be null");
            }
        }

        /// Prevent subsequently installed application callbacks from entering after
        /// their owning build scope has been cleared.
        auto guard_callbacks(std::weak_ptr<void> lifetime) -> NodeBuilder& {
            callback_lifetime_ = std::move(lifetime);
            return *this;
        }

        auto bind_scope(reactive::ReactiveScope& scope) noexcept -> NodeBuilder& {
            binding_scope_ = &scope;
            return *this;
        }

        template<typename... Configure>
            requires(std::invocable<Configure, Node&> && ...)
        auto configure(Configure&&... configure) -> NodeBuilder& {
            (static_cast<void>(std::invoke(std::forward<Configure>(configure), *node_)), ...);
            return *this;
        }

        template<typename Handler>
            requires requires(Node& node, Handler&& handler) {
                node.set_on_click(std::forward<Handler>(handler));
            }
        auto on_click(Handler&& handler) -> NodeBuilder& {
            node_->set_on_click(guarded(std::forward<Handler>(handler)));
            return *this;
        }

        template<typename Handler>
            requires requires(Node& node, Handler&& handler) {
                node.set_on_submit(std::forward<Handler>(handler));
            }
        auto on_submit(Handler&& handler) -> NodeBuilder& {
            node_->set_on_submit(guarded(std::forward<Handler>(handler)));
            return *this;
        }

        template<typename Handler>
            requires requires(Node& node, Handler&& handler) {
                node.set_on_change(std::forward<Handler>(handler));
            }
        auto on_change(Handler&& handler) -> NodeBuilder& {
            node_->set_on_change(guarded(std::forward<Handler>(handler)));
            return *this;
        }

        auto checked(bool checked) -> NodeBuilder&
            requires requires(Node& node) { node.set_checked(checked); }
        {
            node_->set_checked(checked);
            return *this;
        }

        auto value(float value) -> NodeBuilder&
            requires requires(Node& node) { node.set_value(value); }
        {
            node_->set_value(value);
            return *this;
        }

        auto range(float minimum, float maximum) -> NodeBuilder&
            requires requires(Node& node) { node.set_range(minimum, maximum); }
        {
            node_->set_range(minimum, maximum);
            return *this;
        }

        auto step(float step) -> NodeBuilder&
            requires requires(Node& node) { node.set_step(step); }
        {
            node_->set_step(step);
            return *this;
        }

        auto tone(theme::ButtonTone tone) -> NodeBuilder&
            requires requires(Node& node) { node.set_tone(tone); }
        {
            node_->set_tone(tone);
            return *this;
        }

        auto treatment(theme::ButtonTreatment treatment) -> NodeBuilder&
            requires requires(Node& node) { node.set_treatment(treatment); }
        {
            node_->set_treatment(treatment);
            return *this;
        }

        auto gap(float gap) -> NodeBuilder&
            requires requires(Node& node) { node.set_gap(gap); }
        {
            node_->set_gap(gap);
            return *this;
        }

        auto width(float width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto width(scene::PercentLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto width(scene::FillLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto width(scene::ContentLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto height(float height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto height(scene::PercentLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto height(scene::FillLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto height(scene::ContentLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto min_width(float width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_min_width(width);
            return *this;
        }

        auto min_width(scene::PercentLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_min_width(width);
            return *this;
        }

        auto max_width(float width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_max_width(width);
            return *this;
        }

        auto max_width(scene::PercentLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_max_width(width);
            return *this;
        }

        auto min_height(float height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_min_height(height);
            return *this;
        }

        auto min_height(scene::PercentLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_min_height(height);
            return *this;
        }

        auto max_height(float height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_max_height(height);
            return *this;
        }

        auto max_height(scene::PercentLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_max_height(height);
            return *this;
        }

        auto aspect_ratio(float ratio) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_aspect_ratio(ratio);
            return *this;
        }

        auto cross_alignment(LayoutAlignment alignment) -> NodeBuilder&
            requires requires(Node& node) { node.set_cross_alignment(alignment); }
        {
            node_->set_cross_alignment(alignment);
            return *this;
        }

        auto font_size(float size) -> NodeBuilder&
            requires requires(Node& node) { node.set_font_size(size); }
        {
            node_->set_font_size(size);
            return *this;
        }

        auto font_size(scene::PercentLength size) -> NodeBuilder&
            requires requires(Node& node) { node.set_font_size(size); }
        {
            node_->set_font_size(size);
            return *this;
        }

        auto color_token(theme::ColorToken token) -> NodeBuilder&
            requires requires(Node& node) { node.set_color_token(token); }
        {
            node_->set_color_token(token);
            return *this;
        }

        auto wheel_step(float step) -> NodeBuilder&
            requires requires(Node& node) { node.set_wheel_step(step); }
        {
            node_->set_wheel_step(step);
            return *this;
        }

        auto autofocus() -> NodeBuilder&
            requires requires(Node& node) { node.request_focus(); }
        {
            node_->request_focus();
            return *this;
        }

        template<visual::Path Path, typename Value>
            requires property::WritableValue<Node, Path, Value>
        auto set(Path path, Value&& value) -> NodeBuilder& {
            property::write(*node_, path, std::forward<Value>(value));
            return *this;
        }

        template<visual::Path Path, typename Source>
            requires property::Writable<Node, Path> && requires(Source& source) {
                { source.get() } -> std::convertible_to<property::value_t<Path>>;
            }
        auto bind(Path path, Source& source) -> NodeBuilder& {
            if (binding_scope_ == nullptr) {
                throw std::logic_error("NodeBuilder property binding requires a BuildContext");
            }
            binding_scope_->effect([weak = std::weak_ptr<Node>(node_), path, &source] {
                if (const auto current = weak.lock()) {
                    property::write(*current, path, source.get());
                }
            });
            return *this;
        }

        template<visual::Path Path>
            requires property::Animatable<Node, Path>
        auto behavior(Path path, animation::Behavior<property::value_t<Path>> behavior)
            -> NodeBuilder& {
            property::set_behavior(*node_, path, std::move(behavior));
            return *this;
        }

        /// 声明式缓动：`.behavior(visual::container.radius, motion::tween(0.3F).easing(...))`。
        template<visual::Path Path>
            requires property::Animatable<Node, Path>
        auto behavior(Path path, animation::motion::TweenSpec spec) -> NodeBuilder& {
            property::set_behavior(*node_, path, spec.behavior<property::value_t<Path>>());
            return *this;
        }

        /// 声明式弹簧：`.spring(visual::container.radius, motion::spring().stiffness(280.0F))`。
        template<visual::Path Path>
            requires property::Springable<Node, Path>
        auto spring(Path path, animation::SpringSpec spec) -> NodeBuilder& {
            property::set_spring(*node_, path, std::move(spec));
            return *this;
        }

        template<typename... Child>
            requires(detail::AddableChild<Node, Child> && ...)
        auto children(Child&&... child) -> NodeBuilder& {
            (static_cast<void>(node_->add(detail::materialize(std::forward<Child>(child)))), ...);
            return *this;
        }

        template<typename Child>
            requires detail::SettableChild<Node, Child>
        auto child(Child&& child) -> NodeBuilder& {
            node_->set_child(detail::materialize(std::forward<Child>(child)));
            return *this;
        }

        auto expose(std::shared_ptr<Node>& target) -> NodeBuilder& {
            target = node_;
            return *this;
        }

        [[nodiscard]] auto get() noexcept -> Node& {
            return *node_;
        }

        [[nodiscard]] auto get() const noexcept -> const Node& {
            return *node_;
        }

        [[nodiscard]] auto build() const -> std::shared_ptr<Node> {
            return node_;
        }

    private:
        template<typename Handler>
        [[nodiscard]] auto guarded(Handler&& handler) const {
            return [lifetime = callback_lifetime_,
                    handler = std::forward<Handler>(handler)](auto&&... args) mutable {
                if (lifetime.has_value() && lifetime->expired()) {
                    return;
                }
                static_cast<void>(std::invoke(handler, std::forward<decltype(args)>(args)...));
            };
        }

        std::shared_ptr<Node> node_;
        std::optional<std::weak_ptr<void>> callback_lifetime_;
        reactive::ReactiveScope* binding_scope_ = nullptr;
    };

    template<typename Node, typename... Args>
        requires std::constructible_from<Node, Args...>
    [[nodiscard]] auto make(Args&&... args) -> NodeBuilder<Node> {
        return NodeBuilder<Node>(std::make_shared<Node>(std::forward<Args>(args)...));
    }

    template<typename Node>
    [[nodiscard]] auto from(std::shared_ptr<Node> node) -> NodeBuilder<Node> {
        return NodeBuilder<Node>(std::move(node));
    }

    // ──自由函数工厂：用简洁的名称创建常见控件 ────────────────
    // 这些是 make<T>(args...) 的语义别名，不引入新的对象模型。

    // ── 布局 ──

    [[nodiscard]] inline auto row() -> NodeBuilder<Row> {
        return make<Row>();
    }

    [[nodiscard]] inline auto column() -> NodeBuilder<Column> {
        return make<Column>();
    }

    [[nodiscard]] inline auto flex(LayoutAxis axis = LayoutAxis::horizontal) -> NodeBuilder<Flex> {
        return make<Flex>(axis);
    }

    [[nodiscard]] inline auto padding(foundation::NanInsets insets) -> NodeBuilder<Padding> {
        return make<Padding>(insets);
    }

    [[nodiscard]] inline auto center() -> NodeBuilder<Center> {
        return make<Center>();
    }

    [[nodiscard]] inline auto expanded(int flex_factor = 1) -> NodeBuilder<Expanded> {
        return make<Expanded>(flex_factor);
    }

    [[nodiscard]] inline auto flex_item(scene::LayoutFlexPolicy policy = {})
        -> NodeBuilder<FlexItem> {
        return make<FlexItem>(policy);
    }

    [[nodiscard]] inline auto scroll_view(ScrollAxis axis = ScrollAxis::vertical)
        -> NodeBuilder<ScrollView> {
        return make<ScrollView>(axis);
    }

    [[nodiscard]] inline auto grid(int columns = 2) -> NodeBuilder<Grid> {
        return make<Grid>(columns);
    }

    [[nodiscard]] inline auto stack() -> NodeBuilder<Stack> {
        return make<Stack>();
    }

} // namespace nandina::widget::authoring

namespace nandina::widget
{
    /// Public root-view result. Builders remain the preferred composition type and
    /// materialize to this retained scene node at the page boundary.
    using View = std::shared_ptr<scene::NanNode2D>;
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP
