//
// app/nan_router — typed current-page router implementation.
//

#include "nan_router.hpp"

#include "../foundation/nan_logger.hpp"
#include "../scene/scene_tree.hpp"

namespace nandina::app
{
    NanRouter::NanRouter(
        reactive::Graph& graph,
        const theme::NanTheme& theme,
        NanStore* store,
        StoreKey store_key,
        resource::ResourceManager* resources,
        text::FontLoader* font_loader,
        text::FontFamilyRegistry* font_families,
        UiDispatcher* dispatcher,
        BackgroundExecutor* background_executor,
        scene::OverlayHost* overlay_host
    ):
        graph_(&graph),
        theme_(&theme),
        store_(store),
        store_key_(store_key),
        resources_(resources),
        font_loader_(font_loader),
        font_families_(font_families),
        dispatcher_(dispatcher),
        background_executor_(background_executor),
        overlay_host_(overlay_host),
        host_(std::make_shared<RouterOutlet>()) {
        // 当前路由的响应式来源：外壳可以在任何页面存在之前就绑定它。
        current_page_ = std::make_unique<reactive::Signal<PageKey>>(graph, PageKey {});

        // PageContext::ui() is available for routers built from a static theme
        // too. Keep an internal manager only for BuildContext's theme service;
        // application windows use their own manager through the other overload.
        owned_theme_manager_ = std::make_unique<theme::ThemeManager>();
        owned_theme_manager_->set_theme(theme);
        theme_manager_ = owned_theme_manager_.get();
        navigation_state_->submit = [this](
                                        PageKey page_key,
                                        std::unique_ptr<detail::PageBase> page
                                    ) {
            return submit_navigation(page_key, std::move(page));
        };
        // `Navigation::navigate_to()` 的类型擦除入口：只有 Router 同时拥有路由表
        // 与生命周期，所以查表 + 判空在 Router 侧完成。
        navigation_state_->activate = [this](const PageKey key) {
            return activate_route(key);
        };
    }

    NanRouter::~NanRouter() {
        if (navigation_state_) {
            navigation_state_->submit = {};
            navigation_state_->activate = {};
        }
        command_lifetime_.reset();
    }

    NanRouter::NanRouter(
        reactive::Graph& graph,
        theme::ThemeManager& theme_manager,
        NanStore* store,
        StoreKey store_key,
        resource::ResourceManager* resources,
        text::FontLoader* font_loader,
        text::FontFamilyRegistry* font_families,
        UiDispatcher* dispatcher,
        BackgroundExecutor* background_executor,
        scene::OverlayHost* overlay_host
    ):
        NanRouter(
            graph,
            theme_manager.theme(),
            store,
            store_key,
            resources,
            font_loader,
            font_families,
            dispatcher,
            background_executor,
            overlay_host
        ) {
        theme_manager_ = &theme_manager;
        owned_theme_manager_.reset();
        current_page_ = std::make_unique<reactive::Signal<PageKey>>(graph, PageKey {});
    }

    auto NanRouter::host() -> std::shared_ptr<scene::NanControl> {
        return host_;
    }

    auto NanRouter::graph() -> reactive::Graph& {
        return *graph_;
    }

    auto NanRouter::theme() const -> const theme::NanTheme& {
        return theme_manager_ != nullptr ? theme_manager_->theme() : *theme_;
    }

    auto NanRouter::store_base() -> NanStore* {
        return store_;
    }

    auto NanRouter::current_address() const -> std::string_view {
        return current_ ? std::string_view {current_->address} : std::string_view {};
    }

    auto describe(const RoutesError& error) -> std::string {
        switch (error.kind) {
            case RoutesErrorKind::empty:
                return "routes are empty";
            case RoutesErrorKind::already_configured:
                return "routes are already configured (or a page is already active)";
            case RoutesErrorKind::invalid_entry:
                return "route[" + std::to_string(error.index) + "] has an invalid page key";
            case RoutesErrorKind::duplicate_page:
                return "route[" + std::to_string(error.index)
                    + "] duplicates the page type of route[" + std::to_string(error.conflict) + "]";
            case RoutesErrorKind::duplicate_key:
                return "route[" + std::to_string(error.index)
                    + "] reuses the display key of route[" + std::to_string(error.conflict) + "]";
        }
        return "unknown routes error";
    }

