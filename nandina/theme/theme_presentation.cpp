// theme/theme_presentation — scope-bound read-only projection of ThemeManager state.
#include "theme_presentation.hpp"

#include "../reactive/graph.hpp"

#include <exception>

namespace nandina::theme
{
    struct ThemePresentation::Impl final: ThemeObserver, std::enable_shared_from_this<Impl> {
        ThemeManager* manager;
        reactive::Graph& graph;
        reactive::Signal<ThemePresentationState> state;
        reactive::Event<ThemePresentationState> changed;
        std::exception_ptr callback_failure;
        bool connected = true;

        Impl(ThemeManager& source, reactive::Graph& target):
            manager(&source),
            graph(target),
            state(target, {.appearance = source.appearance(), .revision = source.revision()}) {}

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

        auto prepare_theme_presentation(const ThemeManager& source)
            -> ThemeObserverUpdate override {
            const ThemePresentationState next {
                .appearance = source.appearance(),
                .revision = source.revision(),
            };
            if (!connected || manager == nullptr || state.peek() == next) {
                return {};
            }
            // Both callbacks retain the signal until this publication finishes. A scope
            // can disappear from an effect or subscriber without deleting the active state.
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
                        if (auto failure = std::exchange(self->callback_failure, nullptr)) {
                            std::rethrow_exception(failure);
                        }
                    },
            };
        }
    };

    ThemePresentation::ThemePresentation(ThemeManager& manager, reactive::Graph& graph):
        impl_(std::make_shared<Impl>(manager, graph)) {
        manager.add_observer(*impl_);
    }

    ThemePresentation::~ThemePresentation() {
        impl_->disconnect();
    }

    auto ThemePresentation::valid() const noexcept -> bool {
        return impl_->manager != nullptr;
    }

    auto ThemePresentation::get() const -> ThemePresentationState {
        return impl_->state.get();
    }

    auto ThemePresentation::appearance() const -> ColorAppearance {
        return get().appearance;
    }

    auto ThemePresentation::revision() const -> std::uint64_t {
        return get().revision;
    }

    auto ThemePresentation::readonly() const -> reactive::ReadSignal<ThemePresentationState> {
        return impl_->state.as_readonly();
    }

    auto ThemePresentation::subscribe(std::function<void(ThemePresentationState)> handler)
        -> reactive::Subscription {
        return impl_->changed.subscribe([weak = std::weak_ptr<Impl>(impl_),
                                         handler =
                                             std::move(handler)](ThemePresentationState next) {
            if (const auto current = weak.lock(); current && current->connected && current->manager)
            {
                try {
                    handler(next);
                }
                catch (...) {
                    if (!current->callback_failure) {
                        current->callback_failure = std::current_exception();
                    }
                }
            }
        });
    }
} // namespace nandina::theme
