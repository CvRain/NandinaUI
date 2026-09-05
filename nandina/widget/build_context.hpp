//
// widget/build_context - explicit authoring services for one build scope.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP

#include "../reactive/graph.hpp"
#include "../reactive/scope.hpp"
#include "../theme/theme_manager.hpp"
#include "authoring.hpp"
#include "component_traits.hpp"

#include <cmath>
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::resource
{
    class ResourceManager;
}

namespace nandina::widget
{
    namespace build_context_detail
    {
        template<typename Source>
        using list_item_t =
            typename std::remove_cvref_t<decltype(std::declval<Source&>().get())>::value_type;
    } // namespace build_context_detail

    /// Lightweight, non-owning services passed through page and component construction.
    /// A derived region replaces only the reactive scope; graph and theme stay page-wide.
    class BuildContext {
    public:
        BuildContext(
            reactive::Graph& graph,
            reactive::ReactiveScope& scope,
            theme::ThemeManager& themes,
            resource::ResourceManager* resources = nullptr
        ) noexcept:
            BuildContext(graph, scope, themes, scope, resources) {}

        [[nodiscard]] auto graph() const noexcept -> reactive::Graph& {
            return *graph_;
        }

        [[nodiscard]] auto scope() const noexcept -> reactive::ReactiveScope& {
            return *scope_;
        }

        [[nodiscard]] auto theme() const noexcept -> const theme::NanTheme& {
            return themes_->theme();
        }

        [[nodiscard]] auto theme_manager() const noexcept -> theme::ThemeManager& {
            return *themes_;
        }

        [[nodiscard]] auto resource_manager() const noexcept -> resource::ResourceManager* {
            return resources_;
        }

        [[nodiscard]] auto with_scope(reactive::ReactiveScope& scope) const noexcept
            -> BuildContext {
            return BuildContext(*graph_, scope, *themes_, *callback_scope_, resources_);
        }

        template<typename T, typename... Args>
        [[nodiscard]] auto signal(Args&&... args) const -> reactive::Signal<T>& {
            return scope_->signal<T>(std::forward<Args>(args)...);
        }

        template<typename T>
        [[nodiscard]] auto signal_value(T initial) const -> reactive::Signal<T>& {
            return scope_->signal_value(std::move(initial));
        }

        template<typename Fn>
            requires std::invocable<Fn>
        [[nodiscard]] auto computed(Fn&& fn) const
            -> reactive::Computed<std::invoke_result_t<Fn>>& {
            return scope_->computed(std::forward<Fn>(fn));
        }

        template<typename Fn>
            requires std::invocable<Fn>
        [[nodiscard]] auto effect(Fn&& fn) const -> reactive::Effect& {
            return scope_->effect(std::forward<Fn>(fn));
        }

        template<typename... Args, typename Handler>
        void connect(const reactive::Event<Args...>& event, Handler&& handler) const {
            scope_->connect(event, std::forward<Handler>(handler));
        }

        /// Bind a tracked source to an ordinary widget setter. The current build
        /// scope owns the effect, while a weak target prevents detached widgets
        /// from being kept alive solely by a binding.
        template<typename Node, typename Setter, typename Source>
            requires requires(Node& node, Setter setter, Source& source) {
                std::invoke(setter, node, source.get());
            }
        void bind(const std::shared_ptr<Node>& target, Setter setter, Source& source) const {
            scope_->effect(
                [weak = std::weak_ptr<Node>(target), setter = std::move(setter), &source] {
                    if (const auto current = weak.lock()) {
                        std::invoke(setter, *current, source.get());
                    }
                }
            );
        }

        /// Bind a tracked source to a shared typed visual property path. Unsupported
        /// node/part/field combinations are rejected by the Writable concept.
        template<typename Node, visual::Path Path, typename Source>
            requires property::Writable<Node, Path> && requires(Source& source) {
                { source.get() } -> std::convertible_to<property::value_t<Path>>;
            }
        void bind(const std::shared_ptr<Node>& target, Path path, Source& source) const {
            scope_->effect([weak = std::weak_ptr<Node>(target), path, &source] {
                if (const auto current = weak.lock()) {
                    property::write(*current, path, source.get());
                }
            });
        }

