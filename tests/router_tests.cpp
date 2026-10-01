//
// Router + Store tests.
//

#include <nandina/app/nan_router.hpp>
#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/app/nan_store.hpp>
#include <nandina/foundation/geometry.hpp>
#include <nandina/reactive/effect.hpp>
#include <nandina/reactive/signal.hpp>
#include <nandina/scene/control.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/theme/theme.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/scene/overlay_host.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <concepts>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace
{
    using namespace nandina;

    // PageContext 只暴露弱 `Navigation` 句柄：它是可安全按值捕获的导航入口。
    // 不得再退回"把 NanRouter& 交给页面"的旧形态——那既是封装泄漏（页面可以
    // 调 clear()/start() 重置路由），也让构建期 Context 的悬垂捕获重新变得可能。
    // 见 docs/references/page_and_router.md 第 8 节。
    //
    // 断言必须写成受 Context 参数化的 concept：非模板里的 requires 表达式对
    // 非法成员是硬错误，而不是"不满足"。
    template<typename Context>
    concept ExposesRouter = requires(Context& context) { context.router(); };

    static_assert(!ExposesRouter<app::PageContext>);
    static_assert(requires(app::PageContext& context) { context.navigation(); });

    // 证明这个 concept 真的能识别出成员，否则上面的断言可能只是"永远为真"。
    struct RouterProbe {
        auto router() -> int;
    };
    static_assert(ExposesRouter<RouterProbe>);

    struct TestStore final: app::NanStore {
        explicit TestStore(reactive::Graph& graph): count(graph, 0) {}
        reactive::Signal<int> count;
    };

    struct HomeParams {
        int user_id = 0;
    };

    struct DetailParams {
        int blog_id = 0;
    };

    class HomePage final: public app::Page<HomeParams> {
    public:
        explicit HomePage(HomeParams params): Page(params) {}

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(320, 200));
            root->set_name("home-root");
            root->set_position(foundation::NanPoint(static_cast<float>(params().user_id), 0.0F));
            context.store<TestStore>().count.set(1);
            return root;
        }
    };

    class DetailPage final: public app::Page<DetailParams> {
    public:
        explicit DetailPage(DetailParams params): Page(params) {}

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200, 100));
            root->set_name("detail-root");
            context.store<TestStore>().count.set(params().blog_id);
            return root;
        }
    };

    class PlainPage final: public app::Page<> {
    public:
        PlainPage() = default;

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(100, 50));
            root->set_background(context.theme().palette.primary);
            return root;
        }
    };

    class SecondPlainPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext&) -> widget::View override {
            return std::make_shared<scene::NanControl>(foundation::NanSize(120, 60));
        }
    };

    struct RouteProbeParams {
        int* builds = nullptr;
        bool* fail = nullptr;
        bool* destroyed = nullptr;
        int* destruction_count = nullptr;
    };

    class RouteProbePage final: public app::Page<RouteProbeParams> {
    public:
        explicit RouteProbePage(RouteProbeParams params): Page(params) {}

        ~RouteProbePage() override {
            if (params().destroyed != nullptr) {
                *params().destroyed = true;
            }
            if (params().destruction_count != nullptr) {
                ++*params().destruction_count;
            }
        }

        [[nodiscard]] auto build(app::PageContext&) -> widget::View override {
            ++*params().builds;
            if (params().fail != nullptr && *params().fail) {
                throw std::runtime_error("probe page build failed");
            }
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }

    };

    struct DispatcherProbeParams {
        bool* available = nullptr;
        app::UiDispatcher** dispatcher = nullptr;
    };

    class DispatcherProbePage final: public app::Page<DispatcherProbeParams> {
    public:
        explicit DispatcherProbePage(DispatcherProbeParams params): Page(params) {}

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            *params().available = context.has_dispatcher();
            *params().dispatcher = &context.dispatcher();
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }
    };

    struct ScopedObserverLog {
        int observed = 0;
        int runs = 0;
    };

    struct ScopedObserverParams {
        ScopedObserverLog* log = nullptr;
    };

    class ScopedObserverPage final: public app::Page<ScopedObserverParams> {
    public:
        explicit ScopedObserverPage(ScopedObserverParams params): Page(params) {}

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            auto& store = context.store<TestStore>();
            auto* log = params().log;
            context.scope().effect([&store, log] {
                log->observed = store.count.get();
                ++log->runs;
            });
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }
    };

    struct ScopedEventParams {
        const reactive::Event<int>* event = nullptr;
        int* observed = nullptr;
    };

    class ScopedEventPage final: public app::Page<ScopedEventParams> {
    public:
        explicit ScopedEventPage(ScopedEventParams params): Page(params) {}

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            context.ui().connect(*params().event, [observed = params().observed](const int value) {
                *observed += value;
            });
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }
    };

    struct RetainedCallbackParams {
        std::shared_ptr<widget::Button>* root = nullptr;
        int* calls = nullptr;
        bool* destroyed = nullptr;
    };

    class RetainedCallbackPage final: public app::Page<RetainedCallbackParams> {
    public:
        explicit RetainedCallbackPage(RetainedCallbackParams params): Page(std::move(params)) {}

        ~RetainedCallbackPage() override {
            *params().destroyed = true;
        }

        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override {
            auto ui = context.ui();
            auto button = ui.make<widget::Button>("Retained root").on_click([this] {
                ++*params().calls;
            });
            auto root = button.build();
            *params().root = root;
            return root;
        }
    };

    struct AsyncPageParams {
        std::atomic_bool* started = nullptr;
        std::atomic_bool* cancelled = nullptr;
        bool* completed = nullptr;
    };

    class AsyncPage final: public app::Page<AsyncPageParams> {
    public:
        explicit AsyncPage(AsyncPageParams params): Page(params) {}

        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            const auto params = this->params();
            context.async_scope().run(
                [params](app::CancellationToken token) {
                    params.started->store(true, std::memory_order_release);
                    while (!token.stop_requested()) {
                        std::this_thread::yield();
                    }
                    params.cancelled->store(true, std::memory_order_release);
                },
                [params](std::expected<void, std::exception_ptr>) { *params.completed = true; }
            );
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }
    };

    /// Records what a page sees of the window-installed overlay portal.
    struct OverlayProbe {
        bool context_has = false;
        bool ui_has = false;
        scene::OverlayHost* context_host = nullptr;
        scene::OverlayHost* ui_host = nullptr;
    };

    OverlayProbe g_overlay_probe;

    class OverlayProbePage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context)
            -> widget::View override {
            g_overlay_probe.context_has = context.has_overlay_host();
            if (g_overlay_probe.context_has) {
                g_overlay_probe.context_host = &context.overlay_host();
            }
            auto ui = context.ui();
            g_overlay_probe.ui_has = ui.has_overlay_host();
            if (g_overlay_probe.ui_has) {
                g_overlay_probe.ui_host = &ui.overlay_host();
            }
            return std::make_shared<scene::NanControl>(foundation::NanSize(10.0F, 10.0F));
        }
    };

    /// 覆写窗口错误钩子，记录失败次数与消息。
    class ErrorProbeWindow final: public app::NanWindow {
    public:
        using NanWindow::NanWindow;

        int errors = 0;
        std::string last_message;
    protected:
        void on_error(std::exception_ptr error) override {
            ++errors;
            try {
                std::rethrow_exception(error);
            }
            catch (const std::exception& caught) {
                last_message = caught.what();
            }
            catch (...) {
                last_message = "non-standard exception";
            }
        }
    };

    /// 含可聚焦控件的页面，用来观察切换后的焦点落点。
    class FocusablePage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override {
            auto ui = context.ui();
            return ui.column()
                .gap(4.0F)
                .children(
                    ui.make<widget::Button>("First"),
                    ui.make<widget::Button>("Second")
                )
                .build();
        }
    };

    /// 在自己的构建作用域里 present 一个浮层，并把 handle 存到页面对象上——
    /// 与 Dialog / Tooltip / Select 持有 portal handle 的方式一致。
    class OverlayHoldingPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override {
            auto ui = context.ui();
            handle_ = std::make_unique<scene::OverlayHandle>(
                context.overlay_host().present(
                    ui.make<widget::Label>("page overlay").build(),
                    scene::OverlayOptions {.level = scene::OverlayLevel::popup}
                )
            );
            return std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 20.0F));
        }

    private:
        std::unique_ptr<scene::OverlayHandle> handle_;
    };

    [[nodiscard]] auto is_within(const scene::NanNode& scope, const scene::NanNode* node) -> bool {
        if (node == nullptr) {
            return false;
        }
        return node == &scope || scope.is_ancestor_of(*node);
    }
} // namespace

