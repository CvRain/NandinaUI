/**
 * theme/theme_observation — read-only observation channels for D2 step 4.
 *
 * Three scope-bound channels complement the §5.5 presentation projection:
 *   - ThemeSelectionPresentation: selection state (mode, slots, preference), which changes
 *     even when the effective theme does not (e.g. editing the dark slot in light mode);
 *   - ThemeCatalogPresentation: the scheme/family directory (registration of unused schemes);
 *   - ThemeChangeHub: explicit before/after event for effective theme identity, appearance
 *     or content changes; selection-only and catalog-only updates do not fire it.
 *
 * All channels follow the D2 publication protocol: staged writes, synchronous delivery,
 * reentrancy rejection before mutation, and callback exceptions isolated per subscriber.
 * Contract: docs/references/theme_authoring_and_state.md §5.1–§5.4, §14.
 */

#ifndef NANDINA_EXPERIMENT_THEME_THEME_OBSERVATION_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_OBSERVATION_HPP

#include "../reactive/event.hpp"
#include "../reactive/signal.hpp"
#include "theme_manager.hpp"
#include "theme_scheme.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace nandina::theme
{
    /// Selection observation payload: exactly the committed theme selection.
    using ThemeSelectionState = ThemeSelection;

    /// Directory observation payload. `version` is the catalog revision (entries only).
    struct ThemeCatalogState {
        std::uint64_t version = 0;
        std::vector<ThemeScheme> schemes;
        std::vector<ThemeSchemeFamily> families;
    };

    /// One endpoint of an effective theme change. `scheme` is absent for an anonymous
    /// fixed source; `system` owns the effective design-system snapshot at that time.
    struct ThemeChangeEndpoint {
        std::optional<ThemeSchemeId> scheme;
        ColorAppearance appearance = ColorAppearance::light;
        std::shared_ptr<const DesignSystem> system;

        friend auto operator==(const ThemeChangeEndpoint&, const ThemeChangeEndpoint&)
            -> bool = default;
    };

    /// Effective theme change: identity, appearance or effective content snapshot changed.
    /// Motion-only publications and non-effective selection/catalog edits do not fire it.
    struct ThemeChange {
        ThemeChangeEndpoint before;
        ThemeChangeEndpoint after;
    };

    /// Read-only reactive projection of the committed selection (§5.1 selection dimension).
    class ThemeSelectionPresentation final {
    public:
        ThemeSelectionPresentation(ThemeManager& manager, reactive::Graph& graph);
        ~ThemeSelectionPresentation();

        ThemeSelectionPresentation(const ThemeSelectionPresentation&) = delete;
        auto operator=(const ThemeSelectionPresentation&) -> ThemeSelectionPresentation& = delete;
        ThemeSelectionPresentation(ThemeSelectionPresentation&&) = delete;
        auto operator=(ThemeSelectionPresentation&&) -> ThemeSelectionPresentation& = delete;

        [[nodiscard]] auto valid() const noexcept -> bool;
        [[nodiscard]] auto get() const -> ThemeSelectionState;
        /// Borrowed view for computed/bind. The projection and Graph must outlive every read;
        /// manager destruction alone leaves the last selection readable while the scope lives.
        [[nodiscard]] auto readonly() const -> reactive::ReadSignal<ThemeSelectionState>;
        /// Side-effect subscription; no synthetic initial event.
        [[nodiscard]] auto subscribe(std::function<void(ThemeSelectionState)> handler)
            -> reactive::Subscription;

    private:
        struct Impl;
        std::shared_ptr<Impl> impl_;
    };

    /// Read-only reactive projection of the scheme/family directory (§5.1 catalog dimension).
    class ThemeCatalogPresentation final {
    public:
        ThemeCatalogPresentation(ThemeManager& manager, reactive::Graph& graph);
        ~ThemeCatalogPresentation();

        ThemeCatalogPresentation(const ThemeCatalogPresentation&) = delete;
        auto operator=(const ThemeCatalogPresentation&) -> ThemeCatalogPresentation& = delete;
        ThemeCatalogPresentation(ThemeCatalogPresentation&&) = delete;
        auto operator=(ThemeCatalogPresentation&&) -> ThemeCatalogPresentation& = delete;

        [[nodiscard]] auto valid() const noexcept -> bool;
        [[nodiscard]] auto get() const -> ThemeCatalogState;
        [[nodiscard]] auto subscribe(std::function<void(ThemeCatalogState)> handler)
            -> reactive::Subscription;

    private:
        struct Impl;
        std::shared_ptr<Impl> impl_;
    };

    /// Multi-subscriber explicit theme change event; scope-owned through BuildContext.
    class ThemeChangeHub final {
    public:
        explicit ThemeChangeHub(ThemeManager& manager);
        ~ThemeChangeHub();

        ThemeChangeHub(const ThemeChangeHub&) = delete;
        auto operator=(const ThemeChangeHub&) -> ThemeChangeHub& = delete;
        ThemeChangeHub(ThemeChangeHub&&) = delete;
        auto operator=(ThemeChangeHub&&) -> ThemeChangeHub& = delete;

        [[nodiscard]] auto valid() const noexcept -> bool;
        [[nodiscard]] auto subscribe(std::function<void(const ThemeChange&)> handler)
            -> reactive::Subscription;

    private:
        struct Impl;
        std::shared_ptr<Impl> impl_;
    };
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_OBSERVATION_HPP
