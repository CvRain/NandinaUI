//
// app/nan_page — typed page and page context contracts.
//
// Route params are downward data (Angular route params / Svelte page data style).
// Shared app state lives in a developer-defined NanStore and is accessed through
// PageContext, so deep pages can update the store and later page builds can read
// the same draft without reverse route plumbing.
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP
#define NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP

#include "../reactive/graph.hpp"
#include "../reactive/scope.hpp"
#include "../resource/resource_manager.hpp"
#include "../scene/node2d.hpp"
#include "../theme/theme.hpp"
#include "../widget/build_context.hpp"
#include "async_scope.hpp"
#include "nan_store.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nandina::text
{
    class FontLoader;
    class FontFamilyRegistry;
} // namespace nandina::text

namespace nandina::theme
{
    class ThemeManager;
}

namespace nandina::app
{

    using NanTypeKey = const void*;

    struct NoParams {};

    template<typename T>
    [[nodiscard]] auto nan_type_key() -> NanTypeKey {
        static const int token = 0;
        return &token;
    }

    class NanPage;
    template<typename ParamsT>
    class NanPageT;

    namespace detail
    {
        struct NavigationState {
            std::move_only_function<bool(NanTypeKey, std::unique_ptr<NanPage>)> submit;
        };
    } // namespace detail

    class Navigation {
    public:
        Navigation() = default;

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        [[nodiscard]] auto navigate(ParamsT params) const -> bool {
            return submit<PageT>(std::make_unique<PageT>(std::move(params)));
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto navigate() const -> bool {
            return submit<PageT>(std::make_unique<PageT>());
        }

        [[nodiscard]] auto valid() const noexcept -> bool {
            return !state_.expired();
        }

    private:
        explicit Navigation(std::shared_ptr<detail::NavigationState> state): state_(std::move(state)) {}

        template<typename PageT>
        [[nodiscard]] auto submit(std::unique_ptr<PageT> page) const -> bool {
            const auto state = state_.lock();
            if (!state || !state->submit) {
                return false;
            }
            return state->submit(nan_type_key<PageT>(), std::move(page));
        }

        friend class NanRouter;
        friend class PageContext;
        std::weak_ptr<detail::NavigationState> state_;
    };

    class NanRouter;

    class PageContext {
    public:
        PageContext(
            NanRouter& router,
            reactive::Graph& graph,
            reactive::ReactiveScope& scope,
            const theme::NanTheme& theme,
            NanStore* store,
            NanTypeKey store_key,
            resource::ResourceManager* resources = nullptr,
            text::FontLoader* font_loader = nullptr,
            text::FontFamilyRegistry* font_families = nullptr,
            AsyncScope* async_scope = nullptr,
            theme::ThemeManager* theme_manager = nullptr,
            UiDispatcher* dispatcher = nullptr,
            scene::OverlayHost* overlay_host = nullptr,
            widget::DragController* drag_controller = nullptr,
            Navigation navigation = {}
        ):
            router_(&router),
            graph_(&graph),
            scope_(&scope),
            theme_(&theme),
            store_(store),
            store_key_(store_key),
            resources_(resources),
            font_loader_(font_loader),
            font_families_(font_families),
            async_scope_(async_scope),
            theme_manager_(theme_manager),
            dispatcher_(dispatcher),
            overlay_host_(overlay_host),
            drag_controller_(drag_controller),
            navigation_(std::move(navigation)) {}

        [[nodiscard]] auto router() -> NanRouter& {
            return *router_;
        }

        /// Copyable, non-owning navigation capability. Capture this value in
        /// callbacks; never capture PageContext itself by reference.
        [[nodiscard]] auto navigation() const -> Navigation { return navigation_; }

        [[nodiscard]] auto graph() -> reactive::Graph& {
            return *graph_;
        }

        [[nodiscard]] auto scope() -> reactive::ReactiveScope& {
            return *scope_;
        }

        [[nodiscard]] auto theme() const -> const theme::NanTheme& {
            return *theme_;
        }

        [[nodiscard]] auto has_theme_manager() const noexcept -> bool {
            return theme_manager_ != nullptr;
        }

        [[nodiscard]] auto theme_manager() -> theme::ThemeManager& {
            if (!theme_manager_) {
                throw std::runtime_error("PageContext::theme_manager: service is unavailable");
            }
            return *theme_manager_;
        }

        [[nodiscard]] auto ui() -> widget::BuildContext {
            return widget::BuildContext(
                graph(),
                scope(),
                theme_manager(),
                resources_,
                overlay_host_,
                drag_controller_
            );
        }

        [[nodiscard]] auto has_store() const -> bool {
            return store_ != nullptr;
        }

        [[nodiscard]] auto has_resource_services() const -> bool {
            return resources_ != nullptr && font_loader_ != nullptr && font_families_ != nullptr;
        }