TEST_CASE("configured router navigates between registered typed pages", "[app][router][navigate]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    router.set_store(store);

    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>(app::RouteOptions {.address = "home", .title = "Home"}),
        app::route<DetailPage>(app::RouteOptions {.address = "detail", .title = "Detail"}),
    }));
    const auto navigation = router.navigation();
    REQUIRE(navigation.valid());
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 4}));
    REQUIRE(router.route_mode());
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.current_address() == "home");
    REQUIRE(router.current_page_key() == app::nan_type_key<HomePage>());

    auto* first_root = router.host()->get_child(0);
    REQUIRE(first_root != nullptr);
    REQUIRE(navigation.navigate<DetailPage>(DetailParams {.blog_id = 8}));
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.current_address() == "detail");
    REQUIRE(router.current_page_key() == app::nan_type_key<DetailPage>());
    REQUIRE(router.host()->get_child(0) != first_root);
    REQUIRE_FALSE(navigation.navigate<PlainPage>());
}

TEST_CASE("route configuration validates page types and display keys", "[app][router][configure]") {
    reactive::Graph graph;

    // key/title 是可选显示元数据，不是路由身份：最简写法 route<PageT>() 必须能配上，
    // 否则"类型就是路由身份"就名不副实。
    app::NanRouter keyless_router {graph, theme::default_theme()};
    REQUIRE(keyless_router.configure(app::Routes {
        app::route<PlainPage>(),
        app::route<SecondPlainPage>(),
    }));
    REQUIRE(keyless_router.route_mode());
    REQUIRE(keyless_router.route<PlainPage>() != nullptr);
    REQUIRE(keyless_router.route<PlainPage>()->options.address.empty());

    // 无 key 的路由照样能启动与导航，只是没有可显示的地址文字。
    REQUIRE(keyless_router.start<PlainPage>());
    REQUIRE(keyless_router.current_page_key() == app::nan_type_key<PlainPage>());
    REQUIRE(keyless_router.current_address().empty());

    // 页面类型是路由身份：同一类型注册两次必然是笔误。
    app::NanRouter duplicate_page_router {graph, theme::default_theme()};
    REQUIRE_FALSE(duplicate_page_router.configure(app::Routes {
        app::route<PlainPage>({.address = "a"}),
        app::route<PlainPage>({.address = "b"}),
    }));
    REQUIRE_FALSE(duplicate_page_router.route_mode());

    // 设了 key 就必须唯一，否则按 key 展示/高亮的界面会出现歧义。
    app::NanRouter duplicate_key_router {graph, theme::default_theme()};
    REQUIRE_FALSE(duplicate_key_router.configure(app::Routes {
        app::route<PlainPage>({.address = "same"}),
        app::route<SecondPlainPage>({.address = "same"}),
    }));
    REQUIRE_FALSE(duplicate_key_router.route_mode());
}

TEST_CASE("router outlet replaces its mounted page without changing outlet identity", "[app][router][outlet]") {
    auto outlet = std::make_shared<app::RouterOutlet>();
    auto first = std::make_shared<scene::NanControl>(foundation::NanSize(20, 20));
    auto second = std::make_shared<scene::NanControl>(foundation::NanSize(30, 30));

    REQUIRE(&outlet->set_page(first) == first.get());
    REQUIRE(outlet->page() == first.get());
    REQUIRE(outlet->child_count() == 1);

    REQUIRE(&outlet->set_page(second) == second.get());
    REQUIRE(outlet->page() == second.get());
    REQUIRE(outlet->child_count() == 1);

    outlet->clear_page();
    REQUIRE(outlet->page() == nullptr);
    REQUIRE(outlet->child_count() == 0);
}

