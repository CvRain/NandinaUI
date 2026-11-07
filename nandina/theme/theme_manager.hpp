/**
 * theme/theme_manager — 命名主题、外观感知主题族、revision 通知与原子 DesignSystem 应用。
 *
 * 目录和 ThemeSelection 是唯一可写主题状态；DesignSystem 是当前方案的组件解析适配快照。
 */

#ifndef NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP

#include "appearance.hpp"
#include "design_system.hpp"
#include "motion.hpp"
#include "nan_style.hpp"
#include "theme_scheme.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::theme
{
    /** Latest platform preference snapshot supplied by the application host. */
    struct SystemPreferences {
        ColorAppearance appearance = ColorAppearance::light;
        bool reduced_motion = false;

        auto operator==(const SystemPreferences&) const -> bool = default;
    };

    class ThemeManager;

    /// Internal publication staging: release deferred effects before notifying subscribers.
    /// Callbacks own their temporary state and remain safe if the observer is disconnected.
    struct ThemeObserverUpdate {
        std::function<void()> commit;
        std::function<void()> notify;
    };

    /** 主题 revision 观察者（SceneTree 实现，向挂载节点广播 on_theme_changed）。 */
    class ThemeObserver {
    public:
        virtual ~ThemeObserver() = default;
        virtual void on_theme_revision_changed(const ThemeManager& manager) = 0;
        virtual void on_theme_manager_destroyed(const ThemeManager& manager) noexcept = 0;
        /// Called after all internal observers; all projections stage before any commit/notify.
        /// Projections decide what changed for their own channel (presence, selection,
        /// catalog, change events); equivalent states return an empty update.
        [[nodiscard]] virtual auto prepare_theme_publication(const ThemeManager&)
            -> ThemeObserverUpdate {
            return {};
        }
    };

    /// Synchronous publication rejects reentrant writes before mutation. Delivery continues
    /// after callback failures, then rethrows the first exception without reverting the commit.
    class ThemeManager {
    public:
        ThemeManager();
        ~ThemeManager();

        ThemeManager(const ThemeManager&) = delete;
        auto operator=(const ThemeManager&) -> ThemeManager& = delete;
        ThemeManager(ThemeManager&&) = delete;
        auto operator=(ThemeManager&&) -> ThemeManager& = delete;

        /**
         * @return 当前生效的设计系统快照（含遗留 NanStyle 规则合并）
         */
        [[nodiscard]] auto design_system() const -> const DesignSystem&;

        /**
         * @return 当前生效快照的共享指针（widget 低价持有，避免整份拷贝）
         */
        [[nodiscard]] auto design_system_shared() const noexcept
            -> std::shared_ptr<const DesignSystem>;

        [[nodiscard]] auto register_scheme(ThemeScheme scheme)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto replace_scheme(ThemeScheme scheme)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto remove_scheme(const ThemeSchemeId& id)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto register_family(ThemeSchemeFamily family)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto remove_family(const ThemeFamilyId& id)
            -> std::expected<void, ThemeCatalogError>;
        /// Borrowed directory view; entry pointers/references expire on any successful write.
        /// Copy a ThemeScheme or its shared content when retaining it across a write.
        [[nodiscard]] auto catalog() const noexcept -> const ThemeSchemeCatalog&;
        [[nodiscard]] auto select(ThemeSelection selection)
            -> std::expected<void, ThemeSelectionError>;
        /// Borrowed committed value, invalidated by a successful write.
        [[nodiscard]] auto selection() const noexcept -> const ThemeSelection&;
        /// Null for explicitly unregistered content; copy the ID to keep it across a write.
        [[nodiscard]] auto active_scheme_id() const noexcept -> const ThemeSchemeId*;

        /**
         * Atomically replace the host-supplied platform snapshot.
         * Publishes at most one revision, and only when an effective preference changes.
         */
        void set_system_preferences(SystemPreferences preferences);
        [[nodiscard]] auto system_preferences() const noexcept -> SystemPreferences;
        /// Convenience update for hosts that receive appearance separately.
        void set_system_appearance(ColorAppearance appearance);

        [[nodiscard]] auto theme() const -> const NanTheme&;
        [[nodiscard]] auto appearance() const noexcept -> ColorAppearance;
        void set_motion_preference(MotionPreference preference);
        /// Convenience update for hosts that receive motion separately.
        void set_system_reduced_motion(bool reduced);
        [[nodiscard]] auto motion_preference() const noexcept -> MotionPreference;
        [[nodiscard]] auto reduced_motion() const noexcept -> bool;
        [[nodiscard]] auto revision() const noexcept -> std::uint64_t;

        void set_style(std::shared_ptr<const NanStyle> style);
        [[nodiscard]] auto style() const -> const NanStyle&;

        void add_observer(ThemeObserver& observer);
        void remove_observer(ThemeObserver& observer) noexcept;

    protected:
        /// 通知/销毁期间拒绝修改；测试兼容层（theme_test::Manager）的写入口复用此守卫。
        void require_idle() const;

    private:
        friend struct StyleDocument;
        struct ObserverState;
        void publish_revision();
        /// Staged-only publication: selection or catalog changed while the effective theme
        /// did not. No revision bump and no internal invalidation broadcast.
        void publish_staged();
        void publish_publication(bool invalidate_presentation);
        void commit_catalog(
            ThemeSchemeCatalog catalog,
            bool motion_changed = false,
            std::optional<SystemPreferences> host = std::nullopt,
            std::shared_ptr<const NanStyle> style = {}
        );

        ThemeSchemeCatalog catalog_;
        std::shared_ptr<const ThemeSchemeContent> active_content_;
        std::optional<ThemeSchemeId> active_id_;
        ColorAppearance active_appearance_ = ColorAppearance::light;
        MotionPreference motion_preference_ = MotionPreference::system;
        SystemPreferences system_preferences_;
        std::shared_ptr<const NanStyle> style_ = default_style();
        std::shared_ptr<const DesignSystem> system_;
        std::shared_ptr<const DesignSystem> effective_;
        NanTheme theme_view_;
        std::shared_ptr<ObserverState> observers_;
        std::uint64_t revision_ = 1;
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
