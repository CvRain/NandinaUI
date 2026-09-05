//
// app/root_view - functional root view adapter for ordinary single-window apps.
//

#ifndef NANDINA_EXPERIMENT_APP_ROOT_VIEW_HPP
#define NANDINA_EXPERIMENT_APP_ROOT_VIEW_HPP

#include "nan_page.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace nandina::app
{
    template<typename Factory>
    concept RootViewFactory =
        std::invocable<Factory&, PageContext&> || std::invocable<Factory&, widget::BuildContext&>;

    namespace detail
    {
        using ErasedRootViewFactory =
            std::move_only_function<std::shared_ptr<scene::NanNode2D>(PageContext&)>;

        struct RootViewParams {
            ErasedRootViewFactory factory;
        };

        class RootViewPage final: public NanPageT<RootViewParams> {
        public:
            explicit RootViewPage(RootViewParams params): NanPageT(std::move(params)) {}

            [[nodiscard]] auto route_key() const -> std::string_view override {
                return "root";
            }

            [[nodiscard]] auto build(PageContext& context)
                -> std::shared_ptr<scene::NanNode2D> override {
                if (!params().factory) {
                    throw std::invalid_argument("RootViewPage: root factory is empty");
                }
                return std::invoke(params().factory, context);
            }
        };

        template<typename Factory>
            requires RootViewFactory<std::decay_t<Factory>>
        [[nodiscard]] auto make_root_view_params(Factory&& factory) -> RootViewParams {
            return RootViewParams {
                .factory = [factory = std::forward<Factory>(factory)](
                               PageContext& context
                           ) mutable -> std::shared_ptr<scene::NanNode2D> {
                    if constexpr (std::invocable<Factory&, PageContext&>) {
                        auto result = std::invoke(factory, context);
                        auto root = widget::authoring::detail::materialize(std::move(result));
                        static_assert(
                            std::derived_from<
                                typename decltype(root)::element_type,
                                scene::NanNode2D>,
                            "root view factories must return a Node2D builder or shared pointer"
                        );
                        return root;
                    }
                    else {
                        auto ui = context.ui();
                        auto result = std::invoke(factory, ui);
                        auto root = widget::authoring::detail::materialize(std::move(result));
                        static_assert(
                            std::derived_from<
                                typename decltype(root)::element_type,
                                scene::NanNode2D>,
                            "root view factories must return a Node2D builder or shared pointer"
                        );
                        return root;
                    }
                },
            };
        }
    } // namespace detail
} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_ROOT_VIEW_HPP
