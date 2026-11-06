/**
 * theme/theme_manager — stage catalog and selection changes, then publish one resolved snapshot.
 * Scheme content is the source of truth; DesignSystem adapts one palette for existing widgets.
 * Host appearance, motion and optional style overlays are committed before observers run.
 */

#include "theme_manager.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

namespace nandina::theme
{
    struct ThemeManager::ObserverState {
        struct Entry {
            ThemeObserver* observer;
        };
        ThemeManager* manager = nullptr;
        bool publishing = false;
        std::vector<std::shared_ptr<Entry>> entries;
    };

    namespace
    {
        /** 遗留 ButtonStyleRule → 配方规则（字段槽位映射）。 */
        [[nodiscard]] auto to_recipe_rule(const ButtonStyleRule& legacy) -> ButtonRecipeRule {
            ButtonRecipeRule rule;
            rule.selector = legacy.selector;
            rule.container_fill = legacy.background;
            rule.container_border = legacy.border_color;
            rule.container_border_width = legacy.border_width;
            rule.container_radius = legacy.radius;
            rule.label_color = legacy.foreground;
            rule.label_font_size = legacy.font_size;
            rule.focus_ring_color = legacy.focus_ring_color;
            rule.focus_ring_width = legacy.focus_ring_width;
            rule.metrics_height = legacy.height;
            rule.metrics_padding_x = legacy.padding_x;
            return rule;
        }

        /** 遗留 TextFieldStyleRule → 配方规则（字段槽位映射）。 */
        [[nodiscard]] auto to_recipe_rule(const TextFieldStyleRule& legacy) -> TextFieldRecipeRule {
            TextFieldRecipeRule rule;
            rule.state = legacy.state;
            rule.container_fill = legacy.background;
            rule.container_border = legacy.border_color;
            rule.container_border_width = legacy.border_width;
            rule.container_radius = legacy.radius;
            rule.value_color = legacy.foreground;
            rule.placeholder_color = legacy.placeholder;
            rule.selection_color = legacy.selection;
            rule.focus_ring_color = legacy.focus_ring_color;
            rule.focus_ring_width = legacy.focus_ring_width;
            rule.font_size = legacy.font_size;
            rule.metrics_height = legacy.height;
            rule.metrics_padding_x = legacy.padding_x;
            return rule;
        }

        /**
         * 把遗留 NanStyle 规则合并进 DesignSystem 拷贝，生成「有效快照」。
         * 迁移期新旧规则并存：设计系统自带规则 + 遗留 set_style() 规则都生效。
         */
        [[nodiscard]] auto merge_style(
            const std::shared_ptr<const DesignSystem>& system,
            const std::shared_ptr<const NanStyle>& style
        ) -> std::shared_ptr<const DesignSystem> {
            auto merged = std::make_shared<DesignSystem>(*system);
            for (const auto& legacy: style->button_rules()) {
                merged->components.button.rules.push_back(to_recipe_rule(legacy));
            }
            for (const auto& legacy: style->text_field_rules()) {
                merged->components.text_field.rules.push_back(to_recipe_rule(legacy));
            }
            return merged;
        }
    } // namespace

    ThemeManager::ThemeManager(): observers_(std::make_shared<ObserverState>()) {
        observers_->manager = this;
        const auto system = default_design_system();
        const auto light = ThemeSchemeId::parse("nandina:default/light").value();
        const auto dark = ThemeSchemeId::parse("nandina:default/dark").value();
        const auto family = ThemeFamilyId::parse("nandina:default").value();
        (void)catalog_.register_family({family, "Nandina"});
        (void)catalog_.register_scheme(
            {light,
             "Nandina Light",
             ColorAppearance::light,
             family,
             std::make_shared<const ThemeSchemeContent>(
                 ThemeSchemeContent::from(system, ColorAppearance::light)
             )}
        );
        (void)catalog_.register_scheme(
            {dark,
             "Nandina Dark",
             ColorAppearance::dark,
             family,
             std::make_shared<const ThemeSchemeContent>(
                 ThemeSchemeContent::from(system, ColorAppearance::dark)
             )}
        );
        (void)catalog_.select(
            AppearanceThemeSelection {light, dark, ThemePreference::system},
            system_preferences_.appearance
        );
        active_id_ = light;
        active_content_ = catalog_.active()->content;
        system_ = std::make_shared<const DesignSystem>(active_content_->design_system());
        effective_ = merge_style(system_, style_);
        theme_view_ = NanTheme {active_content_->tokens, active_content_->palette};
    }

    ThemeManager::~ThemeManager() {
        const auto observers = observers_;
        observers->manager = nullptr;
        const auto snapshot = observers->entries;
        for (const auto& entry: snapshot) {
            if (auto* observer = std::exchange(entry->observer, nullptr)) {
                observer->on_theme_manager_destroyed(*this);
            }
        }
        observers->entries.clear();
    }

    void ThemeManager::require_idle() const {
        if (observers_->publishing || observers_->manager == nullptr) {
            throw std::logic_error("ThemeManager cannot be changed during theme notification");
        }
    }

    void ThemeManager::commit_catalog(
        ThemeSchemeCatalog catalog,
        const bool motion_changed,
        const std::optional<SystemPreferences> host,
        std::shared_ptr<const NanStyle> style
    ) {
        const auto* current = catalog.active();
        auto next_id = current ? std::optional(current->id) : std::nullopt;
        const auto next_content = current
            ? current->content
            : std::get<UnregisteredThemeSource>(
                  std::get<FixedThemeSelection>(*catalog.selection()).source
              )
                  .content;
        const auto next_appearance = catalog.active_appearance();
        if (!style) {
            style = style_;
        }
        const bool changed = active_content_ != next_content || active_id_ != next_id
            || active_appearance_ != next_appearance;
        const bool style_changed = style_ != style;
        // All allocation and resolution finish before the live selection or content is changed.
        auto system = system_;
        auto effective = effective_;
        auto view = theme_view_;
        if (changed) {
            system = std::make_shared<const DesignSystem>(next_content->design_system());
            view = NanTheme {next_content->tokens, next_content->palette};
        }
        if (changed || style_changed) {
            effective = merge_style(system, style);
        }
        if (host) {
            system_preferences_ = *host;
        }
        catalog_ = std::move(catalog);
        active_content_ = next_content;
        active_id_ = std::move(next_id);
        active_appearance_ = next_appearance;
        system_ = std::move(system);
        effective_ = std::move(effective);
        style_ = std::move(style);
        theme_view_ = std::move(view);
        if (changed || style_changed || motion_changed) {
            publish_revision();
        }
    }

    auto ThemeManager::register_scheme(ThemeScheme scheme)
        -> std::expected<void, ThemeCatalogError> {
        require_idle();
        auto staged = catalog_;
        auto result = staged.register_scheme(std::move(scheme));
        if (result) {
            commit_catalog(std::move(staged));
        }
        return result;
    }
    auto ThemeManager::replace_scheme(ThemeScheme scheme)
        -> std::expected<void, ThemeCatalogError> {
        require_idle();
        auto staged = catalog_;
        auto result = staged.replace_scheme(std::move(scheme));
        if (result) {
            commit_catalog(std::move(staged));
        }
        return result;
    }
    auto ThemeManager::remove_scheme(const ThemeSchemeId& id)
        -> std::expected<void, ThemeCatalogError> {
        require_idle();
        auto staged = catalog_;
        auto result = staged.remove_scheme(id);
        if (result) {
            commit_catalog(std::move(staged));
        }
        return result;
    }
    auto ThemeManager::register_family(ThemeSchemeFamily family)
        -> std::expected<void, ThemeCatalogError> {
        require_idle();
        auto staged = catalog_;
        auto result = staged.register_family(std::move(family));
        if (result) {
            commit_catalog(std::move(staged));
        }
        return result;
    }
    auto ThemeManager::remove_family(const ThemeFamilyId& id)
        -> std::expected<void, ThemeCatalogError> {
        require_idle();
        auto staged = catalog_;
        auto result = staged.remove_family(id);
        if (result) {
            commit_catalog(std::move(staged));
        }
        return result;
    }
    auto ThemeManager::catalog() const noexcept -> const ThemeSchemeCatalog& {
        return catalog_;
    }
    auto ThemeManager::selection() const noexcept -> const ThemeSelection& {
        return *catalog_.selection();
    }
    auto ThemeManager::active_scheme_id() const noexcept -> const ThemeSchemeId* {
        return active_id_ ? &*active_id_ : nullptr;
    }
    auto ThemeManager::select(ThemeSelection selection)
        -> std::expected<void, ThemeSelectionError> {
        require_idle();
        auto staged = catalog_;
        auto result = staged.select(std::move(selection), system_preferences_.appearance);
        if (result) {
            commit_catalog(std::move(staged));
        }
        return result;
    }
    auto ThemeManager::design_system() const -> const DesignSystem& {
        return *effective_;
    }
    auto ThemeManager::design_system_shared() const noexcept
        -> std::shared_ptr<const DesignSystem> {
        return effective_;
    }
    auto ThemeManager::theme() const -> const NanTheme& {
        return theme_view_;
    }
    auto ThemeManager::appearance() const noexcept -> ColorAppearance {
        return active_appearance_;
    }

