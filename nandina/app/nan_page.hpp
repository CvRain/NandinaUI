//
// app/nan_page — typed page and page context contracts.
//
// Route params are downward data (Angular route params / Svelte page data style).
// Shared app state lives in a developer-defined NanStore and is accessed through
// PageContext, so deep pages can update the store and keep-alive ancestor pages
// react through Signal/Effect without reverse route plumbing.
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
#include <memory>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
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
            UiDispatcher* dispatcher = nullptr
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
            dispatcher_(dispatcher) {}

        [[nodiscard]] auto router() -> NanRouter& {
            return *router_;
        }

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
            return widget::BuildContext(graph(), scope(), theme_manager(), resources_);
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
    };

    class NanPage {
    public:
        virtual ~NanPage() = default;

        NanPage(const NanPage&) = delete;
        auto operator=(const NanPage&) -> NanPage& = delete;
        NanPage(NanPage&&) = delete;
        auto operator=(NanPage&&) -> NanPage& = delete;

        [[nodiscard]] virtual auto route_key() const -> std::string_view = 0;
        [[nodiscard]] virtual auto params_type_key() const -> NanTypeKey = 0;
        [[nodiscard]] virtual auto build(PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> = 0;

        /// Called when this keep-alive page becomes the top of the stack (visible again).
        /// The default implementation is a no-op. Pages override this to refresh
        /// state that may have changed while the page was hidden (e.g. visit counters,
        /// live data from a shared Store).
        virtual void on_activate(PageContext& context) {}

        /// Called when this keep-alive page is no longer the top of the stack (hidden
        /// by another page being pushed on top). The page root remains mounted and
        /// reactive bindings stay active. The default implementation is a no-op.
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

    /**
     * Application-facing page base. The router still owns PageContext and the page
     * scope; ordinary pages only implement the IDE-friendly BuildContext overload.
     */
    template<typename ParamsT = NoParams>
    class Page: public NanPageT<ParamsT> {
    public:
        using Params = ParamsT;
        using NanPageT<ParamsT>::NanPageT;

        [[nodiscard]] auto route_key() const -> std::string_view override {
            // 仅作为进程内默认身份；持久化/深链接页面应覆写为稳定字符串。
            return typeid(*this).name();
        }

        [[nodiscard]] auto build(PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> final {
            auto ui = context.ui();
            return build(ui);
        }

        [[nodiscard]] virtual auto build(widget::BuildContext& ui) -> widget::View = 0;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP
