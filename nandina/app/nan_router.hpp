//
// app/nan_router — typed route registry and current-page navigation.
//
// The single-current-route API is implemented alongside the legacy stack surface
// while callers and tests migrate. New code configures Routes and uses Navigation;
// stack methods remain only for the in-progress alpha migration.
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
#define NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP

#include "../reactive/graph.hpp"
#include "../scene/control.hpp"
#include "../theme/theme_manager.hpp"
#include "nan_page.hpp"
#include "nan_store.hpp"
#include "async_scope.hpp"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::app
{

    class PageFrame;
    class NanPage;
    class NanRouter;

    struct RouteOptions {
        std::string key;
        std::string title;
        std::string icon;
        bool show_in_nav = true;
    };

    struct RouteEntry {
        NanTypeKey page_key = nullptr;
        NanTypeKey params_key = nullptr;
        RouteOptions options;
    };

    class Routes {
    public:
        Routes() = default;
        Routes(std::initializer_list<RouteEntry> entries): entries_(entries) {}

        [[nodiscard]] auto entries() const noexcept -> const std::vector<RouteEntry>& {
            return entries_;
        }

        [[nodiscard]] auto find(NanTypeKey key) const noexcept -> const RouteEntry* {
            for (const auto& entry: entries_) {
                if (entry.page_key == key) {
                    return &entry;
                }
            }
            return nullptr;
        }

    private:
        std::vector<RouteEntry> entries_;
    };

    template<typename PageT>
        requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
    [[nodiscard]] auto route(RouteOptions options = {}) -> RouteEntry {
        if (options.key.empty()) {
            options.key = options.title;
        }
        return RouteEntry {
            .page_key = nan_type_key<PageT>(),
            .params_key = nan_type_key<typename PageT::Params>(),
            .options = std::move(options),
        };
    }

    class NanRouter {
    public:
        explicit NanRouter(
            reactive::Graph& graph,
            const theme::NanTheme& theme,
            NanStore* store = nullptr,
            NanTypeKey store_key = nullptr,
            resource::ResourceManager* resources = nullptr,
            text::FontLoader* font_loader = nullptr,
            text::FontFamilyRegistry* font_families = nullptr,
            UiDispatcher* dispatcher = nullptr,
            BackgroundExecutor* background_executor = nullptr,
            scene::OverlayHost* overlay_host = nullptr
        );
        explicit NanRouter(
            reactive::Graph& graph,
            theme::ThemeManager& theme_manager,
            NanStore* store = nullptr,
            NanTypeKey store_key = nullptr,
            resource::ResourceManager* resources = nullptr,
            text::FontLoader* font_loader = nullptr,
            text::FontFamilyRegistry* font_families = nullptr,
            UiDispatcher* dispatcher = nullptr,
            BackgroundExecutor* background_executor = nullptr,
            scene::OverlayHost* overlay_host = nullptr
        );
        ~NanRouter();

        NanRouter(const NanRouter&) = delete;
        auto operator=(const NanRouter&) -> NanRouter& = delete;
        NanRouter(NanRouter&&) = delete;
        auto operator=(NanRouter&&) -> NanRouter& = delete;

        [[nodiscard]] auto host() -> std::shared_ptr<scene::NanControl>;

        /// 窗口级拖拽服务。由 NanWindow 在构造 Router 后立刻注入，页面经
        /// PageContext 取用（避免依赖"内容已挂载"这种时序）。
        void set_drag_controller(widget::DragController* controller) noexcept {
            drag_controller_ = controller;
        }
        [[nodiscard]] auto drag_controller() const noexcept -> widget::DragController* {
            return drag_controller_;
        }
        [[nodiscard]] auto graph() -> reactive::Graph&;
        [[nodiscard]] auto theme() const -> const theme::NanTheme&;
        [[nodiscard]] auto store_base() -> NanStore*;
        [[nodiscard]] auto depth() const -> std::size_t;
        [[nodiscard]] auto empty() const -> bool;
        [[nodiscard]] auto current_key() const -> std::string_view;
        [[nodiscard]] auto current_page_key() const noexcept -> NanTypeKey {
            return current_page_key_;
        }
        [[nodiscard]] auto can_pop() const -> bool;

        /// Configure the single-current-route model. Routes are immutable after
        /// configuration; the legacy stack API remains available until migration
        /// of the application and tests is complete.
        [[nodiscard]] auto configure(Routes routes) -> bool;
        [[nodiscard]] auto navigation() const -> Navigation;
        [[nodiscard]] auto routes() const noexcept -> const Routes& { return routes_; }
        [[nodiscard]] auto route(NanTypeKey page_key) const noexcept -> const RouteEntry* {
            return routes_.find(page_key);
        }

        template<typename PageT>
        [[nodiscard]] auto route() const noexcept -> const RouteEntry* {
            return route(nan_type_key<PageT>());
        }
        [[nodiscard]] auto route_mode() const noexcept -> bool { return route_mode_; }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        [[nodiscard]] auto start(Routes routes, ParamsT params) -> bool {
            if (!configure(std::move(routes))) {
                return false;
            }
            return apply_navigation(nan_type_key<PageT>(), std::make_unique<PageT>(std::move(params)));
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto start(Routes routes) -> bool {
            if (!configure(std::move(routes))) {
                return false;
            }
            return apply_navigation(nan_type_key<PageT>(), std::make_unique<PageT>());
        }

        /// Start the explicitly configured route table at its initial page.
        /// This overload keeps route declaration and startup selection separate,
        /// which is useful for NanWindow::use_router(Routes).
        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
            && std::constructible_from<PageT, ParamsT>
        [[nodiscard]] auto start(ParamsT params) -> bool {
            if (!route_mode_) {
                return false;
            }
            return apply_navigation(
                nan_type_key<PageT>(),
                std::make_unique<PageT>(std::move(params))
            );
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto start() -> bool {
            if (!route_mode_) {
                return false;
            }
            return apply_navigation(nan_type_key<PageT>(), std::make_unique<PageT>());
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        void set_store(StoreT& store) {
            store_ = &store;
            store_key_ = nan_type_key<StoreT>();
        }

        void clear_store();

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        auto push(ParamsT params) -> PageT& {
            auto page = std::make_unique<PageT>(std::move(params));
            auto* raw = page.get();
            push_page(std::move(page));
            return *raw;
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        [[nodiscard]] auto request_push(ParamsT params) -> bool {
            return post_command([this, params = std::move(params)]() mutable {
                (void)push<PageT>(std::move(params));
            });
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto push() -> PageT& {
            auto page = std::make_unique<PageT>();
            auto* raw = page.get();
            push_page(std::move(page));
            return *raw;
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto request_push() -> bool {
            return post_command([this] { (void)push<PageT>(); });
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        auto replace(ParamsT params) -> PageT& {
            remove_top();
            return push<PageT>(std::move(params));
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        [[nodiscard]] auto request_replace(ParamsT params) -> bool {
            return post_command([this, params = std::move(params)]() mutable {
                (void)replace<PageT>(std::move(params));
            });
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto replace() -> PageT& {
            remove_top();
            return push<PageT>();
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto request_replace() -> bool {
            return post_command([this] { (void)replace<PageT>(); });
        }

        auto pop() -> bool;
        auto pop_to(std::string_view route_key) -> bool;
        void clear();

        [[nodiscard]] auto request_pop() -> bool;
        [[nodiscard]] auto request_pop_to(std::string route_key) -> bool;
        [[nodiscard]] auto request_clear() -> bool;

        /// 页面转场（默认关闭=即时切换）。开启后 push 淡入、pop/replace 淡出，且淡出
        /// 完成前保留被替换页面的生命周期（scope/async），随后才销毁。
        void set_transition_enabled(bool enabled);
        [[nodiscard]] auto transition_enabled() const -> bool;
        void set_transition_duration(float seconds);

    private:
        using NodePtr = std::shared_ptr<scene::NanNode>;

        struct Frame {
            std::unique_ptr<NanPage> page;
            std::shared_ptr<scene::NanNode2D> root;
            /// 转场包装节点（开启转场时非空，承载每页的淡入淡出 opacity）。
            std::shared_ptr<PageFrame> frame;
            std::unique_ptr<reactive::ReactiveScope> scope;
            std::unique_ptr<AsyncScope> async_scope;
            std::string key;
            bool active = false;
        };

        void push_page(std::unique_ptr<NanPage> page, std::string route_key = {});
        [[nodiscard]] auto submit_navigation(NanTypeKey page_key, std::unique_ptr<NanPage> page)
            -> bool;
        void flush_pending_navigation();
        [[nodiscard]] auto apply_navigation(NanTypeKey page_key, std::unique_ptr<NanPage> page)
            -> bool;
        void sync_visibility();
        void attach_root(const std::shared_ptr<scene::NanNode2D>& root);
        void detach_root(const std::shared_ptr<scene::NanNode2D>& root);
        void drop_frame(Frame& frame);
        /// 移除栈顶：开启转场时淡出并延迟 drop，否则即时 drop。
        void remove_top();
        [[nodiscard]] auto frame_node(const Frame& frame) const -> std::shared_ptr<scene::NanNode2D>;
        void fade_frame(Frame& frame, float target);
        void drop_completed_exits();
        [[nodiscard]] auto post_command(std::move_only_function<void()> command) -> bool;
        [[nodiscard]] auto make_context_for(Frame& frame) -> PageContext;

        reactive::Graph* graph_;
        const theme::NanTheme* theme_;
        std::unique_ptr<theme::ThemeManager> owned_theme_manager_;
        theme::ThemeManager* theme_manager_ = nullptr;
        NanStore* store_ = nullptr;
        NanTypeKey store_key_ = nullptr;
        NanTypeKey current_page_key_ = nullptr;
        resource::ResourceManager* resources_ = nullptr;
        text::FontLoader* font_loader_ = nullptr;
        text::FontFamilyRegistry* font_families_ = nullptr;
        UiDispatcher* dispatcher_ = nullptr;
        BackgroundExecutor* background_executor_ = nullptr;
        scene::OverlayHost* overlay_host_ = nullptr;
        widget::DragController* drag_controller_ = nullptr;
        std::shared_ptr<scene::NanControl> host_;
        Routes routes_;
        bool route_mode_ = false;
        std::shared_ptr<detail::NavigationState> navigation_state_ =
            std::make_shared<detail::NavigationState>();
        struct PendingNavigation {
            NanTypeKey page_key = nullptr;
            std::unique_ptr<NanPage> page;
        };
        std::mutex pending_navigation_mutex_;
        std::optional<PendingNavigation> pending_navigation_;
        bool navigation_task_posted_ = false;
        std::vector<Frame> frames_;
        /// 淡出中的页面：生命周期（scope/async）保留，淡出完成后销毁。
        std::vector<Frame> exiting_;
        bool transition_enabled_ = false;
        float transition_duration_ = 0.2F;
        std::shared_ptr<void> command_lifetime_ = std::make_shared<int>(0);
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