TEST_CASE("router outlet swaps pages without exposing two children", "[app][router][outlet]") {
    scene::NanSceneTree tree;
    auto outlet = std::make_shared<app::RouterOutlet>();
    tree.set_root(outlet);

    auto first = std::make_shared<scene::NanControl>(foundation::NanSize(20, 20));
    auto second = std::make_shared<scene::NanControl>(foundation::NanSize(30, 30));

    (void)outlet->set_page(first);
    REQUIRE(outlet->page() == first.get());
    REQUIRE(outlet->child_count() == 1);
    REQUIRE(outlet->pending_page() == nullptr);

    (void)outlet->set_page(second);
    // 换页之后仍然只有一个子节点：旧页面先摘、新页面后挂。
    REQUIRE(outlet->child_count() == 1);
    REQUIRE(outlet->page() == second.get());
    REQUIRE_FALSE(first->is_inside_tree());
    REQUIRE(second->is_inside_tree());
}

TEST_CASE(
    "router outlet coalesces repeated swaps inside one traversal phase",
    "[app][router][outlet]"
) {
    scene::NanSceneTree tree;
    auto outlet = std::make_shared<app::RouterOutlet>();
    tree.set_root(outlet);

    auto first = std::make_shared<scene::NanControl>(foundation::NanSize(20, 20));
    auto second = std::make_shared<scene::NanControl>(foundation::NanSize(30, 30));
    auto third = std::make_shared<scene::NanControl>(foundation::NanSize(40, 40));
    (void)outlet->set_page(first);
    REQUIRE(outlet->child_count() == 1);

    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        // 遍历期间 remove_child 会直接抛，所以换页必须排到 flush。
        outlet->set_page(second);
        outlet->set_page(third);
        REQUIRE(outlet->child_count() == 1);
        REQUIRE(outlet->page() == first.get());
        REQUIRE(outlet->pending_page() == third);
    }

    tree.flush_tree_mutations();
    // 两次换页合并成最后一次：若各排一个替换，第二个会用陈旧的 current 摘错节点，
    // 在 Outlet 里留下一个没有任何引用的僵尸页面。
    REQUIRE(outlet->child_count() == 1);
    REQUIRE(outlet->page() == third.get());
    REQUIRE_FALSE(second->is_inside_tree());
    REQUIRE(second.use_count() == 1);
}

TEST_CASE(
    "navigating twice inside one traversal phase leaves exactly one page",
    "[app][router][navigate]"
) {
    reactive::Graph graph;
    scene::NanSceneTree tree;
    // 不装 dispatcher：导航就地生效，于是"遍历期间换页"这条路径会被真正走到。
    app::NanRouter router {graph, theme::default_theme()};
    tree.set_root(router.outlet());
    REQUIRE(router.configure(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<SecondPlainPage>({.address = "second"}),
    }));

    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(router.outlet()->child_count() == 1);

    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        REQUIRE(navigation.navigate<SecondPlainPage>());
        REQUIRE(navigation.navigate<PlainPage>());
        REQUIRE(router.outlet()->child_count() == 1);
    }

    tree.flush_tree_mutations();
    REQUIRE(router.current_address() == "plain");
    REQUIRE(router.outlet()->child_count() == 1);
    REQUIRE(router.outlet()->page() != nullptr);
}

TEST_CASE("focus reaches a page whose swap was deferred", "[app][router][focus]") {
    reactive::Graph graph;
    scene::NanSceneTree tree;
    app::NanRouter router {graph, theme::default_theme()};
    tree.set_root(router.outlet());
    REQUIRE(router.configure(app::Routes {
        app::route<FocusablePage>({.address = "focusable"}),
    }));

    const auto navigation = router.navigation();
    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        REQUIRE(navigation.navigate<FocusablePage>());
        // 换页被延迟到 flush，此刻页面还没挂上。
        REQUIRE(router.outlet()->page() == nullptr);
    }

    tree.flush_tree_mutations();
    tree.flush_post_layout_actions();
    auto* root = router.outlet()->page();
    REQUIRE(root != nullptr);
    // 焦点交接被推到布局之后，所以延迟换页也拿得到焦点。
    REQUIRE(tree.focused_node() != nullptr);
    REQUIRE(is_within(*root, tree.focused_node()));
}

TEST_CASE("window shell keeps the router outlet stable", "[app][router][shell]") {
    app::NanApplication application;
    app::NanWindow window {application, {}};
    auto& router = window.use_router(app::Routes {
        app::route<PlainPage>({.address = "plain", .title = "Plain"}),
        app::route<SecondPlainPage>({.address = "second", .title = "Second"}),
    });
    app::RouterOutlet* observed_outlet = nullptr;
    window.set_shell([&](app::ShellContext& context) -> widget::View {
        observed_outlet = context.outlet().get();
        return context.ui().center().child(context.outlet()).build();
    });

    REQUIRE(observed_outlet != nullptr);
    REQUIRE(router.start<PlainPage>());
    REQUIRE(window.scene_tree().layout_root(foundation::NanSize(720, 640)) > 0);
    INFO("shell size=" << window.overlay_host().content()->width() << "x"
                        << window.overlay_host().content()->height());
    INFO("outlet size=" << observed_outlet->width() << "x" << observed_outlet->height());
    REQUIRE(observed_outlet->width() > 600.0F);
    REQUIRE(observed_outlet->height() == Catch::Approx(640.0F));
    REQUIRE(observed_outlet->page() != nullptr);
    REQUIRE(window.overlay_host().content() != nullptr);
    REQUIRE(window.overlay_host().content()->child_count() == 1);

    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<SecondPlainPage>());
    REQUIRE(observed_outlet->page() != nullptr);
    REQUIRE(observed_outlet == router.outlet().get());
    REQUIRE(router.outlet()->page() != nullptr);
}

TEST_CASE("window routes page build failures to its on_error hook", "[app][router][error][shell]") {
    app::NanApplication application;
    int builds = 0;
    bool fail = false;
    const RouteProbeParams params {.builds = &builds, .fail = &fail};
    // 同前：窗口（及其 Router）声明在探针状态之后，保证它先析构。
    ErrorProbeWindow window {application, {}};
    auto& router = window.use_router(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<RouteProbePage>({.address = "probe"}),
    });

    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(application.dispatcher().drain() == 1);
    auto* stable_root = router.host()->get_child(0);
    REQUIRE(stable_root != nullptr);

    // 生产路径：导航在任务阶段提交，构建失败必须交给窗口的 on_error，
    // 而不是穿出 drain() 终止进程。
    fail = true;
    REQUIRE(navigation.navigate<RouteProbePage>(params));
    REQUIRE(application.dispatcher().drain() == 1);
    REQUIRE(window.errors == 1);
    REQUIRE(window.last_message == "probe page build failed");
    REQUIRE(router.current_address() == "plain");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(router.host()->get_child(0) == stable_root);
}

TEST_CASE("navigation moves keyboard focus into the new page", "[app][router][focus]") {
    app::NanApplication application;
    app::NanWindow window {application, {}};
    auto& router = window.use_router(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<FocusablePage>({.address = "focusable"}),
    });
    auto& tree = window.scene_tree();
    const auto navigation = router.navigation();

    REQUIRE(navigation.navigate<FocusablePage>());
    REQUIRE(application.dispatcher().drain() == 1);
    auto* focusable_root = router.host()->get_child(0);
    REQUIRE(focusable_root != nullptr);

    // 新页面按默认焦点规则拿到焦点，而不是留在原地或落空。
    REQUIRE(tree.focused_node() != nullptr);
    REQUIRE(is_within(*focusable_root, tree.focused_node()));

    // 切到没有可聚焦控件的页面：焦点必须清空，不能指向已经拆除的旧节点。
    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(application.dispatcher().drain() == 1);
    REQUIRE(router.host()->get_child(0) != focusable_root);
    REQUIRE(tree.focused_node() == nullptr);
}

TEST_CASE("navigating away closes the overlays a page presented", "[app][router][overlay]") {
    app::NanApplication application;
    app::NanWindow window {application, {}};
    auto& router = window.use_router(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<OverlayHoldingPage>({.address = "overlay"}),
    });
    const auto navigation = router.navigation();

    REQUIRE(navigation.navigate<OverlayHoldingPage>());
    REQUIRE(application.dispatcher().drain() == 1);
    REQUIRE(window.overlay_host().overlay_count() == 1);

    // 页面退役时必须带走它自己的浮层；否则切页后会留下一个没有归属的面板。
    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(application.dispatcher().drain() == 1);
    REQUIRE(window.overlay_host().overlay_count() == 0);
}

TEST_CASE("navigation requests are deferred when a UI dispatcher is installed", "[app][router][navigate]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        theme,
        nullptr,
        app::StoreKey {},
        nullptr,
        nullptr,
        nullptr,
        &dispatcher,
    };
    router.set_store(store);
    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>(app::RouteOptions {.address = "home"}),
        app::route<DetailPage>(app::RouteOptions {.address = "detail"}),
    }));
    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    REQUIRE_FALSE(router.current_page_key().valid());
    REQUIRE(dispatcher.pending_count() == 1);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.current_address() == "home");

    REQUIRE(navigation.navigate<DetailPage>(DetailParams {.blog_id = 2}));
    REQUIRE(router.current_address() == "home");
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.current_address() == "detail");
}

TEST_CASE(
    "navigation coalesces requests queued in one UI task phase",
    "[app][router][navigate]"
) {
    reactive::Graph graph;
    app::UiDispatcher dispatcher;
    TestStore store {graph};
    app::NanRouter router {
        graph,
        theme::default_theme(),
        &store,
        app::store_key<TestStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher,
    };
    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>({.address = "home"}),
        app::route<DetailPage>({.address = "detail"}),
    }));
    const auto navigation = router.navigation();

    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    REQUIRE(navigation.navigate<DetailPage>(DetailParams {.blog_id = 2}));
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 3}));
    REQUIRE(dispatcher.pending_count() == 1);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.current_address() == "home");
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.host()->child_count() == 1);
}

TEST_CASE(
    "route navigation rebuilds the current page and preserves it on build failure",
    "[app][router][navigate]"
) {
    reactive::Graph graph;
    int builds = 0;
    bool fail = false;
    bool destroyed = false;
    int destruction_count = 0;
    // router 必须声明在探针状态**之后**：作用域结束时它先析构，页面析构会写回
    // destroyed / destruction_count；顺序反过来就是 stack-use-after-scope（ASan 会报）。
    app::NanRouter router {graph, theme::default_theme()};
    REQUIRE(router.configure(app::Routes {
        app::route<RouteProbePage>({.address = "probe"}),
    }));
    const auto navigation = router.navigation();
    const RouteProbeParams params {
        .builds = &builds,
        .fail = &fail,
        .destroyed = &destroyed,
        .destruction_count = &destruction_count,
    };

    REQUIRE(navigation.navigate<RouteProbePage>(params));
    auto* first_root = router.host()->get_child(0);
    REQUIRE(builds == 1);
    REQUIRE(first_root != nullptr);

    REQUIRE(navigation.navigate<RouteProbePage>(params));
    REQUIRE(builds == 2);
    REQUIRE(router.host()->get_child(0) != first_root);
    REQUIRE(destroyed);
    REQUIRE(destruction_count == 1);
    auto* stable_root = router.host()->get_child(0);

    destroyed = false;
    fail = true;

    // 构建失败不再抛异常：带 dispatcher 时调用点在 UiDispatcher::drain() 的任务里，
    // 抛出去会穿出主循环终止进程。改为返回失败 + 把错误交给处理器，并保持当前
    // 页面与当前路由不变。
    std::string reported_route;
    std::exception_ptr reported_error;
    router.set_page_error_handler(
        [&reported_route, &reported_error](std::string_view key, std::exception_ptr error) {
            reported_route = std::string(key);
            reported_error = error;
        }
    );

    REQUIRE_FALSE(navigation.navigate<RouteProbePage>(params));
    REQUIRE(reported_error != nullptr);
    REQUIRE(reported_route == "probe");
    REQUIRE(router.current_address() == "probe");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(router.host()->get_child(0) == stable_root);
    REQUIRE(destroyed);
    REQUIRE(destruction_count == 2);

    // 没有处理器时同样不抛，只记 error 日志——错误绝不被吞掉也不该升级成崩溃。
    reported_error = nullptr;
    router.set_page_error_handler({});
    destroyed = false;
    REQUIRE_FALSE(navigation.navigate<RouteProbePage>(params));
    REQUIRE(reported_error == nullptr);
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(router.host()->get_child(0) == stable_root);
}

TEST_CASE(
    "a failed page build in the task phase is reported instead of escaping drain",
    "[app][router][navigate][error]"
) {
    reactive::Graph graph;
    app::UiDispatcher dispatcher;
    int builds = 0;
    bool fail = false;
    bool destroyed = false;
    int destruction_count = 0;
    // 同前：router 声明在探针状态之后，保证它先析构。
    app::NanRouter router {
        graph,
        theme::default_theme(),
        nullptr,
        app::StoreKey {},
        nullptr,
        nullptr,
        nullptr,
        &dispatcher,
    };
    REQUIRE(router.configure(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<RouteProbePage>({.address = "probe"}),
    }));

    std::string reported_route;
    std::exception_ptr reported_error;
    router.set_page_error_handler(
        [&reported_route, &reported_error](std::string_view key, std::exception_ptr error) {
            reported_route = std::string(key);
            reported_error = error;
        }
    );

    const auto navigation = router.navigation();
    const RouteProbeParams params {
        .builds = &builds,
        .fail = &fail,
        .destroyed = &destroyed,
        .destruction_count = &destruction_count,
    };

    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(dispatcher.drain() == 1);
    auto* stable_root = router.host()->get_child(0);
    REQUIRE(stable_root != nullptr);

    // 生产路径：导航在任务阶段提交，构建失败必须就地被接住。
    fail = true;
    REQUIRE(navigation.navigate<RouteProbePage>(params));
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(reported_error != nullptr);
    REQUIRE(reported_route == "probe");
    REQUIRE(router.current_address() == "plain");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(router.host()->get_child(0) == stable_root);
}

TEST_CASE("navigation handles expire after their router is destroyed", "[app][router][navigate]") {
    reactive::Graph graph;
    app::Navigation navigation;
    {
        app::NanRouter router {graph, theme::default_theme()};
        REQUIRE(router.configure(app::Routes {app::route<PlainPage>({.address = "plain"})}));
        navigation = router.navigation();
        REQUIRE(navigation.valid());
    }
    REQUIRE_FALSE(navigation.valid());
    REQUIRE_FALSE(navigation.navigate<PlainPage>());
}

TEST_CASE("router exposes its UI dispatcher through page context", "[app][router][dispatcher]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    app::NanRouter router {graph, theme, nullptr, app::StoreKey {}, nullptr, nullptr, nullptr,
                           &dispatcher};
    bool available = false;
    app::UiDispatcher* observed = nullptr;

    REQUIRE(router.configure(app::Routes {
        app::route<DispatcherProbePage>({.address = "dispatcher-probe"}),
    }));
    REQUIRE(router.navigation().navigate<DispatcherProbePage>(DispatcherProbeParams {
        .available = &available,
        .dispatcher = &observed,
    }));
    REQUIRE(dispatcher.drain() == 1);

    REQUIRE(available);
    REQUIRE(observed == &dispatcher);
}

TEST_CASE("navigation requests defer route replacement to the UI task phase", "[app][router][navigate]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    TestStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        theme,
        &store,
        app::store_key<TestStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>({.address = "home"}),
        app::route<DetailPage>({.address = "detail"}),
        app::route<PlainPage>({.address = "plain"}),
    }));
    const auto navigation = router.navigation();

    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    REQUIRE(navigation.navigate<DetailPage>(DetailParams {.blog_id = 7}));
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 2}));
    REQUIRE_FALSE(router.current_page_key().valid());
    REQUIRE(dispatcher.pending_count() == 1);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.current_address() == "home");
    REQUIRE(router.current_page_key() == app::nan_type_key<HomePage>());
    REQUIRE(store.count.peek() == 1);

    // A request made after the first task phase is queued for the next one.
    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(router.current_address() == "home");
    REQUIRE(dispatcher.pending_count() == 1);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.current_address() == "plain");
}

TEST_CASE("queued navigation expires with its router", "[app][router][navigate]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    app::Navigation navigation;
    {
        app::NanRouter
            router {graph, theme, nullptr, app::StoreKey {}, nullptr, nullptr, nullptr, &dispatcher};
        REQUIRE(router.configure(app::Routes {
            app::route<PlainPage>({.address = "plain"}),
        }));
        navigation = router.navigation();
        REQUIRE(navigation.navigate<PlainPage>());
        REQUIRE(navigation.valid());
    }

    REQUIRE(dispatcher.pending_count() == 1);
    std::size_t drained = 0;
    REQUIRE_NOTHROW(drained = dispatcher.drain());
    REQUIRE(drained == 1);
    REQUIRE_FALSE(navigation.valid());
    REQUIRE_FALSE(navigation.navigate<PlainPage>());
}

TEST_CASE("router frame cancellation suppresses page async completion", "[app][router][async]") {
    using namespace std::chrono_literals;

    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    app::BackgroundExecutor executor {1};
    app::NanRouter
        router {graph, theme, nullptr, app::StoreKey {}, nullptr, nullptr, nullptr, &dispatcher,
                &executor};
    std::atomic_bool started = false;
    std::atomic_bool cancelled = false;
    bool completed = false;

    REQUIRE(router.configure(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<AsyncPage>({.address = "async"}),
    }));
    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(navigation.navigate<AsyncPage>(AsyncPageParams {
        .started = &started,
        .cancelled = &cancelled,
        .completed = &completed,
    }));
    REQUIRE(dispatcher.drain() == 1);
    const auto start_deadline = std::chrono::steady_clock::now() + 2s;
    while (!started.load(std::memory_order_acquire)
           && std::chrono::steady_clock::now() < start_deadline)
    {
        std::this_thread::sleep_for(1ms);
    }
    REQUIRE(started.load(std::memory_order_acquire));

    REQUIRE(navigation.navigate<PlainPage>());
    REQUIRE(dispatcher.drain() == 1);
    const auto cancel_deadline = std::chrono::steady_clock::now() + 2s;
    while (!cancelled.load(std::memory_order_acquire)
           && std::chrono::steady_clock::now() < cancel_deadline)
    {
        std::this_thread::sleep_for(1ms);
    }
    REQUIRE(cancelled.load(std::memory_order_acquire));
    (void)dispatcher.drain();
    REQUIRE_FALSE(completed);
}

TEST_CASE("typed navigation keeps one current page and destroys the previous page", "[app][router][navigate]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme, &store, app::store_key<TestStore>()};
    int destruction_count = 0;
    int probe_builds = 0;
    bool destroyed = false;
    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>({.address = "home"}),
        app::route<DetailPage>({.address = "detail"}),
        app::route<RouteProbePage>({.address = "probe"}),
    }));
    const auto navigation = router.navigation();

    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    auto* first_root = router.host()->get_child(0);
    REQUIRE(first_root != nullptr);
    REQUIRE(router.outlet()->page() != nullptr);

    REQUIRE(navigation.navigate<RouteProbePage>(RouteProbeParams {
        .builds = &probe_builds,
        .destroyed = &destroyed,
        .destruction_count = &destruction_count,
    }));
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.current_address() == "probe");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(router.host()->get_child(0) != first_root);
    REQUIRE(destroyed == false);

    REQUIRE(navigation.navigate<DetailPage>(DetailParams {.blog_id = 2}));
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.current_address() == "detail");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(destruction_count == 1);
    REQUIRE(destroyed);
}

