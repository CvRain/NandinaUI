//
// widget/list_view - typed list model binding over the keyed region runtime.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_LIST_VIEW_HPP
#define NANDINA_EXPERIMENT_WIDGET_LIST_VIEW_HPP

#include "declarative.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <vector>

namespace nandina::widget
{

    template<typename Source, typename Item>
    concept ListDataModelSource = requires(Source& source) {
        { source.get() } -> std::same_as<const std::vector<Item>&>;
    };

    /// Typed application-facing list view over ForEach's keyed reconciliation.
    /// A source only needs a tracked get() returning a vector; it does not inherit a model base.
    template<typename Item, std::equality_comparable Key, typename NodeT = scene::NanControl>
        requires std::derived_from<NodeT, scene::NanControl>
    class ListView: public ForEach<Item, Key, NodeT> {
    public:
        using Base = ForEach<Item, Key, NodeT>;
        using KeyFunction = typename Base::KeyFunction;
        using CreateFunction = typename Base::CreateFunction;
        using UpdateFunction = typename Base::UpdateFunction;

        ListView(
            reactive::Graph& graph,
            KeyFunction key,
            CreateFunction create,
            UpdateFunction update = {},
            LayoutAxis axis = LayoutAxis::vertical
        ):
            Base(graph, std::move(key), std::move(create), std::move(update), axis) {}

        [[nodiscard]] static auto create(
            reactive::Graph& graph,
            KeyFunction key,
            CreateFunction create,
            UpdateFunction update = {},
            LayoutAxis axis = LayoutAxis::vertical
        ) -> std::shared_ptr<ListView> {
            return std::make_shared<ListView>(
                graph,
                std::move(key),
                std::move(create),
                std::move(update),
                axis
            );
        }

        template<typename Source>
            requires ListDataModelSource<Source, Item>
        void set_model(Source& source) {
            Base::bind(source);
        }
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LIST_VIEW_HPP