        [[nodiscard]] auto has_async_scope() const noexcept -> bool {
            return async_scope_ != nullptr;
        }

        [[nodiscard]] auto async_scope() -> AsyncScope& {
            if (!async_scope_) {
                throw std::runtime_error("PageContext::async_scope: service is unavailable");
            }
            return *async_scope_;
        }

        [[nodiscard]] auto has_dispatcher() const noexcept -> bool {
            return dispatcher_ != nullptr;
        }

        [[nodiscard]] auto dispatcher() -> UiDispatcher& {
            if (!dispatcher_) {
                throw std::runtime_error("PageContext::dispatcher: service is unavailable");
            }
            return *dispatcher_;
        }

        [[nodiscard]] auto has_overlay_host() const noexcept -> bool {
            return overlay_host_ != nullptr;
        }

        /// Window-installed overlay portal. Pages normally reach it through
        /// `ui().overlay_host()`; this accessor exists for contexts that need it
        /// without constructing a BuildContext.
        [[nodiscard]] auto overlay_host() -> scene::OverlayHost& {
            if (overlay_host_ == nullptr) {
                throw std::runtime_error("PageContext::overlay_host: service is unavailable");
            }
            return *overlay_host_;
        }

        [[nodiscard]] auto resources() -> resource::ResourceManager& {
            if (!resources_) {
                throw std::runtime_error("PageContext::resources: services are unavailable");
            }
            return *resources_;
        }

        [[nodiscard]] auto font_loader() -> text::FontLoader& {
            if (!font_loader_) {
                throw std::runtime_error("PageContext::font_loader: services are unavailable");
            }
            return *font_loader_;
        }

        [[nodiscard]] auto font_families() -> text::FontFamilyRegistry& {
            if (!font_families_) {
                throw std::runtime_error("PageContext::font_families: services are unavailable");
            }
            return *font_families_;
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        [[nodiscard]] auto store() -> StoreT& {
            if (store_ == nullptr || store_key_ != nan_type_key<StoreT>()) {
                throw std::runtime_error(
                    "PageContext::store: requested store type is not installed"
                );
            }
            return static_cast<StoreT&>(*store_);
        }

    private:
        NanRouter* router_;
        reactive::Graph* graph_;
        reactive::ReactiveScope* scope_;
        const theme::NanTheme* theme_;
        NanStore* store_;
        NanTypeKey store_key_ = nullptr;
        resource::ResourceManager* resources_ = nullptr;
        text::FontLoader* font_loader_ = nullptr;
        text::FontFamilyRegistry* font_families_ = nullptr;
        AsyncScope* async_scope_ = nullptr;
        theme::ThemeManager* theme_manager_ = nullptr;
        UiDispatcher* dispatcher_ = nullptr;
        scene::OverlayHost* overlay_host_ = nullptr;
        widget::DragController* drag_controller_ = nullptr;
        Navigation navigation_;
    };

    class NanPage {
    public:
        virtual ~NanPage() = default;

        NanPage(const NanPage&) = delete;
        auto operator=(const NanPage&) -> NanPage& = delete;
        NanPage(NanPage&&) = delete;
        auto operator=(NanPage&&) -> NanPage& = delete;

        /// Legacy identity used by the stack router. Route mode obtains identity
        /// from Routes, so modern pages may leave this empty.
        [[nodiscard]] virtual auto route_key() const -> std::string_view { return {}; }
        [[nodiscard]] virtual auto params_type_key() const -> NanTypeKey = 0;
        [[nodiscard]] virtual auto build(PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> = 0;

        /// Legacy keep-alive hook. Route mode does not call it.
        virtual void on_activate(PageContext& context) {}

        /// Legacy keep-alive hook. Route mode does not call it.
        virtual void on_deactivate(PageContext& context) {}

    protected:
        NanPage() = default;
    };

    template<typename ParamsT>
    class NanPageT: public NanPage {
    public:
        using Params = ParamsT;

        NanPageT()
            requires std::default_initializable<Params>
        = default;
        explicit NanPageT(Params params): params_(std::move(params)) {}

        [[nodiscard]] auto params() const -> const Params& {
            return params_;
        }

        [[nodiscard]] auto params() -> Params& {
            return params_;
        }

        [[nodiscard]] auto params_type_key() const -> NanTypeKey override {
            return nan_type_key<Params>();
        }

    private:
        Params params_;
    };

    /// Application-facing page base for the route model. A page receives the full
    /// app context exactly once; BuildContext remains a widget-layer value.
    template<typename ParamsT = NoParams>
    class Page: public NanPageT<ParamsT> {
    public:
        using Params = ParamsT;
        using NanPageT<ParamsT>::NanPageT;

        [[nodiscard]] virtual auto build(PageContext& context) -> widget::View = 0;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP
