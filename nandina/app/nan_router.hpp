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
#include "async_scope.hpp"
#include "nan_page.hpp"
#include "nan_store.hpp"
#include "router_outlet.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
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
        /// 供应用显示的静态地址文字，**不是路由身份**——身份是页面类型（`PageKey`）。
        /// 留空时回退到 `title`；两者都空也可以，只是没有可显示的文字。
        /// 第一版不提供可解析的 path/深链接，所以这里不做参数匹配。
        ///
        /// 刻意不叫 `key`：那个名字会和 `RouteEntry::page_key` 撞车，让人误以为它
        /// 参与路由匹配。这里唯一的作用就是给界面和日志一个可读的名字。
        std::string address;
        std::string title;
        std::string icon;
        bool show_in_nav = true;

        friend auto operator==(const RouteOptions&, const RouteOptions&) -> bool = default;
    };

    struct RouteEntry {
        PageKey page_key = {};
        ParamsKey params_key = {};
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

        /// 逐字段比较（含 `activate` 函数指针）。测试与"两份路由表是否等价"的判断
        /// 需要它；手写逐字段断言既啰嗦，又会在新增字段时静默漏掉。
        friend auto operator==(const RouteEntry&, const RouteEntry&) -> bool = default;
    };

    /// `Routes::validate()` 与 `NanRouter::configure()` 的失败原因。
    ///
    /// 单一 `bool` 撑不起可用的错误信息：「重复注册 HomePage」和「路由表是空的」
    /// 是两种完全不同的写法错误，调用方（`NanWindow::use_router`）需要据此给出
    /// 能定位问题的提示，而不是一句"invalid routes"。
    enum class RoutesErrorKind : std::uint8_t {
        /// 路由表为空。
        empty,
        /// 已经配置过路由表，或已有当前页面：路由表在启动前一次性确定。
        already_configured,
        /// 条目的类型键无效——通常是默认构造的 `RouteEntry`，没经过 `route<PageT>()`。
        invalid_entry,
        /// 同一页面类型注册了两次。
        duplicate_page,
        /// 有两个条目用了同一个非空的显示地址（`RouteOptions::address`）。
        duplicate_key,
    };

    struct RoutesError {
        RoutesErrorKind kind = RoutesErrorKind::empty;
        /// 出错条目的下标。`empty` / `already_configured` 时无意义。
        std::size_t index = 0;
        /// 与 `index` 冲突的前一条目下标；无冲突时为 0。
        std::size_t conflict = 0;
    };

    /// 人类可读的失败描述，含条目下标，可直接用作异常信息或日志。
    [[nodiscard]] auto describe(const RoutesError& error) -> std::string;

    /// 页面注册表：页面类型 → 导航元数据的唯一声明点。
    ///
    /// 路由**身份是页面类型**（`PageKey`），`RouteOptions::key` 只是给应用显示的
    /// 静态地址文字。表在声明期（`validate()` / `configure()`）一次性校验，
    /// 之后只读——`find()` 这类查询是 `noexcept` 的纯查表。
    class Routes {
    public:
        using const_iterator = std::vector<RouteEntry>::const_iterator;

        Routes() = default;
        Routes(std::initializer_list<RouteEntry> entries): entries_(entries) {}

        [[nodiscard]] auto entries() const noexcept -> const std::vector<RouteEntry>& {
            return entries_;
        }

        [[nodiscard]] auto begin() const noexcept -> const_iterator {
            return entries_.begin();
        }

        [[nodiscard]] auto end() const noexcept -> const_iterator {
            return entries_.end();
        }

        /// 查找路由表中指定页面类型键的条目；没有找到时返回 nullptr。
        ///
        /// 这里刻意保留「可空指针 + `noexcept`」：失败原因只有「未注册」一种，
        /// 为它套一层 `optional` 会引入第二种"空"（engaged 但值为 nullptr），
        /// 套 `expected` 则要为单值枚举付仪式感。需要表达"必须存在"的调用点用
        /// `at()`；需要表达"为什么没进去"的是 `Navigation::navigate_to()`。
        [[nodiscard]] auto find(const PageKey key) const noexcept -> const RouteEntry* {
            if (const auto iter = std::ranges::find(entries_, key, &RouteEntry::page_key);
                iter != entries_.end())
            {
                return &*iter;
            }
            return nullptr;
        }

        [[nodiscard]] auto contains(const PageKey key) const noexcept -> bool {
            return find(key) != nullptr;
        }

        [[nodiscard]] auto index_of(const PageKey key) const noexcept
            -> std::optional<std::size_t> {
            const auto iter = std::ranges::find(entries_, key, &RouteEntry::page_key);
            if (iter == entries_.end()) {
                return std::nullopt;
            }
            return static_cast<std::size_t>(iter - entries_.begin());
        }

        /// 按页面类型取条目；未注册时抛 `std::out_of_range`。
        /// 对齐 `std::map::at`：调用点已经确定"它一定在"，不该被 nullptr 检查打断。
        [[nodiscard]] auto at(const PageKey key) const -> const RouteEntry& {
            if (const auto* entry = find(key); entry != nullptr) {
                return *entry;
            }
            throw std::out_of_range("Routes::at: page type is not registered");
        }

        /// 按下标取条目；越界时抛 `std::out_of_range`（`operator[]` 仍然不检查）。
        [[nodiscard]] auto at(const std::size_t index) const -> const RouteEntry& {
            return entries_.at(index);
        }

        /// 侧边栏 / 命令面板 / 快捷键表要的只读视图：只含"点一下就能进去"的条目
        /// （`show_in_nav` 且页面可默认构造）。
        ///
        /// 两个条件的组合收在框架里一次，消费方就不必各自重复它、也就不会各自
        /// 漏掉其中一条（漏掉 `activate` 判断的界面会出现点了没反应的死条目）。
        ///
        /// 返回的指针指向本表内部，生存期以 `Routes` 为准。
        [[nodiscard]] auto nav_entries() const -> std::vector<const RouteEntry*> {
            std::vector<const RouteEntry*> visible;
            visible.reserve(entries_.size());
            for (const auto& entry: entries_) {
                if (entry.options.show_in_nav && entry.activate != nullptr) {
                    visible.push_back(&entry);
                }
            }
            return visible;
        }

        [[nodiscard]] auto length() const noexcept -> std::size_t {
            return entries_.size();
        }

        [[nodiscard]] auto empty() const noexcept -> bool {
            return entries_.empty();
        }

        [[nodiscard]] auto operator[](const std::size_t index) const -> const RouteEntry& {
            return entries_[index];
        }

        auto push_back(RouteEntry entry) -> void {
            entries_.push_back(std::move(entry));
        }

        auto push_back(const RouteEntry& entry) -> void {
            entries_.push_back(entry);
        }

        /// 表内一致性校验：非空、类型键有效、页面类型不重复、非空的显示地址不重复。
        /// 与 Router 状态无关，所以可以在交给 `configure()` 之前先自查。
        [[nodiscard]] auto validate() const -> std::expected<void, RoutesError>;

    private:
        std::vector<RouteEntry> entries_;
    };

    template<typename PageT>
        requires std::derived_from<PageT, Page<typename PageT::Params>>
    [[nodiscard]] auto route(RouteOptions options = {}) -> RouteEntry {
        if (options.address.empty()) {
            options.address = options.title;
        }
        return RouteEntry {
            .page_key = page_key<PageT>(),
            .params_key = params_key<typename PageT::Params>(),
            .options = std::move(options),
            // 注意这里是**立即调用的泛型 lambda**，而不是 `if constexpr` 写在函数体里：
            // 后者只会切换函数体，函数指针本身永远非空 —— 于是需要构造参数的页面会得到
            // 一个"永远返回 false 的激活入口"，在导航里表现为点了没反应的死条目。
            // 指针本身必须在编译期就是 nullptr，消费方才能据此把它排除。
            .activate = []() -> RouteEntry::ActivateFn {
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
            StoreKey store_key = {},
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
            StoreKey store_key = {},
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
        [[nodiscard]] auto outlet() -> std::shared_ptr<RouterOutlet> {
            return host_;
        }

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

        /// 当前页面的显示用地址文字；没有当前页面、或该路由没设 address/title 时为空。
        /// 与 `current_page_key()` 成对：那个是身份，这个是给人看的名字。
        [[nodiscard]] auto current_address() const -> std::string_view;

        /// 当前页面的类型键——这才是路由身份。没有当前页面时为空键。
        [[nodiscard]] auto current_page_key() const noexcept -> PageKey {
            return current_ ? current_->page_key : PageKey {};
        }

        /// 当前页面类型键的**响应式来源**：`apply_navigation()` 成功换页后更新它。
        ///
        /// 为什么由 Router 持有：窗口外壳（导航栏 / 侧边栏）在任何页面存在之前就建好了
        /// —— `set_shell()` 早于 `start()`。所以"当前是哪一页"必须能被观察，否则外壳
        /// 只能靠应用手动同步（首屏还必然漏掉一次），或者每帧轮询。
        [[nodiscard]] auto current_page() -> reactive::Signal<PageKey>& {
            return *current_page_;
        }

        /// 当前页面对应的路由条目；没有当前页面、或它不在路由表里时为空。
        /// 外壳拿它取标题 / 图标，不必自己 `find(current_page_key())`。
        [[nodiscard]] auto current_entry() const noexcept -> const RouteEntry* {
            return route_mode_ ? routes_.find(current_page_key()) : nullptr;
        }

        /// 当前页面是否就是 `key` —— 导航高亮的唯一判断依据。
        [[nodiscard]] auto is_current(const PageKey key) const noexcept -> bool {
            return current_page_key() == key;
        }

        /// 设置当前路由表并校验。路由表在配置后不可变。
        ///
        /// 返回失败原因而不是 `bool`：`use_router()` 要把「重复注册了哪个页面」
        /// 和「路由表为空」区分开才给得出有用的提示。
        [[nodiscard]] auto configure(Routes routes) -> std::expected<void, RoutesError>;
        [[nodiscard]] auto navigation() const -> Navigation;
        [[nodiscard]] auto routes() const noexcept -> const Routes& {
            return routes_;
        }
        [[nodiscard]] auto route(const PageKey page_key) const noexcept -> const RouteEntry* {
            return routes_.find(page_key);
        }

        template<typename PageT>
        [[nodiscard]] auto route() const noexcept -> const RouteEntry* {
            return route(page_key<PageT>());
        }
        [[nodiscard]] auto route_mode() const noexcept -> bool {
            return route_mode_;
        }

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
                page_key<PageT>(),
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
            return apply_navigation(page_key<PageT>(), std::make_unique<PageT>());
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        void set_store(StoreT& store) {
            store_ = &store;
            store_key_ = store_key<StoreT>();
        }

        void clear_store();

        /// Destroy the current page and detach its root. Used by NanWindow during
        /// shutdown; navigation never exposes history operations.
        void clear();

        /// 页面构建 / 切换期错误的去处。`NanWindow` 会把它接到 `on_error()`，
        /// 所以应用通常覆写窗口钩子；不继承窗口的场景与测试用这个注入点接管。
        ///
        /// 未安装处理器时**不静默**：Router 至少记录一条 error 日志。
        ///
        /// 第一个参数是出错路由的**显示地址**（`RouteOptions::address`），只用于
        /// 提示；页面身份是 `PageKey`，诊断信息里不要拿它当键用。
        using PageErrorHandler =
            std::function<void(std::string_view route_address, std::exception_ptr error)>;
        void set_page_error_handler(PageErrorHandler handler);

    private:
        /// 当前唯一页面。Router 只维护一个当前路由，所以这里是 optional 而不是栈；
        /// 页面身份（page_key）也只存在于此，避免出现第二个"当前"的说法。
        struct Frame {
            PageKey page_key = {};
            std::unique_ptr<detail::PageBase> page;
            std::shared_ptr<scene::NanNode2D> root;
            std::unique_ptr<reactive::ReactiveScope> scope;
            std::unique_ptr<AsyncScope> async_scope;
            std::string address;
        };

        /// 把构建期异常交给已安装的处理器；没有处理器时记 error 日志，绝不吞掉。
        void report_page_error(std::string_view route_address, std::exception_ptr error);

        /// 切换完成后把焦点交给新页面；旧页面持有焦点时先清空，避免焦点留在
        /// 已经拆除的节点上。换页被延迟时改在布局之后交接。
        void restore_focus_after_navigation();

        /// 按默认焦点规则把焦点送入给定页面根节点；焦点已在有效节点上则不动。
        void focus_first_in_page(scene::NanNode2D& root);

        /// 构建新页面但**不挂载**：挂载交给 `RouterOutlet` 的原子换页。
        /// 构建失败时抛出，并且不留下任何可见状态，调用方的旧页面保持完整。
        [[nodiscard]] auto build_frame(
            PageKey page_key,
            std::unique_ptr<detail::PageBase> page,
            std::string route_address
        ) -> Frame;

        /// 让页面回调失效、取消异步、解除焦点，但**不拆根**：拆根属于换页事务的一部分，
        /// 由 Outlet 在替换时一并完成（见 page_and_router.md §4 的退役顺序）。
        void retire_frame(Frame& frame);

        /// `Navigation::navigate_to()` 的实现：查表、判空、调用条目上的 activate。
        /// 放在 Router 侧是因为只有它同时拥有路由表与生命周期。
        [[nodiscard]] auto activate_route(PageKey page_key) const
            -> std::expected<bool, NavigationError>;

        [[nodiscard]] auto
        submit_navigation(PageKey page_key, std::unique_ptr<detail::PageBase> page) -> bool;
        void flush_pending_navigation();
        [[nodiscard]] auto
        apply_navigation(PageKey page_key, std::unique_ptr<detail::PageBase> page) -> bool;
        [[nodiscard]] auto post_ui_task(std::move_only_function<void()> task) -> bool;

        reactive::Graph* graph_;
        /// 由构造函数创建（graph_ 就绪之后），见 current_page()。
        std::unique_ptr<reactive::Signal<PageKey>> current_page_;
        const theme::NanTheme* theme_;
        std::unique_ptr<theme::ThemeManager> owned_theme_manager_;
        theme::ThemeManager* theme_manager_ = nullptr;
        NanStore* store_ = nullptr;
        StoreKey store_key_ = {};
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
            PageKey page_key = {};
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
