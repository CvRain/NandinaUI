//
// Router + Store tests.
//

#include "app/nan_router.hpp"
#include "app/nan_store.hpp"
#include "foundation/geometry.hpp"
#include "reactive/effect.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "scene/input_event.hpp"
#include "theme/theme.hpp"
#include "widget/controls.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <string_view>
#include <thread>

namespace
{
    using namespace nandina;

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

    class HomePage final: public app::NanPageT<HomeParams> {
    public:
        explicit HomePage(HomeParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "home";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(320, 200));
            root->set_name("home-root");
            root->set_position(foundation::NanPoint(static_cast<float>(params().user_id), 0.0F));
            context.store<TestStore>().count.set(1);
            return root;
        }
    };

    class DetailPage final: public app::NanPageT<DetailParams> {
    public:
        explicit DetailPage(DetailParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "detail";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200, 100));
            root->set_name("detail-root");
            context.store<TestStore>().count.set(params().blog_id);
            return root;
        }
    };

    class PlainPage final: public app::NanPageT<app::NoParams> {
    public:
        PlainPage() = default;

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "plain";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(100, 50));
            root->set_background(context.theme().palette.primary);
            return root;
        }
    };

    struct DispatcherProbeParams {
        bool* available = nullptr;
        app::UiDispatcher** dispatcher = nullptr;
    };

    class DispatcherProbePage final: public app::NanPageT<DispatcherProbeParams> {
    public:
        explicit DispatcherProbePage(DispatcherProbeParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "dispatcher-probe";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
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

    class ScopedObserverPage final: public app::NanPageT<ScopedObserverParams> {
    public:
        explicit ScopedObserverPage(ScopedObserverParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "scoped-observer";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
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

    class ScopedEventPage final: public app::NanPageT<ScopedEventParams> {
    public:
        explicit ScopedEventPage(ScopedEventParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "scoped-event";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
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

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "retained-callback";
        }

        [[nodiscard]] auto build(widget::BuildContext& ui) -> widget::View override {
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

    class AsyncPage final: public app::NanPageT<AsyncPageParams> {
    public:
        explicit AsyncPage(AsyncPageParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "async";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
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

    struct TeardownParams {
        bool* destroyed = nullptr;
    };

    class TeardownPage final: public app::NanPageT<TeardownParams> {
    public:
        explicit TeardownPage(TeardownParams params): NanPageT(params) {}

        ~TeardownPage() override {
            *params().destroyed = true;
        }

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "teardown";
        }

        [[nodiscard]] auto build(app::PageContext&) -> std::shared_ptr<scene::NanNode2D> override {
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }
    };

    void tick_router(scene::NanSceneTree& tree, const float dt) {
        tree.process(dt);
        {
            auto phase = tree.enter_phase(scene::FramePhase::animation);
            tree.advance_animations(dt);
        }
    }
} // namespace

TEST_CASE("router pushes keep-alive pages and toggles top visibility", "[app][router]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    router.set_store(store);

    auto& home = router.push<HomePage>(HomeParams {.user_id = 42});
    REQUIRE(home.params().user_id == 42);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");
    REQUIRE(router.host()->child_count() == 1);

    auto* home_root = router.host()->get_child(0)->as_node2d();
    REQUIRE(home_root != nullptr);
    REQUIRE(home_root->visible());
    REQUIRE(store.count.peek() == 1);

    router.push<DetailPage>(DetailParams {.blog_id = 7});
    REQUIRE(router.depth() == 2);
    REQUIRE(router.current_key() == "detail");
    REQUIRE(router.host()->child_count() == 2);
    REQUIRE_FALSE(home_root->visible());
    auto* detail_root = router.host()->get_child(1)->as_node2d();
    REQUIRE(detail_root != nullptr);
    REQUIRE(detail_root->visible());
    REQUIRE(store.count.peek() == 7);

    REQUIRE(router.pop());
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(home_root->visible());
    REQUIRE_FALSE(router.pop());
}

TEST_CASE("router exposes its UI dispatcher through page context", "[app][router][dispatcher]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    app::NanRouter router {graph, theme, nullptr, nullptr, nullptr, nullptr, nullptr, &dispatcher};
    bool available = false;
    app::UiDispatcher* observed = nullptr;

    router.push<DispatcherProbePage>(DispatcherProbeParams {
        .available = &available,
        .dispatcher = &observed,
    });

    REQUIRE(available);
    REQUIRE(observed == &dispatcher);
}

TEST_CASE("router commands defer stack mutations to the UI task phase", "[app][router][command]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    TestStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        theme,
        &store,
        app::nan_type_key<TestStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    router.push<HomePage>(HomeParams {.user_id = 1});

    REQUIRE(router.request_push<DetailPage>(DetailParams {.blog_id = 7}));
    REQUIRE(router.depth() == 1);
    REQUIRE(dispatcher.pending_count() == 1);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.depth() == 2);
    REQUIRE(router.current_key() == "detail");

    REQUIRE(router.request_pop());
    REQUIRE(router.depth() == 2);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.depth() == 1);

    REQUIRE(router.request_push<DetailPage>(DetailParams {.blog_id = 8}));
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.request_pop_to("home"));
    REQUIRE(router.depth() == 2);
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");

    REQUIRE(router.request_replace<PlainPage>());
    REQUIRE(router.current_key() == "home");
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "plain");

    REQUIRE(router.request_clear());
    REQUIRE_FALSE(router.empty());
    REQUIRE(dispatcher.drain() == 1);
    REQUIRE(router.empty());
}

TEST_CASE("queued router commands expire with their router", "[app][router][command]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    {
        app::NanRouter
            router {graph, theme, nullptr, nullptr, nullptr, nullptr, nullptr, &dispatcher};
        REQUIRE(router.request_push<PlainPage>());
    }

    REQUIRE(dispatcher.pending_count() == 1);
    std::size_t drained = 0;
    REQUIRE_NOTHROW(drained = dispatcher.drain());
    REQUIRE(drained == 1);

    app::NanRouter immediate_only {graph, theme};
    REQUIRE_FALSE(immediate_only.request_push<PlainPage>());
    REQUIRE(immediate_only.empty());
}

TEST_CASE("router frame cancellation suppresses page async completion", "[app][router][async]") {
    using namespace std::chrono_literals;

    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::UiDispatcher dispatcher;
    app::BackgroundExecutor executor {1};
    app::NanRouter
        router {graph, theme, nullptr, nullptr, nullptr, nullptr, nullptr, &dispatcher, &executor};
    std::atomic_bool started = false;
    std::atomic_bool cancelled = false;
    bool completed = false;

    router.push<PlainPage>();
    router.push<AsyncPage>(AsyncPageParams {
        .started = &started,
        .cancelled = &cancelled,
        .completed = &completed,
    });
    const auto start_deadline = std::chrono::steady_clock::now() + 2s;
    while (!started.load(std::memory_order_acquire)
           && std::chrono::steady_clock::now() < start_deadline)
    {
        std::this_thread::sleep_for(1ms);
    }
    REQUIRE(started.load(std::memory_order_acquire));

    REQUIRE(router.pop());
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

TEST_CASE("router pop_to keeps target frame and removes newer frames", "[app][router]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme, &store, app::nan_type_key<TestStore>()};

    router.push<HomePage>(HomeParams {.user_id = 1});
    router.push<DetailPage>(DetailParams {.blog_id = 2});
    router.push<DetailPage>(DetailParams {.blog_id = 3});

    REQUIRE(router.depth() == 3);
    REQUIRE(router.pop_to("detail"));
    REQUIRE(router.depth() == 3);
    REQUIRE(router.pop_to("home"));
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE_FALSE(router.pop_to("missing"));
}

TEST_CASE("store updates propagate to keep-alive page effects", "[app][router][store]") {
    reactive::Graph graph;
    TestStore store {graph};
    int observed = 0;

    {
        reactive::EffectScope scope {graph};
        scope.add([&] { observed = store.count.get(); });

        const auto theme = theme::default_theme();
        app::NanRouter router {graph, theme, &store, app::nan_type_key<TestStore>()};
        router.push<HomePage>(HomeParams {.user_id = 1});
        REQUIRE(observed == 1);

        router.push<DetailPage>(DetailParams {.blog_id = 9});
        REQUIRE(observed == 9);

        store.count.set(12);
        REQUIRE(observed == 12);
    }
}

TEST_CASE(
    "router supports no-params pages and passes theme through context",
    "[app][router][theme]"
) {
    reactive::Graph graph;
    auto app_theme = theme::default_theme();
    app_theme.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    app::NanRouter router {graph, app_theme};

    auto& page = router.push<PlainPage>();
    REQUIRE(page.params_type_key() == app::nan_type_key<app::NoParams>());
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "plain");

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

TEST_CASE("router clears page reactive scope when a frame is popped", "[app][router][scope]") {
    reactive::Graph graph;
    TestStore store {graph};
    ScopedObserverLog log;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme, &store, app::nan_type_key<TestStore>()};

    router.push<HomePage>(HomeParams {.user_id = 1});
    router.push<ScopedObserverPage>(ScopedObserverParams {.log = &log});

    REQUIRE(log.observed == 1);
    REQUIRE(log.runs == 1);

    store.count.set(5);
    REQUIRE(log.observed == 5);
    REQUIRE(log.runs == 2);

    REQUIRE(router.pop());
    store.count.set(9);
    REQUIRE(log.observed == 5);
    REQUIRE(log.runs == 2);
}

TEST_CASE(
    "router disconnects page event subscriptions when a frame is popped",
    "[app][router][scope]"
) {
    reactive::Graph graph;
    reactive::Event<int> event;
    TestStore store {graph};
    int observed = 0;
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes, &store, app::nan_type_key<TestStore>()};

    router.push<HomePage>(HomeParams {.user_id = 1});
    router.push<ScopedEventPage>(ScopedEventParams {.event = &event, .observed = &observed});
    REQUIRE(event.subscriber_count() == 1);

    event.emit(2);
    REQUIRE(observed == 2);
    REQUIRE(router.pop());
    REQUIRE(event.subscriber_count() == 0);

    event.emit(3);
    REQUIRE(observed == 2);
}

TEST_CASE("retained page roots cannot invoke callbacks after pop", "[app][router][lifecycle]") {
    reactive::Graph graph;
    TestStore store {graph};
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes, &store, app::nan_type_key<TestStore>()};
    std::shared_ptr<widget::Button> retained;
    int calls = 0;
    bool destroyed = false;

    router.push<HomePage>(HomeParams {.user_id = 1});
    router.push<RetainedCallbackPage>(RetainedCallbackParams {
        .root = &retained,
        .calls = &calls,
        .destroyed = &destroyed,
    });

    scene::KeyEvent active_click {257, scene::KeyEvent::Action::press};
    REQUIRE(retained->on_input(active_click));
    REQUIRE(calls == 1);

    REQUIRE(router.pop());
    REQUIRE(destroyed);
    scene::KeyEvent stale_click {257, scene::KeyEvent::Action::press};
    REQUIRE(retained->on_input(stale_click));
    REQUIRE(calls == 1);
}

struct LifecycleLog {
    int activations = 0;
    int deactivations = 0;
};

struct LifecycleParams {
    LifecycleLog* log = nullptr;
    std::string key = "lifecycle";
};

class LifecyclePage final: public app::NanPageT<LifecycleParams> {
public:
    explicit LifecyclePage(LifecycleParams params): NanPageT(params) {}

    [[nodiscard]] auto route_key() const -> std::string_view override {
        return params().key;
    }

    [[nodiscard]] auto build(app::PageContext& context)
        -> std::shared_ptr<scene::NanNode2D> override {
        return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
    }

    void on_activate(app::PageContext& /*context*/) override {
        ++params().log->activations;
    }

    void on_deactivate(app::PageContext& /*context*/) override {
        ++params().log->deactivations;
    }
};

TEST_CASE("page on_activate fires on initial push", "[app][router][lifecycle]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    LifecycleLog log;

    router.push<LifecyclePage>(LifecycleParams {.log = &log});
    REQUIRE(log.activations == 1);
    REQUIRE(log.deactivations == 0);
}

TEST_CASE("page on_deactivate fires when another page pushes on top", "[app][router][lifecycle]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    LifecycleLog first_log;
    LifecycleLog second_log;

    router.push<LifecyclePage>(LifecycleParams {.log = &first_log});
    REQUIRE(first_log.activations == 1);
    REQUIRE(first_log.deactivations == 0);

    router.push<LifecyclePage>(LifecycleParams {.log = &second_log});
    REQUIRE(first_log.deactivations == 1);
    REQUIRE(second_log.activations == 1);
    REQUIRE(second_log.deactivations == 0);
}