        /// Construct a custom component with its own reactive lifetime. The component
        /// receives the derived context and releases its subscriptions/effects before
        /// the concrete node is destroyed.
        template<typename Node, typename... Args>
            requires std::derived_from<Node, scene::NanNode>
        [[nodiscard]] auto make(Args&&... args) const {
            if constexpr (requires {
                              ComponentTraits<Node>::make(
                                  std::declval<const BuildContext&>(),
                                  std::declval<Args>()...
                              );
                          })
            {
                auto result = ComponentTraits<Node>::make(*this, std::forward<Args>(args)...);
                return prepare_builder(std::move(result));
            }
            else {
                static_assert(
                    std::constructible_from<Node, BuildContext, Args...>,
                    "Component must provide ComponentTraits<T>::make(BuildContext&, ...) "
                    "or a BuildContext-aware constructor"
                );
                auto result = make_scoped_component<Node>(std::forward<Args>(args)...);
                return prepare_builder(std::move(result));
            }
        }

    private:
        BuildContext(
            reactive::Graph& graph,
            reactive::ReactiveScope& scope,
            theme::ThemeManager& themes,
            reactive::ReactiveScope& callback_scope,
            resource::ResourceManager* resources
        ) noexcept:
            graph_(&graph),
            scope_(&scope),
            themes_(&themes),
            callback_scope_(&callback_scope),
            resources_(resources) {}

        template<typename Node, typename... Args>
        [[nodiscard]] auto make_scoped_component(Args&&... args) const
            -> authoring::NodeBuilder<Node> {
            auto scope = std::make_unique<reactive::ReactiveScope>(*graph_);
            auto component =
                std::unique_ptr<Node>(new Node(with_scope(*scope), std::forward<Args>(args)...));
            auto owned = std::shared_ptr<Node>(
                component.release(),
                [scope = std::move(scope)](Node* node) mutable {
                    scope->clear();
                    delete node;
                    scope.reset();
                }
            );
            return authoring::from(std::move(owned));
        }

        template<typename Node>
        [[nodiscard]] auto prepare_builder(authoring::NodeBuilder<Node> builder) const
            -> authoring::NodeBuilder<Node> {
            builder.guard_callbacks(callback_scope_->lifetime());
            builder.bind_scope(*scope_);
            return builder;
        }

    public:
        [[nodiscard]] auto row() const -> authoring::NodeBuilder<Row> {
            return prepare_builder(authoring::row());
        }

        [[nodiscard]] auto column() const -> authoring::NodeBuilder<Column> {
            return prepare_builder(authoring::column());
        }

        [[nodiscard]] auto flex(LayoutAxis axis = LayoutAxis::horizontal) const
            -> authoring::NodeBuilder<Flex> {
            return prepare_builder(authoring::flex(axis));
        }

        [[nodiscard]] auto padding(foundation::NanInsets insets) const
            -> authoring::NodeBuilder<Padding> {
            return prepare_builder(authoring::padding(insets));
        }

        [[nodiscard]] auto center() const -> authoring::NodeBuilder<Center> {
            return prepare_builder(authoring::center());
        }

        [[nodiscard]] auto expanded(int flex_factor = 1) const -> authoring::NodeBuilder<Expanded> {
            return prepare_builder(authoring::expanded(flex_factor));
        }

        [[nodiscard]] auto flex_item(scene::LayoutFlexPolicy policy = {}) const
            -> authoring::NodeBuilder<FlexItem> {
            return prepare_builder(authoring::flex_item(policy));
        }

        [[nodiscard]] auto scroll_view(ScrollAxis axis = ScrollAxis::vertical) const
            -> authoring::NodeBuilder<ScrollView> {
            return prepare_builder(authoring::scroll_view(axis));
        }

        [[nodiscard]] auto grid(int columns = 2) const -> authoring::NodeBuilder<Grid> {
            return prepare_builder(authoring::grid(columns));
        }

        [[nodiscard]] auto stack() const -> authoring::NodeBuilder<Stack> {
            return prepare_builder(authoring::stack());
        }