    void ThemeManager::set_system_preferences(const SystemPreferences preferences) {
        require_idle();
        if (system_preferences_ == preferences) {
            return;
        }
        auto staged = catalog_;
        const auto selected = staged.select(selection(), preferences.appearance);
        if (!selected) {
            throw std::logic_error("Invalid committed theme selection");
        }
        const bool motion_changed = reduced_motion()
            != resolve_reduced_motion(motion_preference_, preferences.reduced_motion);
        commit_catalog(std::move(staged), motion_changed, preferences);
    }
    auto ThemeManager::system_preferences() const noexcept -> SystemPreferences {
        return system_preferences_;
    }
    void ThemeManager::set_system_appearance(const ColorAppearance appearance) {
        require_idle();
        auto preferences = system_preferences_;
        preferences.appearance = appearance;
        set_system_preferences(preferences);
    }
    void ThemeManager::set_motion_preference(const MotionPreference preference) {
        require_idle();
        const bool previous = reduced_motion();
        motion_preference_ = preference;
        if (reduced_motion() != previous) {
            publish_revision();
        }
    }
    void ThemeManager::set_system_reduced_motion(const bool reduced) {
        require_idle();
        auto preferences = system_preferences_;
        preferences.reduced_motion = reduced;
        set_system_preferences(preferences);
    }
    auto ThemeManager::motion_preference() const noexcept -> MotionPreference {
        return motion_preference_;
    }
    auto ThemeManager::reduced_motion() const noexcept -> bool {
        return resolve_reduced_motion(motion_preference_, system_preferences_.reduced_motion);
    }
    auto ThemeManager::revision() const noexcept -> std::uint64_t {
        return revision_;
    }
    void ThemeManager::set_style(std::shared_ptr<const NanStyle> style) {
        require_idle();
        if (!style) {
            throw std::invalid_argument("ThemeManager style cannot be null");
        }
        if (style_ == style) {
            return;
        }
        auto effective = merge_style(system_, style);
        style_ = std::move(style);
        effective_ = std::move(effective);
        publish_revision();
    }
    auto ThemeManager::style() const -> const NanStyle& {
        return *style_;
    }

    void ThemeManager::add_observer(ThemeObserver& observer) {
        if (observers_->manager == nullptr) {
            throw std::logic_error("Cannot observe a destroyed ThemeManager");
        }
        const auto found =
            std::ranges::find_if(observers_->entries, [&observer](const auto& entry) {
                return entry->observer == &observer;
            });
        if (found == observers_->entries.end()) {
            observers_->entries.push_back(std::make_shared<ObserverState::Entry>(&observer));
        }
    }

    void ThemeManager::remove_observer(ThemeObserver& observer) noexcept {
        for (const auto& entry: observers_->entries) {
            if (entry->observer == &observer) {
                entry->observer = nullptr;
            }
        }
        std::erase_if(observers_->entries, [](const auto& entry) {
            return entry->observer == nullptr;
        });
    }

    void ThemeManager::publish_revision() {
        ++revision_;
        const auto observers = observers_;
        struct PublishingGuard {
            std::shared_ptr<ObserverState> state;
            ~PublishingGuard() {
                state->publishing = false;
            }
        } guard {observers};
        observers->publishing = true;
        const auto snapshot = observers->entries;
        std::exception_ptr failure;
        const auto deliver = [&failure](const auto& callback) {
            try {
                callback();
            }
            catch (...) {
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        };
        for (const auto& entry: snapshot) {
            if (entry->observer && observers->manager) {
                deliver([&] { entry->observer->on_theme_revision_changed(*observers->manager); });
            }
        }
        std::vector<ThemeObserverUpdate> updates;
        for (const auto& entry: snapshot) {
            if (entry->observer && observers->manager) {
                deliver([&] {
                    updates.push_back(
                        entry->observer->prepare_theme_presentation(*observers->manager)
                    );
                });
            }
        }
        // All projections are staged before effects or application subscriptions run.
        // Owned callbacks keep their staging state alive even if a scope is removed.
        for (const auto& update: updates) {
            if (update.commit) {
                deliver(update.commit);
            }
        }
        for (const auto& update: updates) {
            if (update.notify) {
                deliver(update.notify);
            }
        }
        if (failure) {
            std::rethrow_exception(failure);
        }
    }

} // namespace nandina::theme
