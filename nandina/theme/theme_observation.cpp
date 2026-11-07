// theme/theme_observation — selection, catalog and explicit theme-change channels.
#include "theme_observation.hpp"

#include "../reactive/graph.hpp"

#include <exception>

namespace nandina::theme
{
    namespace
    {
        /// Wrap a subscriber so a throwing handler does not block later subscribers and the
        /// first failure can be rethrown after the whole publication delivered.
        template<typename Impl, typename Payload>
        auto subscribe_guarded(
            reactive::Event<Payload>& event,
            const std::weak_ptr<Impl>& weak,
            std::function<void(Payload)> handler
        ) -> reactive::Subscription {
            return event.subscribe([weak, handler = std::move(handler)](Payload next) {
                const auto current = weak.lock();
                if (!current || !current->connected || current->manager == nullptr) {
                    return;
                }
                try {
                    handler(std::move(next));
                }
                catch (...) {
                    if (!current->callback_failure) {
                        current->callback_failure = std::current_exception();
                    }
                }
            });
        }

        template<typename Impl>
        void rethrow_callback_failure(const std::shared_ptr<Impl>& self) {
            if (auto failure = std::exchange(self->callback_failure, nullptr)) {
                std::rethrow_exception(failure);
            }
        }
    } // namespace

    // ─── selection projection ──────────────────────────────────────────────────

    struct ThemeSelectionPresentation::Impl final:
        ThemeObserver,
        std::enable_shared_from_this<Impl> {
        ThemeManager* manager;
        reactive::Graph& graph;
        reactive::Signal<ThemeSelectionState> state;
        reactive::Event<ThemeSelectionState> changed;
        std::exception_ptr callback_failure;
        bool connected = true;

        Impl(ThemeManager& source, reactive::Graph& target):
            manager(&source),
            graph(target),
            state(target, source.selection()) {}
        Impl(const Impl&) = delete;
        auto operator=(const Impl&) -> Impl& = delete;

        void disconnect() noexcept {
            connected = false;
            if (manager) {
                manager->remove_observer(*this);
                manager = nullptr;
            }
        }

        void on_theme_revision_changed(const ThemeManager&) override {}

        void on_theme_manager_destroyed(const ThemeManager&) noexcept override {
            manager = nullptr;
        }

        auto prepare_theme_publication(const ThemeManager& source) -> ThemeObserverUpdate override {
            const auto next = source.selection();
            if (!connected || manager == nullptr || state.peek() == next) {
                return {};
            }
            auto self = shared_from_this();
            auto deferred = std::make_shared<reactive::Graph::DeferredEffects>(graph);
            state.set(next);
            return {
                .commit = [self, deferred] { deferred->commit(); },
                .notify =
                    [self, next] {
                        if (!self->connected || self->manager == nullptr) {
                            return;
                        }
                        self->callback_failure = nullptr;
                        self->changed.emit(next);
                        rethrow_callback_failure(self);
                    },
            };
        }
    };

    ThemeSelectionPresentation::ThemeSelectionPresentation(
        ThemeManager& manager,
        reactive::Graph& graph
    ):
        impl_(std::make_shared<Impl>(manager, graph)) {
        manager.add_observer(*impl_);
    }

    ThemeSelectionPresentation::~ThemeSelectionPresentation() {
        impl_->disconnect();
    }

    auto ThemeSelectionPresentation::valid() const noexcept -> bool {
        return impl_->manager != nullptr;
    }

    auto ThemeSelectionPresentation::get() const -> ThemeSelectionState {
        return impl_->state.get();
    }

    auto ThemeSelectionPresentation::readonly() const -> reactive::ReadSignal<ThemeSelectionState> {
        return impl_->state.as_readonly();
    }

    auto ThemeSelectionPresentation::subscribe(std::function<void(ThemeSelectionState)> handler)
        -> reactive::Subscription {
        return subscribe_guarded<Impl, ThemeSelectionState>(
            impl_->changed,
            impl_,
            std::move(handler)
        );
    }

    // ─── catalog projection ────────────────────────────────────────────────────

    namespace
    {
        [[nodiscard]] auto catalog_state(const ThemeManager& manager) -> ThemeCatalogState {
            const auto& catalog = manager.catalog();
            return ThemeCatalogState {
                .version = catalog.version(),
                .schemes = catalog.schemes(),
                .families = catalog.families(),
            };
        }
    } // namespace

    struct ThemeCatalogPresentation::Impl final: ThemeObserver, std::enable_shared_from_this<Impl> {
        ThemeManager* manager;
        reactive::Graph& graph;
        reactive::Signal<ThemeCatalogState> state;
        reactive::Event<ThemeCatalogState> changed;
        std::exception_ptr callback_failure;
        bool connected = true;

        Impl(ThemeManager& source, reactive::Graph& target):
            manager(&source),
            graph(target),
            state(target, catalog_state(source)) {}
        Impl(const Impl&) = delete;
        auto operator=(const Impl&) -> Impl& = delete;

        void disconnect() noexcept {
            connected = false;
            if (manager) {
                manager->remove_observer(*this);
                manager = nullptr;
            }
        }

        void on_theme_revision_changed(const ThemeManager&) override {}

        void on_theme_manager_destroyed(const ThemeManager&) noexcept override {
            manager = nullptr;
        }

        auto prepare_theme_publication(const ThemeManager& source) -> ThemeObserverUpdate override {
            if (!connected || manager == nullptr) {
                return {};
            }
            // Entries only: the catalog revision alone decides whether the directory changed.
            if (source.catalog().version() == state.peek().version) {
                return {};
            }
            auto next = catalog_state(source);
            auto self = shared_from_this();
            auto deferred = std::make_shared<reactive::Graph::DeferredEffects>(graph);
            state.set(next);
            return {
                .commit = [self, deferred] { deferred->commit(); },
                .notify =
                    [self, next = std::move(next)] {
                        if (!self->connected || self->manager == nullptr) {
                            return;
                        }
                        self->callback_failure = nullptr;
                        self->changed.emit(next);
                        rethrow_callback_failure(self);
                    },
            };
        }
    };

    ThemeCatalogPresentation::ThemeCatalogPresentation(
        ThemeManager& manager,
        reactive::Graph& graph
    ):
        impl_(std::make_shared<Impl>(manager, graph)) {
        manager.add_observer(*impl_);
    }

    ThemeCatalogPresentation::~ThemeCatalogPresentation() {
        impl_->disconnect();
    }

    auto ThemeCatalogPresentation::valid() const noexcept -> bool {
        return impl_->manager != nullptr;
    }

    auto ThemeCatalogPresentation::get() const -> ThemeCatalogState {
        return impl_->state.get();
    }

    auto ThemeCatalogPresentation::subscribe(std::function<void(ThemeCatalogState)> handler)
        -> reactive::Subscription {
        return subscribe_guarded<Impl, ThemeCatalogState>(
            impl_->changed,
            impl_,
            std::move(handler)
        );
    }

    // ─── explicit theme change channel ─────────────────────────────────────────

    namespace
    {
        [[nodiscard]] auto change_endpoint(const ThemeManager& manager) -> ThemeChangeEndpoint {
            ThemeChangeEndpoint endpoint;
            if (const auto* scheme = manager.active_scheme_id()) {
                endpoint.scheme = *scheme;
            }
            endpoint.appearance = manager.appearance();
            endpoint.system = manager.design_system_shared();
            return endpoint;
        }
    } // namespace

    struct ThemeChangeHub::Impl final: ThemeObserver, std::enable_shared_from_this<Impl> {
        ThemeManager* manager;
        reactive::Event<ThemeChange> changed;
        std::exception_ptr callback_failure;
        bool connected = true;
        std::optional<ThemeChangeEndpoint> previous;

        explicit Impl(ThemeManager& source): manager(&source), previous(change_endpoint(source)) {}
        Impl(const Impl&) = delete;
        auto operator=(const Impl&) -> Impl& = delete;

        void disconnect() noexcept {
            connected = false;
            if (manager) {
                manager->remove_observer(*this);
                manager = nullptr;
            }
        }

        void on_theme_revision_changed(const ThemeManager&) override {}

        void on_theme_manager_destroyed(const ThemeManager&) noexcept override {
            manager = nullptr;
        }

        auto prepare_theme_publication(const ThemeManager& source) -> ThemeObserverUpdate override {
            if (!connected || manager == nullptr) {
                return {};
            }
            auto next = change_endpoint(source);
            if (previous && *previous == next) {
                return {};
            }
            auto before = previous.value_or(next);
            previous = next;
            auto self = shared_from_this();
            return {
                .commit = [] {},
                .notify =
                    [self, before, next] {
                        if (!self->connected || self->manager == nullptr) {
                            return;
                        }
                        self->callback_failure = nullptr;
                        self->changed.emit(ThemeChange {.before = before, .after = next});
                        rethrow_callback_failure(self);
                    },
            };
        }
    };

    ThemeChangeHub::ThemeChangeHub(ThemeManager& manager): impl_(std::make_shared<Impl>(manager)) {
        manager.add_observer(*impl_);
    }

    ThemeChangeHub::~ThemeChangeHub() {
        impl_->disconnect();
    }

    auto ThemeChangeHub::valid() const noexcept -> bool {
        return impl_->manager != nullptr;
    }

    auto ThemeChangeHub::subscribe(std::function<void(const ThemeChange&)> handler)
        -> reactive::Subscription {
        return subscribe_guarded<Impl, ThemeChange>(impl_->changed, impl_, std::move(handler));
    }
} // namespace nandina::theme
