//
// app/nan_router — typed route registry and current-page navigation.
//
// Routes owns the immutable page registry; Navigation replaces one current page.
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
#define NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP

#include "../reactive/graph.hpp"
#include "../reactive/signal.hpp"
#include "../scene/control.hpp"
#include "../theme/theme_manager.hpp"
#include "nan_page.hpp"
#include "nan_store.hpp"
#include "async_scope.hpp"
#include "router_outlet.hpp"

#include <cstddef>
#include <exception>
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

    class NanRouter;

    struct RouteOptions {
        /// 供应用显示的静态地址文字，**不是路由身份**——身份是页面类型。
        /// 留空时回退到 `title`；两者都空也可以，只是没有可显示的文字。
        /// 第一版不提供可解析的 path/深链接，所以这里不做参数匹配。
        std::string key;
        std::string title;
        std::string icon;
        bool show_in_nav = true;
    };

    struct RouteEntry {
        NanTypeKey page_key = nullptr;
        NanTypeKey params_key = nullptr;
        RouteOptions options;

        /// 类型擦除的"进入这一页"：由 `route<PageT>()` 生成，因此路由表本身就把
        /// 「描述」与「怎么进去」绑在了一起。
        ///
        /// 为什么需要它：侧边栏、命令面板这类消费者是从 `Routes::entries()` **枚举**
        /// 出条目的，运行时只拿得到 `page_key`，拿不到编译期的 `PageT`。没有这个
        /// thunk，每个消费者都得重写一遍 `if (key == …) navigate<PageT>()` 链。
        ///
        /// 参数不可默认构造的页面为 `nullptr`：这类页面需要调用方提供参数，不适合
        /// "点一下就进去"（用 `options.show_in_nav = false` 把它从导航里隐掉）。
        using ActivateFn = bool (*)(const Navigation& navigation);
        ActivateFn activate = nullptr;
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
        requires std::derived_from<PageT, Page<typename PageT::Params>>
    [[nodiscard]] auto route(RouteOptions options = {}) -> RouteEntry {
        if (options.key.empty()) {
            options.key = options.title;
        }
        return RouteEntry {
            .page_key = nan_type_key<PageT>(),
            .params_key = nan_type_key<typename PageT::Params>(),
            .options = std::move(options),
            // 注意这里是**立即调用的泛型 lambda**，而不是 `if constexpr` 写在函数体里：
            // 后者只会切换函数体，函数指针本身永远非空 —— 于是需要构造参数的页面会得到
            // 一个"永远返回 false 的激活入口"，在导航里表现为点了没反应的死条目。
            // 指针本身必须在编译期就是 nullptr，消费方才能据此把它排除。
            .activate =
                []() -> RouteEntry::ActivateFn {
                if constexpr (std::default_initializable<PageT>) {
                    return [](const Navigation& navigation) -> bool {
                        return navigation.navigate<PageT>();
                    };
                }
                else {
                    return nullptr;
                }
            }(),
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
        [[nodiscard]] auto outlet() -> std::shared_ptr<RouterOutlet> { return host_; }

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

        /// 当前页面的显示用地址文字；没有当前页面或路由没设 key/title 时为空。
        [[nodiscard]] auto current_key() const -> std::string_view;

        /// 当前页面的类型键——这才是路由身份。没有当前页面时为 nullptr。
        [[nodiscard]] auto current_page_key() const noexcept -> NanTypeKey {
            return current_ ? current_->page_key : nullptr;
        }

        /// 当前页面类型键的**响应式来源**：`apply_navigation()` 成功换页后更新它。
        ///
        /// 为什么由 Router 持有：窗口外壳（导航栏 / 侧边栏）在任何页面存在之前就建好了
        /// —— `set_shell()` 早于 `start()`。所以"当前是哪一页"必须能被观察，否则外壳
        /// 只能靠应用手动同步（首屏还必然漏掉一次），或者每帧轮询。
        [[nodiscard]] auto current_page() -> reactive::Signal<NanTypeKey>& {
            return *current_page_;
        }

        /// 设置当前路由表。路由表在配置后不可变。
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

        /// 在已配置的路由表上启动初始页面。路由表由 `configure()` 或
        /// `NanWindow::use_router(Routes)` 提供，启动与声明是两步。
        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, Page<ParamsT>>
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
            requires std::derived_from<PageT, Page<typename PageT::Params>>
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

        /// Destroy the current page and detach its root. Used by NanWindow during
        /// shutdown; navigation never exposes history operations.
        void clear();

        /// 页面构建 / 切换期错误的去处。`NanWindow` 会把它接到 `on_error()`，
        /// 所以应用通常覆写窗口钩子；不继承窗口的场景与测试用这个注入点接管。
        ///
        /// 未安装处理器时**不静默**：Router 至少记录一条 error 日志。
        using PageErrorHandler =
            std::function<void(std::string_view route_key, std::exception_ptr error)>;
        void set_page_error_handler(PageErrorHandler handler);

    private:
        /// 当前唯一页面。Router 只维护一个当前路由，所以这里是 optional 而不是栈；
        /// 页面身份（page_key）也只存在于此，避免出现第二个"当前"的说法。
        struct Frame {
            NanTypeKey page_key = nullptr;
            std::unique_ptr<detail::PageBase> page;
            std::shared_ptr<scene::NanNode2D> root;
            std::unique_ptr<reactive::ReactiveScope> scope;
            std::unique_ptr<AsyncScope> async_scope;
            std::string key;
        };

        /// 把构建期异常交给已安装的处理器；没有处理器时记 error 日志，绝不吞掉。
        void report_page_error(std::string_view route_key, std::exception_ptr error);

        /// 切换完成后把焦点交给新页面；旧页面持有焦点时先清空，避免焦点留在
        /// 已经拆除的节点上。换页被延迟时改在布局之后交接。
        void restore_focus_after_navigation();

        /// 按默认焦点规则把焦点送入给定页面根节点；焦点已在有效节点上则不动。
        void focus_first_in_page(scene::NanNode2D& root);

        /// 构建新页面但**不挂载**：挂载交给 `RouterOutlet` 的原子换页。
        /// 构建失败时抛出，并且不留下任何可见状态，调用方的旧页面保持完整。
        [[nodiscard]] auto build_frame(
            NanTypeKey page_key,
            std::unique_ptr<detail::PageBase> page,
            std::string route_key
        ) -> Frame;

        /// 让页面回调失效、取消异步、解除焦点，但**不拆根**：拆根属于换页事务的一部分，
        /// 由 Outlet 在替换时一并完成（见 page_and_router.md §4 的退役顺序）。
        void retire_frame(Frame& frame);

        [[nodiscard]] auto submit_navigation(
            NanTypeKey page_key,
            std::unique_ptr<detail::PageBase> page
        )
            -> bool;
        void flush_pending_navigation();
        [[nodiscard]] auto apply_navigation(
            NanTypeKey page_key,
            std::unique_ptr<detail::PageBase> page
        )
            -> bool;
        [[nodiscard]] auto post_ui_task(std::move_only_function<void()> task) -> bool;

        reactive::Graph* graph_;
        /// 由构造函数创建（graph_ 就绪之后），见 current_page()。
        std::unique_ptr<reactive::Signal<NanTypeKey>> current_page_;
        const theme::NanTheme* theme_;
        std::unique_ptr<theme::ThemeManager> owned_theme_manager_;
        theme::ThemeManager* theme_manager_ = nullptr;
        NanStore* store_ = nullptr;
        NanTypeKey store_key_ = nullptr;
        resource::ResourceManager* resources_ = nullptr;
        text::FontLoader* font_loader_ = nullptr;
        text::FontFamilyRegistry* font_families_ = nullptr;
        UiDispatcher* dispatcher_ = nullptr;
        BackgroundExecutor* background_executor_ = nullptr;
        scene::OverlayHost* overlay_host_ = nullptr;
        widget::DragController* drag_controller_ = nullptr;
        std::shared_ptr<RouterOutlet> host_;
        Routes routes_;
        bool route_mode_ = false;
        std::shared_ptr<detail::NavigationState> navigation_state_ =
            std::make_shared<detail::NavigationState>();
        struct PendingNavigation {
            NanTypeKey page_key = nullptr;
            std::unique_ptr<detail::PageBase> page;
        };
        std::mutex pending_navigation_mutex_;
        std::optional<PendingNavigation> pending_navigation_;
        bool navigation_task_posted_ = false;
        std::optional<Frame> current_;
        PageErrorHandler page_error_handler_;
        std::shared_ptr<void> command_lifetime_ = std::make_shared<int>(0);
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