        template<typename Source, typename TrueFactory>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<bool>;
            } && std::invocable<TrueFactory&, BuildContext>
        [[nodiscard]] auto when(Source& source, TrueFactory&& when_true) const
            -> authoring::NodeBuilder<IfRegion<scene::NanControl>> {
            using Region = IfRegion<scene::NanControl>;
            auto region =
                Region::create(*graph_, make_branch_factory(std::forward<TrueFactory>(when_true)));
            region->bind(source);
            return prepare_builder(authoring::from(std::move(region)));
        }

        template<typename Source, typename TrueFactory, typename FalseFactory>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<bool>;
            }
        && std::invocable<TrueFactory&, BuildContext> && std::invocable<FalseFactory&, BuildContext>
        [[nodiscard]] auto
        when(Source& source, TrueFactory&& when_true, FalseFactory&& when_false) const
            -> authoring::NodeBuilder<IfRegion<scene::NanControl>> {
            using Region = IfRegion<scene::NanControl>;
            auto region = Region::create(
                *graph_,
                make_branch_factory(std::forward<TrueFactory>(when_true)),
                make_branch_factory(std::forward<FalseFactory>(when_false))
            );
            region->bind(source);
            return prepare_builder(authoring::from(std::move(region)));
        }

        template<
            typename Source,
            typename KeyFunction,
            typename CreateFunction,
            typename UpdateFunction = std::nullptr_t>
            requires ListDataModelSource<Source, build_context_detail::list_item_t<Source>>
            && std::invocable<KeyFunction&, const build_context_detail::list_item_t<Source>&>
            && std::invocable<
                         CreateFunction&,
                         BuildContext,
                         const build_context_detail::list_item_t<Source>&>
        [[nodiscard]] auto for_each(
            Source& source,
            KeyFunction&& key,
            CreateFunction&& create,
            UpdateFunction&& update = nullptr
        ) const {
            using Item = build_context_detail::list_item_t<Source>;
            using Key = std::remove_cvref_t<std::invoke_result_t<KeyFunction&, const Item&>>;
            using CreateResult = std::invoke_result_t<CreateFunction&, BuildContext, const Item&>;
            using NodePointer =
                decltype(authoring::detail::materialize(std::declval<CreateResult>()));
            using Node = typename NodePointer::element_type;
            static_assert(std::derived_from<Node, scene::NanControl>);
            using View = ListView<Item, Key, Node>;

            typename View::UpdateFunction update_node;
            if constexpr (!std::same_as<std::remove_cvref_t<UpdateFunction>, std::nullptr_t>) {
                static_assert(std::invocable<UpdateFunction&, Node&, const Item&>);
                update_node = [update = std::forward<UpdateFunction>(update)](
                                  Node& node,
                                  const Item& item
                              ) mutable { std::invoke(update, node, item); };
            }

            auto view = View::create(
                *graph_,
                [key = std::forward<KeyFunction>(key)](const Item& item) mutable {
                    return std::invoke(key, item);
                },
                [ui = *this, create = std::forward<CreateFunction>(create)](
                    reactive::ReactiveScope& scope,
                    const Item& item
                ) mutable {
                    return authoring::detail::materialize(
                        std::invoke(create, ui.with_scope(scope), item)
                    );
                },
                std::move(update_node)
            );
            view->set_model(source);
            return prepare_builder(authoring::from(std::move(view)));
        }

    private:
        template<typename Factory>
        [[nodiscard]] auto make_branch_factory(Factory&& factory) const ->
            typename IfRegion<scene::NanControl>::CreateFunction {
            return [ui = *this, factory = std::forward<Factory>(factory)](
                       reactive::ReactiveScope& scope
                   ) mutable -> std::shared_ptr<scene::NanControl> {
                auto node =
                    authoring::detail::materialize(std::invoke(factory, ui.with_scope(scope)));
                static_assert(
                    std::derived_from<typename decltype(node)::element_type, scene::NanControl>
                );
                return node;
            };
        }

        reactive::Graph* graph_;
        reactive::ReactiveScope* scope_;
        theme::ThemeManager* themes_;
        reactive::ReactiveScope* callback_scope_;
        resource::ResourceManager* resources_ = nullptr;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP
