/**
 * theme/theme_manager — 以 DesignSystem 为单一事实来源的主题管理实现。
 *
 * 所有状态变更收敛到 commit()：换 system_ 指针 → 刷新 theme() 视图 → 发布一次
 * revision。遗留 named-theme / family / style API 是选择器层，不绕过原子路径。
 */

#include "theme_manager.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::theme
{
    namespace
    {
        /** 把遗留 NanTheme（tokens + 单调色板）包装成 DesignSystem 快照。 */
        [[nodiscard]] auto theme_to_design_system(const NanTheme& theme)
            -> std::shared_ptr<const DesignSystem> {
            return std::make_shared<const DesignSystem>(design_system_from_theme(theme));
        }

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

    ThemeManager::ThemeManager() {
        system_ = std::make_shared<const DesignSystem>(default_design_system());
        effective_ = merge_style(system_, style_);
        theme_view_ = NanTheme {system_->tokens, system_->palette(appearance())};
    }

    ThemeManager::~ThemeManager() {
        const auto observers = std::move(observers_);
        for (auto* observer: observers) {
            observer->on_theme_manager_destroyed(*this);
        }
    }

    void ThemeManager::apply(std::shared_ptr<const DesignSystem> system) {
        if (!system) {
            throw std::invalid_argument("ThemeManager design system cannot be null");
        }
        active_family_.clear();
        active_name_ = "application";
        commit(std::move(system));
    }

    auto ThemeManager::design_system() const -> const DesignSystem& {
        return *effective_;
    }

    auto ThemeManager::design_system_shared() const noexcept
        -> std::shared_ptr<const DesignSystem> {
        return effective_;
    }

    auto ThemeManager::register_theme(std::string name, NanTheme theme) -> bool {
        if (name.empty()) {
            throw std::invalid_argument("ThemeManager theme name cannot be empty");
        }
        const bool replaced = themes_.contains(name);
        const bool replaces_active = active_name() == name;
        themes_.insert_or_assign(std::move(name), theme_to_design_system(theme));
        if (replaces_active) {
            sync_to_effective();
        }
        return !replaced;
    }

    auto ThemeManager::contains(const std::string_view name) const -> bool {
        return themes_.contains(name);
    }

    auto ThemeManager::activate(const std::string_view name) -> bool {
        const auto found = themes_.find(name);
        if (found == themes_.end()) {
            return false;
        }
        if (active_family_.empty() && active_name_ == name) {
            return true;
        }
        active_family_.clear();
        active_name_ = found->first;
        commit(found->second);
        return true;
    }

    void ThemeManager::set_theme(NanTheme theme) {
        apply(theme_to_design_system(theme));
    }

    auto ThemeManager::register_family(
        std::string name,
        std::string light_theme,
        std::string dark_theme
    ) -> bool {
        if (name.empty()) {
            throw std::invalid_argument("ThemeManager family name cannot be empty");
        }
        if (!themes_.contains(light_theme) || !themes_.contains(dark_theme)) {
            throw std::invalid_argument("ThemeManager family variants must be registered themes");
        }
        const bool replaced = families_.contains(name);
        families_.insert_or_assign(
            std::move(name),
            ThemeFamily {
                .light_theme = std::move(light_theme),
                .dark_theme = std::move(dark_theme),
            }
        );
        if (active_family_ == name) {
            sync_to_effective();
        }
        return !replaced;
    }

    auto ThemeManager::contains_family(const std::string_view name) const -> bool {
        return families_.contains(name) || system_families_.contains(name);
    }

    auto ThemeManager::activate_family(const std::string_view name) -> bool {
        // 全快照族优先：整体 apply，外观翻转由 appearance() 在快照内完成。
        if (const auto found = system_families_.find(name); found != system_families_.end()) {
            if (active_family_ == name) {
                return true;
            }
            active_family_ = found->first;
            commit(found->second);
            return true;
        }
        const auto found = families_.find(name);
        if (found == families_.end()) {
            return false;
        }
        if (active_family_ == name) {
            return true;
        }
        active_family_ = found->first;
        sync_to_effective();
        return true;
    }

    void ThemeManager::register_theme_family(std::string name, DesignSystem system) {
        if (name.empty()) {
            throw std::invalid_argument("ThemeManager family name cannot be empty");
        }
        auto snapshot = std::make_shared<const DesignSystem>(std::move(system));
        const bool replaces_active = active_family_ == name;
        system_families_.insert_or_assign(std::move(name), std::move(snapshot));
        if (replaces_active) {
            sync_to_effective();
        }
    }

    void ThemeManager::set_preference(const ThemePreference preference) {
        if (preference_ == preference) {
            return;
        }
        const auto previous = appearance();
        preference_ = preference;
        if (appearance() != previous) {
            sync_to_effective();
        }
    }

    void ThemeManager::set_system_preferences(const SystemPreferences preferences) {
        if (system_preferences_ == preferences) {
            return;
        }

        const auto previous_appearance = appearance();
        const bool previous_reduced_motion = reduced_motion();
        system_preferences_ = preferences;
        const bool appearance_changed = appearance() != previous_appearance;
        const bool motion_changed = reduced_motion() != previous_reduced_motion;

        if (appearance_changed) {
            const auto previous_revision = revision_;
            sync_to_effective();
            if (motion_changed && revision_ == previous_revision) {
                publish_revision();
            }
        }
        else if (motion_changed) {
            publish_revision();
        }
    }

    auto ThemeManager::system_preferences() const noexcept -> SystemPreferences {
        return system_preferences_;
    }

    void ThemeManager::set_system_appearance(const ColorAppearance appearance) {
        auto preferences = system_preferences_;
        preferences.appearance = appearance;
        set_system_preferences(preferences);
    }

    auto ThemeManager::theme() const -> const NanTheme& {
        return theme_view_;
    }

    auto ThemeManager::active_name() const noexcept -> std::string_view {
        return effective_theme_name();
    }

    auto ThemeManager::active_family() const noexcept -> std::string_view {
        return active_family_;
    }

    auto ThemeManager::preference() const noexcept -> ThemePreference {
        return preference_;
    }

    auto ThemeManager::appearance() const noexcept -> ColorAppearance {
        switch (preference_) {
            case ThemePreference::light:
                return ColorAppearance::light;
            case ThemePreference::dark:
                return ColorAppearance::dark;
            case ThemePreference::system:
                return system_preferences_.appearance;
        }
        return system_preferences_.appearance;
    }

    void ThemeManager::set_motion_preference(const MotionPreference preference) {
        const bool previous = reduced_motion();
        motion_preference_ = preference;
        if (reduced_motion() != previous) {
            publish_revision();
        }
    }

    void ThemeManager::set_system_reduced_motion(const bool reduced) {
        auto preferences = system_preferences_;
        preferences.reduced_motion = reduced;
        set_system_preferences(preferences);
    }

    auto ThemeManager::motion_preference() const noexcept -> MotionPreference {
        return motion_preference_;
    }

    auto ThemeManager::reduced_motion() const noexcept -> bool {
        return resolve_reduced_motion(
            motion_preference_,
            system_preferences_.reduced_motion
        );
    }

    auto ThemeManager::revision() const noexcept -> std::uint64_t {
        return revision_;
    }

    void ThemeManager::set_style(std::shared_ptr<const NanStyle> style) {
        if (!style) {
            throw std::invalid_argument("ThemeManager style cannot be null");
        }
        style_ = std::move(style);
        rebuild_effective();
    }

    auto ThemeManager::style() const -> const NanStyle& {
        return *style_;
    }

    void ThemeManager::add_observer(ThemeObserver& observer) {
        if (std::ranges::find(observers_, &observer) == observers_.end()) {
            observers_.push_back(&observer);
        }
    }

    void ThemeManager::remove_observer(ThemeObserver& observer) noexcept {
        std::erase(observers_, &observer);
    }

    auto ThemeManager::effective_theme_name() const noexcept -> std::string_view {
        if (active_family_.empty()) {
            return active_name_;
        }
        if (system_families_.contains(active_family_)) {
            return active_family_; // 全快照族：不在 themes_ 中命中，外观翻转走 refresh_theme_view
        }
        const auto& family = families_.at(active_family_);
        return appearance() == ColorAppearance::dark ? family.dark_theme : family.light_theme;
    }

    void ThemeManager::commit(std::shared_ptr<const DesignSystem> system) {
        if (system_ == system) {
            return; // 指针相同：无变化，不发布额外 revision
        }
        system_ = std::move(system);
        rebuild_effective();
    }

    void ThemeManager::sync_to_effective() {
        // 全快照族：快照已内嵌两套 palette，外观变化只需刷新 theme() 视图。
        if (!active_family_.empty() && system_families_.contains(active_family_)) {
            refresh_theme_view();
            return;
        }
        const auto effective = effective_theme_name();
        const auto found = themes_.find(effective);
        if (found != themes_.end()) {
            commit(found->second);
        }
        else {
            refresh_theme_view(); // 无名主题路径（apply / set_theme）：仅外观变体可能变化
        }
    }

    void ThemeManager::rebuild_effective() {
        effective_ = merge_style(system_, style_);
        theme_view_ = NanTheme {system_->tokens, system_->palette(appearance())};
        publish_revision();
    }

    void ThemeManager::refresh_theme_view() {
        theme_view_ = NanTheme {system_->tokens, system_->palette(appearance())};
        publish_revision();
    }

    void ThemeManager::publish_revision() {
        ++revision_;
        const auto observers = observers_;
        for (auto* observer: observers) {
            if (std::ranges::find(observers_, observer) != observers_.end()) {
                observer->on_theme_revision_changed(*this);
            }
        }
    }

} // namespace nandina::theme