TEST_CASE("page on_activate fires when pop restores keep-alive page", "[app][router][lifecycle]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    LifecycleLog first_log;
    LifecycleLog second_log;

    router.push<LifecyclePage>(LifecycleParams {.log = &first_log});
    router.push<LifecyclePage>(LifecycleParams {.log = &second_log});
    // first: 1 activation, 1 deactivation; second: 1 activation
    REQUIRE(first_log.activations == 1);
    REQUIRE(first_log.deactivations == 1);
    REQUIRE(second_log.activations == 1);

    REQUIRE(router.pop());
    // second page dropped (deactivation already handled by drop_frame)
    // first page reactivated
    REQUIRE(first_log.activations == 2);
}

TEST_CASE("pop_to reactivates target keep-alive page", "[app][router][lifecycle]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    LifecycleLog first_log;
    LifecycleLog middle_log;
    LifecycleLog last_log;

    router.push<LifecyclePage>(LifecycleParams {.log = &first_log, .key = "first"});
    router.push<LifecyclePage>(LifecycleParams {.log = &middle_log, .key = "middle"});
    router.push<LifecyclePage>(LifecycleParams {.log = &last_log, .key = "last"});

    REQUIRE(first_log.activations == 1);
    REQUIRE(first_log.deactivations == 1);
    REQUIRE(middle_log.activations == 1);
    REQUIRE(middle_log.deactivations == 1);
    REQUIRE(last_log.activations == 1);

    REQUIRE(router.pop_to("first"));
    // pop_to removes middle and last, reactivates first
    REQUIRE(first_log.activations == 2);
}

TEST_CASE("router clear deactivates active page", "[app][router][lifecycle]") {
    reactive::Graph graph;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    LifecycleLog log;

    router.push<LifecyclePage>(LifecycleParams {.log = &log});
    REQUIRE(log.activations == 1);

    router.clear();
    REQUIRE(log.deactivations == 1);
}

TEST_CASE("router transition fades pages in and preserves lifecycle on exit", "[app][router][transition]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes};
    router.set_transition_enabled(true);
    router.set_transition_duration(0.2F);

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());

    router.push<PlainPage>();
    bool destroyed = false;
    router.push<TeardownPage>(TeardownParams {.destroyed = &destroyed});
    REQUIRE(router.depth() == 2);
    REQUIRE_FALSE(destroyed);

    // 开启转场后，host 的直接子节点是包装帧；顶层页淡入（opacity 从 0 开始）。
    auto* top_frame = router.host()->get_child(1)->as_control();
    REQUIRE(top_frame != nullptr);
    REQUIRE(top_frame->local_opacity() < 1.0F);

    // 推进到淡入完成，进入稳定打开状态。
    for (int i = 0; i < 30; ++i) {
        tick_router(tree, 1.0F / 60.0F);
    }
    REQUIRE(top_frame->local_opacity() == Catch::Approx(1.0F));

    // pop：栈深度立即减少，但被替换页面的生命周期在淡出期间仍保留。
    REQUIRE(router.pop());
    REQUIRE(router.depth() == 1);
    REQUIRE_FALSE(destroyed);

    // 淡出完成后，drop 轮询销毁页面。
    for (int i = 0; i < 30; ++i) {
        tick_router(tree, 1.0F / 60.0F);
    }
    REQUIRE(destroyed);
}

TEST_CASE("router with transition disabled drops pages immediately", "[app][router][transition]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes};
    REQUIRE_FALSE(router.transition_enabled());

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());

    router.push<PlainPage>();
    bool destroyed = false;
    router.push<TeardownPage>(TeardownParams {.destroyed = &destroyed});

    // 关闭转场：host 直接持有页面根（无包装），pop 即时销毁。
    REQUIRE(router.pop());
    REQUIRE(destroyed);
}