    auto Routes::validate() const -> std::expected<void, RoutesError> {
        if (entries_.empty()) {
            return std::unexpected(RoutesError {.kind = RoutesErrorKind::empty});
        }
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            // 类型键只有在经过 `route<PageT>()` 时才有效；默认构造的条目说明有人
            // 手工拼了一个 RouteEntry 却忘了填身份。
            if (!entry.page_key.valid() || !entry.params_key.valid()) {
                return std::unexpected(RoutesError {
                    .kind = RoutesErrorKind::invalid_entry,
                    .index = i,
                });
            }
            for (std::size_t j = 0; j < i; ++j) {
                const auto& earlier = entries_[j];
                // 页面类型是路由身份，重复注册必然是笔误。
                if (earlier.page_key == entry.page_key) {
                    return std::unexpected(RoutesError {
                        .kind = RoutesErrorKind::duplicate_page,
                        .index = i,
                        .conflict = j,
                    });
                }
                // 显示用地址文字是可选元数据，只在其非空时要求唯一——否则两个
                // 都没设 address/title 的路由会互相冲突，`app::route<PageT>()` 这种
                // 最简写法就永远配不上了。
                if (!earlier.options.address.empty() && earlier.options.address == entry.options.address) {
                    return std::unexpected(RoutesError {
                        .kind = RoutesErrorKind::duplicate_key,
                        .index = i,
                        .conflict = j,
                    });
                }
            }
        }
        return {};
    }

    auto NanRouter::configure(Routes routes) -> std::expected<void, RoutesError> {
        if (route_mode_ || current_.has_value()) {
            return std::unexpected(RoutesError {.kind = RoutesErrorKind::already_configured});
        }
        if (const auto valid = routes.validate(); !valid) {
            return valid;
        }
        routes_ = std::move(routes);
        route_mode_ = true;
        return {};
    }

    auto NanRouter::activate_route(const PageKey page_key) const
        -> std::expected<bool, NavigationError> {
        if (!page_key.valid()) {
            return std::unexpected(NavigationError::invalid_key);
        }
        const auto* entry = route_mode_ ? routes_.find(page_key) : nullptr;
        if (entry == nullptr) {
            return std::unexpected(NavigationError::unknown_route);
        }
        // 需要构造参数的页面没有 activate thunk：它在导航里是个"死条目"，
        // 消费方应该据 requires_params 把它排除，而不是遇到"点了没反应"。
        if (entry->activate == nullptr) {
            return std::unexpected(NavigationError::requires_params);
        }
        if (!entry->activate(Navigation {navigation_state_})) {
            return std::unexpected(NavigationError::unavailable);
        }
        return true;
    }

    auto NanRouter::navigation() const -> Navigation {
        return Navigation {navigation_state_};
    }

    void NanRouter::clear_store() {
        store_ = nullptr;
        store_key_ = {};
    }

    void NanRouter::clear() {
        if (current_) {
            retire_frame(*current_);
        }
        if (host_ != nullptr) {
            host_->clear_page();
        }
        current_.reset();
        current_page_->set(PageKey {});
    }

    void NanRouter::set_page_error_handler(PageErrorHandler handler) {
        page_error_handler_ = std::move(handler);
    }

    void NanRouter::report_page_error(
        const std::string_view route_address,
        std::exception_ptr error
    ) {
        if (page_error_handler_) {
            page_error_handler_(route_address, error);
            return;
        }

        // 没有处理器也不能静默：至少留下一条 error 日志，否则页面"点了没反应"
        // 又找不到任何线索。
        std::string message = "unknown error";
        try {
            std::rethrow_exception(error);
        }
        catch (const std::exception& caught) {
            message = caught.what();
        }
        catch (...) {
            message = "non-standard exception";
        }
        log::error("NanRouter: page build failed for route '{}': {}", route_address, message);
    }

    auto NanRouter::build_frame(
        const PageKey page_key,
        std::unique_ptr<detail::PageBase> page,
        std::string route_address
    ) -> Frame {
        if (!page) {
            throw std::runtime_error("NanRouter::build_frame: page is null");
        }

        auto scope = std::make_unique<reactive::ReactiveScope>(*graph_);
        auto async_scope = dispatcher_ && background_executor_
            ? std::make_unique<AsyncScope>(*dispatcher_, *background_executor_)
            : nullptr;
        PageContext context {
            *graph_,
            *scope,
            theme(),
            store_,
            store_key_,
            resources_,
            font_loader_,
            font_families_,
            async_scope.get(),
            theme_manager_,
            dispatcher_,
            overlay_host_,
            drag_controller_,
            navigation()
        };
        auto root = page->build(context);
        if (!root) {
            throw std::runtime_error("NanRouter::build_frame: page build returned null root");
        }
        // 每次进入都必须构建全新的根节点。复用已经在树里的节点会让随后的换页抛错，
        // 而那时旧页面已经退役——正好是"失败却破坏了当前页面"的情形，所以在这里
        // 拦下来，此时还没有任何状态被改动。
        if (root->parent() != nullptr || root->is_inside_tree()) {
            throw std::runtime_error(
                "NanRouter::build_frame: page build returned an already-mounted root; "
                "each entry must build a fresh node"
            );
        }

        root->set_visible(true);
        return Frame {
            .page_key = page_key,
            .page = std::move(page),
            .root = std::move(root),
            .scope = std::move(scope),
            .async_scope = std::move(async_scope),
            .address = std::move(route_address),
        };
    }

    auto NanRouter::submit_navigation(
        const PageKey page_key,
        std::unique_ptr<detail::PageBase> page
    ) -> bool {
        const auto* entry = route_mode_ ? routes_.find(page_key) : nullptr;
        if (entry == nullptr || page == nullptr || entry->params_key != page->params_type_key()) {
            return false;
        }
        if (dispatcher_ == nullptr) {
            return apply_navigation(page_key, std::move(page));
        }

        bool post_task = false;
        {
            std::scoped_lock lock(pending_navigation_mutex_);
            // A navigation request is a value, so replacing the pending value
            // also releases the page instance that would never be entered.
            pending_navigation_ = PendingNavigation {
                .page_key = page_key,
                .page = std::move(page),
            };
            if (!navigation_task_posted_) {
                navigation_task_posted_ = true;
                post_task = true;
            }
        }
        if (!post_task) {
            return true;
        }

        if (post_ui_task([this] { flush_pending_navigation(); })) {
            return true;
        }

        // The dispatcher may be shutting down. Clear only the coalesced value
        // owned by this task; no scene mutation has been accepted.
        std::scoped_lock lock(pending_navigation_mutex_);
        pending_navigation_.reset();
        navigation_task_posted_ = false;
        return false;
    }

    void NanRouter::flush_pending_navigation() {
        std::optional<PendingNavigation> pending;
        {
            std::scoped_lock lock(pending_navigation_mutex_);
            pending = std::move(pending_navigation_);
            pending_navigation_.reset();
            navigation_task_posted_ = false;
        }
        if (pending) {
            (void)apply_navigation(pending->page_key, std::move(pending->page));
        }
    }

    auto NanRouter::apply_navigation(
        const PageKey page_key,
        std::unique_ptr<detail::PageBase> page
    ) -> bool {
        const auto* entry = route_mode_ ? routes_.find(page_key) : nullptr;
        if (entry == nullptr || page == nullptr || entry->params_key != page->params_type_key()) {
            return false;
        }
        try {
            // 先把所有可能抛出的事情做完（构建 + 换页前置条件），再退役旧页面。
            // 顺序反过来会出现"旧页面订阅已清、却仍挂在屏幕上"的中间态。
            auto next = build_frame(page_key, std::move(page), entry->options.address);
            if (host_->child_count() > 1) {
                throw std::logic_error("NanRouter: router outlet holds multiple pages");
            }

            if (current_) {
                retire_frame(*current_);
            }
            // 原子换页：旧页面先摘、新页面后挂，切换期不会出现两个子节点。
            host_->set_page(next.root);
            // 旧 Frame 在此销毁。它的根节点已被 set_page 摘除，所以到这里才真正析构。
            current_ = std::move(next);
            // 换页成功后才发布当前路由：观察者（侧边栏 / 导航栏）据此更新高亮。
            current_page_->set(page_key);
        }
        catch (...) {
            // 失败必须保持当前页面与当前路由不变。这里**不重新抛出**：带 dispatcher
            // 时调用点在 UiDispatcher::drain() 的任务里，抛出去会穿出主循环并终止进程。
            report_page_error(entry->options.address, std::current_exception());
            return false;
        }
        // 切换完成后把键盘焦点交给新页面；旧页面若持有焦点，此时它已经被摘除，
        // 焦点不能留在已销毁的节点上（见 page_and_router.md §4）。
        restore_focus_after_navigation();
        return true;
    }

    void NanRouter::restore_focus_after_navigation() {
        auto* tree = host_ != nullptr ? host_->get_tree() : nullptr;
        if (tree == nullptr || !current_ || !current_->root) {
            return;
        }
        const auto root = current_->root;
        // 换页可能在场景树遍历阶段被延迟，此刻新页面还没挂上。把交接推到布局之后，
        // 两种情况就都能落在真正挂载好的页面上。
        if (!root->is_inside_tree()) {
            tree->post_layout([this, lifetime = std::weak_ptr<void>(command_lifetime_), root] {
                if (lifetime.expired()) {
                    return; // Router 已经销毁。
                }
                // 期间又被导航替换过：这次交接已经过期，不能去动新页面的焦点。
                auto* current_tree = host_ != nullptr ? host_->get_tree() : nullptr;
                if (current_tree == nullptr || !root->is_inside_tree()) {
                    return;
                }
                focus_first_in_page(*root);
            });
            return;
        }
        focus_first_in_page(*root);
    }

    void NanRouter::focus_first_in_page(scene::NanNode2D& root) {
        auto* tree = host_ != nullptr ? host_->get_tree() : nullptr;
        if (tree == nullptr) {
            return;
        }
        // 焦点仍落在有效节点上（例如新页面内部已被主动聚焦）就不打扰它。
        if (auto* current = tree->focused_node(); current != nullptr && current->is_inside_tree()) {
            return;
        }
        // 新页面按默认焦点规则进入；没有可聚焦控件时至少清空焦点，而不是留一个
        // 已经脱离场景树的悬垂目标。
        if (!tree->focus_first_within(root)) {
            tree->set_focus(nullptr);
        }
    }

    void NanRouter::retire_frame(Frame& frame) {
        // 退役顺序按 page_and_router.md §4：先让旧页面的回调失效并解除它的响应式
        // 订阅与异步任务，最后才拆根。反过来（先 detach_root）会让 on_exit_tree 在
        // 页面订阅仍然生效时运行，回调可能去操作正在拆除的子树。
        //
        // ReactiveScope::clear() 本身就按"先失效 generation（guard_callbacks 的凭据）、
        // 再断开外部事件、最后释放 signal/computed/effect"执行，所以这里一步覆盖了
        // 合约里的"失效回调"与"解除响应式订阅"。
        if (frame.scope != nullptr) {
            frame.scope->clear();
        }
        if (frame.async_scope != nullptr) {
            frame.async_scope->clear();
        }
        // 页面拥有的浮层由页面内控件的 OverlayHandle 负责关闭，随后续的换页（摘根）
        // 与 Frame 析构一起发生。这里不额外遍历 OverlayHost：浮层挂在 overlay layer
        // 上，与页面节点之间没有可查询的从属边。
        // 焦点必须先于摘根解除，否则焦点会短暂指向一个不在树内的节点。
        if (auto* tree = host_ != nullptr ? host_->get_tree() : nullptr;
            tree != nullptr && frame.root && tree->focused_node() != nullptr
            && frame.root->is_ancestor_of(*tree->focused_node()))
        {
            tree->set_focus(nullptr);
        }
        // 这里**不摘根**：摘根由 RouterOutlet 换页时一并完成，这样"旧页面退役"
        // 与"新页面挂载"之间不存在两个子节点共存的窗口。
    }

    auto NanRouter::post_ui_task(std::move_only_function<void()> task) -> bool {
        if (dispatcher_ == nullptr) {
            return false;
        }
        auto lifetime = std::weak_ptr<void>(command_lifetime_);
        return dispatcher_->post([lifetime = std::move(lifetime),
                                  task = std::move(task)]() mutable {
            if (const auto alive = lifetime.lock()) {
                task();
            }
        });
    }

} // namespace nandina::app