TEST_CASE("store updates propagate while typed routes are active", "[app][router][store]") {
    reactive::Graph graph;
    TestStore store {graph};
    int observed = 0;

    {
        reactive::EffectScope scope {graph};
        scope.add([&] { observed = store.count.get(); });

        const auto theme = theme::default_theme();
        app::NanRouter router {graph, theme, &store, app::store_key<TestStore>()};
        REQUIRE(router.configure(app::Routes {
            app::route<HomePage>({.address = "home"}),
            app::route<DetailPage>({.address = "detail"}),
        }));
        const auto navigation = router.navigation();
        REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
        REQUIRE(observed == 1);

        REQUIRE(navigation.navigate<DetailPage>(DetailParams {.blog_id = 9}));
        REQUIRE(observed == 9);

        store.count.set(12);
        REQUIRE(observed == 12);
    }
}

TEST_CASE(
    "router supports no-params routes and passes theme through context",
    "[app][router][theme]"
) {
    reactive::Graph graph;
    auto app_theme = theme::default_theme();
    app_theme.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    app::NanRouter router {graph, app_theme};

    REQUIRE(router.configure(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
    }));
    REQUIRE(router.navigation().navigate<PlainPage>());
    REQUIRE(router.outlet()->page() != nullptr);
    REQUIRE(router.current_address() == "plain");

    auto* root = router.host()->get_child(0)->as_node2d();
    REQUIRE(root != nullptr);
    auto* control = static_cast<scene::NanControl*>(root);
    REQUIRE(control->background().has_value());
    REQUIRE(control->background()->alpha() == app_theme.palette.primary.alpha());
}

TEST_CASE("router reads the active ThemeManager theme for new pages", "[app][router][theme]") {
    reactive::Graph graph;
    theme::ThemeManager manager;
    auto first = theme::default_theme();
    first.palette.primary = theme::nan_color(0.41F, 0.12F, 120.0F);
    auto second = theme::default_theme();
    second.palette.primary = theme::nan_color(0.73F, 0.12F, 120.0F);
    REQUIRE(manager.register_theme("first", first));
    REQUIRE(manager.register_theme("second", second));
    REQUIRE(manager.activate("first"));

    app::NanRouter router {graph, manager};
    REQUIRE(router.theme().palette.primary.oklch().light == Catch::Approx(0.41F));
    REQUIRE(manager.activate("second"));
    REQUIRE(router.theme().palette.primary.oklch().light == Catch::Approx(0.73F));
}

TEST_CASE("router clears page reactive scope when a page is replaced", "[app][router][scope]") {
    reactive::Graph graph;
    TestStore store {graph};
    ScopedObserverLog log;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme, &store, app::store_key<TestStore>()};

    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>({.address = "home"}),
        app::route<ScopedObserverPage>({.address = "scoped-observer"}),
        app::route<PlainPage>({.address = "plain"}),
    }));
    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    REQUIRE(navigation.navigate<ScopedObserverPage>(ScopedObserverParams {.log = &log}));

    REQUIRE(log.observed == 1);
    REQUIRE(log.runs == 1);

    store.count.set(5);
    REQUIRE(log.observed == 5);
    REQUIRE(log.runs == 2);

    REQUIRE(navigation.navigate<PlainPage>());
    store.count.set(9);
    REQUIRE(log.observed == 5);
    REQUIRE(log.runs == 2);
}

TEST_CASE(
    "router disconnects page event subscriptions when a page is replaced",
    "[app][router][scope]"
) {
    reactive::Graph graph;
    reactive::Event<int> event;
    TestStore store {graph};
    int observed = 0;
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes, &store, app::store_key<TestStore>()};

    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>({.address = "home"}),
        app::route<ScopedEventPage>({.address = "scoped-event"}),
    }));
    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    REQUIRE(navigation.navigate<ScopedEventPage>(
        ScopedEventParams {.event = &event, .observed = &observed}
    ));
    REQUIRE(event.subscriber_count() == 1);

    event.emit(2);
    REQUIRE(observed == 2);
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 2}));
    REQUIRE(event.subscriber_count() == 0);

    event.emit(3);
    REQUIRE(observed == 2);
}

TEST_CASE("retained page roots cannot invoke callbacks after pop", "[app][router][lifecycle]") {
    reactive::Graph graph;
    TestStore store {graph};
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes, &store, app::store_key<TestStore>()};
    std::shared_ptr<widget::Button> retained;
    int calls = 0;
    bool destroyed = false;

    REQUIRE(router.configure(app::Routes {
        app::route<HomePage>({.address = "home"}),
        app::route<RetainedCallbackPage>({.address = "retained-callback"}),
    }));
    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 1}));
    REQUIRE(navigation.navigate<RetainedCallbackPage>(RetainedCallbackParams {
        .root = &retained,
        .calls = &calls,
        .destroyed = &destroyed,
    }));

    scene::KeyEvent active_click {257, scene::KeyEvent::Action::press};
    REQUIRE(retained->on_input(active_click));
    REQUIRE(calls == 1);

    REQUIRE(navigation.navigate<HomePage>(HomeParams {.user_id = 2}));
    REQUIRE(destroyed);
    scene::KeyEvent stale_click {257, scene::KeyEvent::Action::press};
    REQUIRE(retained->on_input(stale_click));
    REQUIRE(calls == 1);
}

TEST_CASE("router forwards the window overlay portal into page build contexts", "[app][router][overlay]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    g_overlay_probe = {};

    app::NanRouter router {
        graph,
        themes,
        nullptr,
        app::StoreKey {},
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        host.get()
    };
    REQUIRE(router.configure(app::Routes {
        app::route<OverlayProbePage>({.address = "overlay-probe"}),
    }));
    REQUIRE(router.navigation().navigate<OverlayProbePage>());

    REQUIRE(g_overlay_probe.context_has);
    REQUIRE(g_overlay_probe.context_host == host.get());
    REQUIRE(g_overlay_probe.ui_has);
    REQUIRE(g_overlay_probe.ui_host == host.get());
}

TEST_CASE("a route entry carries a type-erased activation", "[app][router][route]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};

    const auto routes = app::Routes {
        app::route<PlainPage>(
            app::RouteOptions {.address = "plain", .title = "Plain", .icon = "plain-icon"}
        ),
        app::route<SecondPlainPage>(app::RouteOptions {.address = "second", .title = "Second"}),
    };
    REQUIRE(router.configure(routes));

    // 描述字段原样保留：侧边栏把它们当数据用。
    const auto* plain = router.route<PlainPage>();
    REQUIRE(plain != nullptr);
    REQUIRE(plain->options.title == "Plain");
    REQUIRE(plain->options.icon == "plain-icon");
    REQUIRE(plain->options.show_in_nav);

    // 类型擦除的跳转：消费方只拿得到 page_key（枚举 Routes::entries() 的结果）时
    // 也能进页面，不必重写 if 链。
    REQUIRE(plain->activate != nullptr);
    const auto navigation = router.navigation();
    REQUIRE(plain->activate(navigation));
    REQUIRE(router.current_page_key() == app::nan_type_key<PlainPage>());
    REQUIRE(router.current_address() == "plain");

    const auto* second = router.route<SecondPlainPage>();
    REQUIRE(second != nullptr);
    REQUIRE(second->activate != nullptr);
    REQUIRE(second->activate(navigation));
    REQUIRE(router.current_page_key() == app::nan_type_key<SecondPlainPage>());
    REQUIRE(router.current_address() == "second");

    // 需要构造参数的页面没有类型擦除入口：凭类型键进不去，所以是 nullptr。
    // 导航条 / 侧边栏据此把它排除，而不是给出一个点了没反应的死条目。
    const auto with_params = app::route<HomePage>(app::RouteOptions {.address = "home"});
    REQUIRE(with_params.activate == nullptr);
}

TEST_CASE("type keys are tagged per category", "[app][router][key]") {
    // 类别不同就是不同类型：把参数键或 Store 键交给路由查询会在编译期失败，
    // 这正是旧的裸 `const void*` 做不到的。
    static_assert(!std::same_as<app::PageKey, app::ParamsKey>);
    static_assert(!std::same_as<app::PageKey, app::StoreKey>);
    static_assert(!std::same_as<app::ParamsKey, app::StoreKey>);
    static_assert(!std::convertible_to<app::ParamsKey, app::PageKey>);
    static_assert(!std::convertible_to<app::StoreKey, app::PageKey>);
    static_assert(std::same_as<app::NanTypeKey, app::PageKey>);

    // 即使 T 相同，类别不同也会分配到不同的令牌地址，所以二者不会互相匹配。
    REQUIRE(app::page_key<PlainPage>().token != app::params_key<PlainPage>().token);
    REQUIRE(app::page_key<PlainPage>().token != app::store_key<PlainPage>().token);
    REQUIRE(app::params_key<PlainPage>().token != app::store_key<PlainPage>().token);

    // 同类别 + 同类型必须稳定：这是"页面类型就是路由身份"的基础。
    REQUIRE(app::page_key<PlainPage>() == app::page_key<PlainPage>());
    REQUIRE(app::nan_type_key<PlainPage>() == app::page_key<PlainPage>());

    // 默认构造即空键，与任何真实键都不相等；`valid()` 是唯一的判空方式。
    const app::PageKey empty {};
    REQUIRE_FALSE(empty.valid());
    REQUIRE(empty != app::page_key<PlainPage>());
    REQUIRE(app::route<PlainPage>().page_key.valid());
    REQUIRE(app::route<PlainPage>().params_key.valid());
}

TEST_CASE("routes expose read-only lookups and a navigation view", "[app][router][routes]") {
    const auto routes = app::Routes {
        app::route<PlainPage>({.address = "plain", .title = "Plain", .icon = "plain-icon"}),
        app::route<SecondPlainPage>({.address = "second", .title = "Second"}),
        // 显式从导航里隐藏：不想在侧边栏出现，但仍可被导航到。
        app::route<RouteProbePage>({.address = "probe", .show_in_nav = false}),
        // 需要构造参数的页面没有 activate —— 即便 show_in_nav 为真也进不去。
        app::route<HomePage>({.address = "home"}),
    };

    REQUIRE(routes.length() == 4);
    REQUIRE_FALSE(routes.empty());
    REQUIRE(routes.contains(app::page_key<PlainPage>()));
    REQUIRE_FALSE(routes.contains(app::page_key<FocusablePage>()));

    REQUIRE(routes.index_of(app::page_key<PlainPage>()) == 0);
    REQUIRE(routes.index_of(app::page_key<HomePage>()) == 3);
    REQUIRE_FALSE(routes.index_of(app::page_key<FocusablePage>()).has_value());

    REQUIRE(routes.at(app::page_key<PlainPage>()).options.title == "Plain");
    REQUIRE(routes.at(1).options.address == "second");
    // `at()` 表达"它一定在"：不在/越界是异常，不是 nullptr。
    REQUIRE_THROWS_AS(routes.at(app::page_key<FocusablePage>()), std::out_of_range);
    REQUIRE_THROWS_AS(routes.at(9), std::out_of_range);

    // 范围 for 直接用，不必先 entries()。
    std::size_t counted = 0;
    for (const auto& entry: routes) {
        REQUIRE(entry.page_key.valid());
        ++counted;
    }
    REQUIRE(counted == 4);

    // nav_entries() 一次收掉"可点击"的两个条件：显示在导航里 && 有 activate。
    const auto nav = routes.nav_entries();
    REQUIRE(nav.size() == 2);
    REQUIRE(nav[0]->page_key == app::page_key<PlainPage>());
    REQUIRE(nav[1]->page_key == app::page_key<SecondPlainPage>());
}

