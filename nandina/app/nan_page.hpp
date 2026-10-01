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
#include <cstdint>
#include <expected>
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

    /// 类型令牌的类别标签。
    ///
    /// 令牌本身只是「某个类型独有的一段地址」，本身不携带任何类别信息：旧的
    /// `const void*` 让页面键、参数键、Store 键在类型层面完全一样，传错位置不会
    /// 报错，只会在运行时"悄悄匹配不上"。标签把类别放回**编译期**——把 Store 键
    /// 交给路由查询、把页面键交给 `set_store()` 都会直接编译失败。
    struct PageTag {};
    struct ParamsTag {};
    struct StoreTag {};

    /// 带类别的类型令牌。`token == nullptr` 表示"空键"（默认构造）。
    template<typename Tag>
    struct TypeKey {
        const void* token = nullptr;

        [[nodiscard]] constexpr auto valid() const noexcept -> bool {
            return token != nullptr;
        }

        friend constexpr auto operator==(TypeKey, TypeKey) noexcept -> bool = default;
    };

    using PageKey = TypeKey<PageTag>;
    using ParamsKey = TypeKey<ParamsTag>;
    using StoreKey = TypeKey<StoreTag>;

    /// 兼容别名。等价于 `PageKey` —— 页面身份是最常见的用法；新代码请直接写
    /// `PageKey` / `ParamsKey` / `StoreKey`，别再退回无类别的裸 `void*`。
    using NanTypeKey = PageKey;

    struct NoParams {};

    /// 取 `T` 的类型令牌：同一 `T` 在同一 `Tag` 下永远是同一个地址，不依赖 RTTI。
    ///
    /// `Tag` 参与函数签名，所以 `nan_type_key<Foo>()`（页面）与
    /// `nan_type_key<Foo, ParamsTag>()`（参数）拿到的是**不同地址**，即使 `Foo`
    /// 既是页面类型又是别人的参数类型也不会互相误判。
    template<typename T, typename Tag = PageTag>
    [[nodiscard]] auto nan_type_key() noexcept -> TypeKey<Tag> {
        static const int token = 0;
        return TypeKey<Tag> {&token};
    }

    /// 页面身份键：路由表按它匹配，与显示用的 `RouteOptions::key` 无关。
    template<typename PageT>
    [[nodiscard]] auto page_key() noexcept -> PageKey {
        return nan_type_key<PageT>();
    }

    /// 页面参数类型键：路由表据此校验"进这一页必须带哪种参数"。
    template<typename ParamsT>
    [[nodiscard]] auto params_key() noexcept -> ParamsKey {
        return nan_type_key<ParamsT, ParamsTag>();
    }

    /// Store 类型键：`set_store()` / `NanApplication::use_store()` 与查询侧共用的身份。
    template<typename StoreT>
    [[nodiscard]] auto store_key() noexcept -> StoreKey {
        return nan_type_key<StoreT, StoreTag>();
    }

    /// `Navigation::navigate_to()` 的失败原因。
    ///
    /// 与 `navigate<PageT>()` 返回 bool 的分工：模板版本的页面类型在**编译期**确定，
    /// 失败只可能是"忘了注册"这类编程错误；而 `navigate_to()` 的键来自运行时枚举
    /// （侧边栏、命令面板遍历路由表），失败是**预期内**的正常路径，返回值必须说清
    /// 为什么，否则消费方只能表现为"点了没反应"。
    enum class NavigationError : std::uint8_t {
        /// 句柄已失效：窗口已关闭或 Router 已销毁。
        unavailable,
        /// 传入的页面键为空（默认构造的 `PageKey`）。
        invalid_key,
        /// 路由表里没有这个页面类型。
        unknown_route,
        /// 该路由指向的页面需要参数，不能"点一下就进去"。
        requires_params,
    };

    [[nodiscard]] constexpr auto describe(const NavigationError error) noexcept -> std::string_view {
        switch (error) {
            case NavigationError::unavailable:
                return "navigation handle is no longer available";
            case NavigationError::invalid_key:
                return "page key is empty";
            case NavigationError::unknown_route:
                return "page type is not registered in Routes";
            case NavigationError::requires_params:
                return "route requires params; use navigate<PageT>(params) instead";
        }
        return "unknown navigation error";
    }

    template<typename ParamsT>
    class Page;

    namespace detail
    {
        class PageBase;

        struct NavigationState {
            std::move_only_function<bool(PageKey, std::unique_ptr<PageBase>)> submit;

            /// 类型擦除的"按页面键进入"：由 Router 安装，内部查表并调用条目上的
            /// `RouteEntry::activate`。放在这里而不是 `Navigation` 上，是因为只有
            /// Router 同时拥有路由表与生命周期——顺带让句柄在窗口关闭后能报出
            /// `Unavailable`，而不是去碰一个已经析构的路由表。
            std::move_only_function<std::expected<bool, NavigationError>(PageKey)> activate;
        };
    } // namespace detail

    class Navigation {
    public:
        Navigation() = default;

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, Page<ParamsT>>
        [[nodiscard]] auto navigate(ParamsT params) const -> bool {
            return submit<PageT>(std::make_unique<PageT>(std::move(params)));
        }

        template<typename PageT>
            requires std::derived_from<PageT, Page<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto navigate() const -> bool {
            return submit<PageT>(std::make_unique<PageT>());
        }

        /// 用运行时才拿到的页面键进入页面。`Routes::nav_entries()` 的消费方
        /// （侧边栏、命令面板、快捷键表）用它，就不必各自重写
        /// `if (key == …) navigate<PageT>()` 链。
        ///
        /// 返回 `true` 表示请求已被接受，与 `navigate<PageT>()` 同义：页面可能被
        /// 排到下一个 UI 任务里才真正切换。返回 `unexpected` 时请求**没有被接受**，
        /// 原因见 `describe(error)`。
        [[nodiscard]] auto navigate_to(const PageKey key) const
            -> std::expected<bool, NavigationError> {
            const auto state = state_.lock();
            if (!state || !state->activate) {
                return std::unexpected(NavigationError::unavailable);
            }
            return state->activate(key);
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
            return state->submit(page_key<PageT>(), std::move(page));
        }

        friend class NanRouter;
        std::weak_ptr<detail::NavigationState> state_;
    };

    class PageContext {
    public:
        PageContext(
            reactive::Graph& graph,
            reactive::ReactiveScope& scope,
            const theme::NanTheme& theme,
            NanStore* store,
            StoreKey store_key,
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
            if (store_ == nullptr || store_key_ != store_key<StoreT>()) {
                throw std::runtime_error(
                    "PageContext::store: requested store type is not installed"
                );
            }
            return static_cast<StoreT&>(*store_);
        }

    private:
        reactive::Graph* graph_;
        reactive::ReactiveScope* scope_;
        const theme::NanTheme* theme_;
        NanStore* store_;
        StoreKey store_key_ = {};
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

    namespace detail
    {
        class PageBase {
        public:
            virtual ~PageBase() = default;

            PageBase(const PageBase&) = delete;
            auto operator=(const PageBase&) -> PageBase& = delete;
            PageBase(PageBase&&) = delete;
            auto operator=(PageBase&&) -> PageBase& = delete;

            [[nodiscard]] virtual auto params_type_key() const -> ParamsKey = 0;
            [[nodiscard]] virtual auto build(PageContext& context) -> widget::View = 0;

        protected:
            PageBase() = default;
        };
    } // namespace detail

    /// Application-facing page base for the route model. A page receives the full
    /// app context exactly once; BuildContext remains a widget-layer value.
    template<typename ParamsT = NoParams>
    class Page: public detail::PageBase {
    public:
        using Params = ParamsT;

        Page()
            requires std::default_initializable<Params>
        = default;
        explicit Page(Params params): params_(std::move(params)) {}

        [[nodiscard]] auto params() const -> const Params& { return params_; }
        [[nodiscard]] auto params() -> Params& { return params_; }

        [[nodiscard]] auto params_type_key() const -> ParamsKey override {
            return params_key<Params>();
        }

    private:
        Params params_;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP
