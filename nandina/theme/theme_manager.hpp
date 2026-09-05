/**
 * theme/theme_manager — 命名主题、外观感知主题族、revision 通知与原子 DesignSystem 应用。
 *
 * 单一事实来源是当前 DesignSystem 快照（system_）。遗留 API（register_theme /
 * activate / register_family / set_preference / set_style）都是其上的选择器与兼容糖，
 * 最终都收敛到一次 commit()：换指针 + 更新 theme() 视图 + 发布一次 revision。
 */

#ifndef NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP

#include "appearance.hpp"
#include "design_system.hpp"
#include "motion.hpp"
#include "nan_style.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::theme
{
    /** 命名主题族：亮色主题名 + 暗色主题名（引用 register_theme 注册的名称）。 */
    using ThemeFamily = struct ThemeFamily {
        std::string light_theme;
        std::string dark_theme;
    };

    /** Latest platform preference snapshot supplied by the application host. */
    struct SystemPreferences {
        ColorAppearance appearance = ColorAppearance::light;
        bool reduced_motion = false;

        auto operator==(const SystemPreferences&) const -> bool = default;
    };

    class ThemeManager;

    /** 主题 revision 观察者（SceneTree 实现，向挂载节点广播 on_theme_changed）。 */
    class ThemeObserver {
    public:
        virtual ~ThemeObserver() = default;
        virtual void on_theme_revision_changed(const ThemeManager& manager) = 0;
        virtual void on_theme_manager_destroyed(const ThemeManager& manager) noexcept = 0;
    };

    class ThemeManager {
    public:
        ThemeManager();
        ~ThemeManager();

        ThemeManager(const ThemeManager&) = delete;
        auto operator=(const ThemeManager&) -> ThemeManager& = delete;
        ThemeManager(ThemeManager&&) = delete;
        auto operator=(ThemeManager&&) -> ThemeManager& = delete;

        // ─── 新原子 API ────────────────────────────────────────────────────────

        /**
         * 原子应用一个设计系统快照：一次指针交换 + 一次 revision 发布。
         * 挂载组件只会看到一次切换，不会看到中间状态。
         *
         * @param system 不可变快照；为 nullptr 时抛出 std::invalid_argument
         */
        void apply(std::shared_ptr<const DesignSystem> system);

        /**
         * @return 当前生效的设计系统快照（含遗留 NanStyle 规则合并）
         */
        [[nodiscard]] auto design_system() const -> const DesignSystem&;

        /**
         * @return 当前生效快照的共享指针（widget 低价持有，避免整份拷贝）
         */
        [[nodiscard]] auto design_system_shared() const noexcept
            -> std::shared_ptr<const DesignSystem>;

        // ─── 遗留 API（DesignSystem 之上的选择器 / 兼容糖） ──────────────────

        [[nodiscard]] auto register_theme(std::string name, NanTheme theme) -> bool;
        [[nodiscard]] auto contains(std::string_view name) const -> bool;
        [[nodiscard]] auto activate(std::string_view name) -> bool;
        void set_theme(NanTheme theme);

        [[nodiscard]] auto
        register_family(std::string name, std::string light_theme, std::string dark_theme) -> bool;
        [[nodiscard]] auto contains_family(std::string_view name) const -> bool;
        [[nodiscard]] auto activate_family(std::string_view name) -> bool;

        /**
         * 注册「全快照主题族」：一个内嵌 light+dark 的 DesignSystem。
         * activate_family 会原子 apply 整个快照；set_preference 只在其内翻转外观。
         */
        void register_theme_family(std::string name, DesignSystem system);

        void set_preference(ThemePreference preference);
        /**
         * Atomically replace the host-supplied platform snapshot.
         * Publishes at most one revision, and only when an effective preference changes.
         */
        void set_system_preferences(SystemPreferences preferences);
        [[nodiscard]] auto system_preferences() const noexcept -> SystemPreferences;
        /// Convenience update for hosts that receive appearance separately.
        void set_system_appearance(ColorAppearance appearance);

        [[nodiscard]] auto theme() const -> const NanTheme&;
        [[nodiscard]] auto active_name() const noexcept -> std::string_view;
        [[nodiscard]] auto active_family() const noexcept -> std::string_view;
        [[nodiscard]] auto preference() const noexcept -> ThemePreference;
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

    private:
        [[nodiscard]] auto effective_theme_name() const noexcept -> std::string_view;
        void commit(std::shared_ptr<const DesignSystem> system);
        void sync_to_effective();
        void rebuild_effective();
        void refresh_theme_view();
        void publish_revision();

        std::map<std::string, std::shared_ptr<const DesignSystem>, std::less<>> themes_;
        std::map<std::string, ThemeFamily, std::less<>> families_;
        /// 全快照主题族（内嵌 light+dark），activate 时整体 apply。
        std::map<std::string, std::shared_ptr<const DesignSystem>, std::less<>> system_families_;
        std::string active_name_ = "default";
        std::string active_family_;
        ThemePreference preference_ = ThemePreference::system;
        MotionPreference motion_preference_ = MotionPreference::system;
        SystemPreferences system_preferences_;
        std::shared_ptr<const NanStyle> style_ = default_style();
        std::shared_ptr<const DesignSystem> system_;
        std::shared_ptr<const DesignSystem> effective_;
        NanTheme theme_view_;
        std::vector<ThemeObserver*> observers_;
        std::uint64_t revision_ = 1;
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