TEST_CASE("route validation reports why a table is rejected", "[app][router][configure]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();

    // 空表。
    {
        app::NanRouter router {graph, theme};
        const auto configured = router.configure(app::Routes {});
        REQUIRE_FALSE(configured);
        REQUIRE(configured.error().kind == app::RoutesErrorKind::empty);
        REQUIRE(app::describe(configured.error()) == "routes are empty");
        REQUIRE_FALSE(router.route_mode());
    }

    // 重复页面类型：连冲突的两个下标一起报出来，调用方才知道改哪一行。
    {
        app::NanRouter router {graph, theme};
        const auto configured = router.configure(app::Routes {
            app::route<PlainPage>({.address = "a"}),
            app::route<PlainPage>({.address = "b"}),
        });
        REQUIRE_FALSE(configured);
        REQUIRE(configured.error().kind == app::RoutesErrorKind::duplicate_page);
        REQUIRE(configured.error().index == 1);
        REQUIRE(configured.error().conflict == 0);
    }

    // 重复显示 key：只在两条都非空时才算冲突。
    {
        app::NanRouter router {graph, theme};
        const auto configured = router.configure(app::Routes {
            app::route<PlainPage>({.address = "same"}),
            app::route<SecondPlainPage>({.address = "same"}),
        });
        REQUIRE_FALSE(configured);
        REQUIRE(configured.error().kind == app::RoutesErrorKind::duplicate_key);
        REQUIRE(app::describe(configured.error()).find("route[1]") != std::string::npos);
    }

    // 已经有当前页面时不允许换表：路由表在启动前一次性确定。
    {
        app::NanRouter router {graph, theme};
        REQUIRE(router.configure(app::Routes {app::route<PlainPage>({.address = "plain"})}));
        REQUIRE(router.start<PlainPage>());
        const auto again = router.configure(app::Routes {
            app::route<PlainPage>({.address = "again"}),
        });
        REQUIRE_FALSE(again);
        REQUIRE(again.error().kind == app::RoutesErrorKind::already_configured);
    }

    // 手工拼的 RouteEntry 没有身份：明确报 invalid_entry，而不是悄悄匹配不上。
    {
        app::NanRouter router {graph, theme};
        const auto configured = router.configure(app::Routes {app::RouteEntry {}});
        REQUIRE_FALSE(configured);
        REQUIRE(configured.error().kind == app::RoutesErrorKind::invalid_entry);
        REQUIRE(app::describe(configured.error()).find("route[0]") != std::string::npos);
    }

    // validate() 与 Router 状态无关，可以先自查再交给 configure()。
    REQUIRE(app::Routes {}.validate().error().kind == app::RoutesErrorKind::empty);
}

TEST_CASE("navigation can enter a page by runtime page key", "[app][router][navigate]") {
    reactive::Graph graph;
    app::NanRouter router {graph, theme::default_theme()};
    REQUIRE(router.configure(app::Routes {
        app::route<PlainPage>({.address = "plain"}),
        app::route<SecondPlainPage>({.address = "second"}),
        app::route<HomePage>({.address = "home"}),
    }));

    const auto navigation = router.navigation();

    // 空键是调用方的 bug，不是"没这个路由"——两者必须能区分。
    const auto empty = navigation.navigate_to(app::PageKey {});
    REQUIRE_FALSE(empty);
    REQUIRE(empty.error() == app::NavigationError::invalid_key);

    // 没有注册的页面类型。
    const auto unknown = navigation.navigate_to(app::page_key<FocusablePage>());
    REQUIRE_FALSE(unknown);
    REQUIRE(unknown.error() == app::NavigationError::unknown_route);

    // 注册了但需要构造参数：消费方据此把它从导航里排除。
    const auto needs_params = navigation.navigate_to(app::page_key<HomePage>());
    REQUIRE_FALSE(needs_params);
    REQUIRE(needs_params.error() == app::NavigationError::requires_params);

    // 正常路径：与 navigate<PageT>() 等价，并发布当前路由。
    const auto entered = navigation.navigate_to(app::page_key<SecondPlainPage>());
    REQUIRE(entered.has_value());
    REQUIRE(*entered);
    REQUIRE(router.current_page_key() == app::page_key<SecondPlainPage>());
    REQUIRE(router.current_address() == "second");
    REQUIRE(router.current_entry() != nullptr);
    REQUIRE(router.current_entry()->options.address == "second");
    REQUIRE(router.is_current(app::page_key<SecondPlainPage>()));
    REQUIRE_FALSE(router.is_current(app::page_key<PlainPage>()));

    // 句柄失效后报 Unavailable，而不是去碰一个已经析构的路由表。
    app::Navigation stale;
    {
        app::NanRouter other {graph, theme::default_theme()};
        REQUIRE(other.configure(app::Routes {app::route<PlainPage>({.address = "plain"})}));
        stale = other.navigation();
    }
    const auto expired = stale.navigate_to(app::page_key<PlainPage>());
    REQUIRE_FALSE(expired);
    REQUIRE(expired.error() == app::NavigationError::unavailable);
    REQUIRE_FALSE(stale.valid());
}

TEST_CASE("route entries compare field by field", "[app][router][route]") {
    const auto first = app::route<PlainPage>({.address = "plain", .title = "Plain"});
    const auto same = app::route<PlainPage>({.address = "plain", .title = "Plain"});
    const auto different = app::route<PlainPage>({.address = "other", .title = "Plain"});
    REQUIRE(first == same);
    REQUIRE(first != different);
    REQUIRE(first.options == same.options);
}

TEST_CASE("the router publishes its current page reactively", "[app][router][signal]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};

    // 窗口外壳（导航栏 / 侧边栏）在 set_shell() 时就会绑定它 —— 那时还没有任何页面，
    // 所以初始值必须是空键，且首屏 start() 也要能被观察到。
    auto& current = router.current_page();
    REQUIRE_FALSE(current.get().valid());

    REQUIRE(router.configure(app::Routes {
        app::route<PlainPage>(app::RouteOptions {.address = "plain", .title = "Plain"}),
        app::route<SecondPlainPage>(app::RouteOptions {.address = "second", .title = "Second"}),
    }));

    REQUIRE(router.start<PlainPage>());
    REQUIRE(current.get() == app::nan_type_key<PlainPage>());

    const auto navigation = router.navigation();
    REQUIRE(navigation.navigate<SecondPlainPage>());
    REQUIRE(current.get() == app::nan_type_key<SecondPlainPage>());

    router.clear();
    REQUIRE_FALSE(current.get().valid());

    // 失败的导航不发布：拿不到路由表时 configure 失败，状态保持不变。
    reactive::Graph other_graph;
    app::NanRouter unconfigured {other_graph, theme};
    REQUIRE_FALSE(unconfigured.configure(app::Routes {}));
    REQUIRE_FALSE(unconfigured.current_page().get().valid());
}
