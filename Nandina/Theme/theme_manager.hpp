//
// Created by cvrain on 2026/2/28.
// Refactored: enum-based theme selection for type safety.
//

#ifndef NANDINA_THEME_MANAGER_HPP
#define NANDINA_THEME_MANAGER_HPP

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

#include "color_schema.hpp"
#include "nandina_types.hpp"
#include "primitive_schema.hpp"

#if defined(_WIN32)
#if defined(NandinaTheme_EXPORTS)
#define NANDINA_THEME_EXPORT Q_DECL_EXPORT
#else
#define NANDINA_THEME_EXPORT Q_DECL_IMPORT
#endif
#else
#define NANDINA_THEME_EXPORT
#endif

namespace Nandina::Theme {

    /**
     * @brief Central theme manager singleton for the Nandina component library.
     *
     * Provides global access to the current theme's colors and design primitives.
     * Supports runtime theme switching among built-in presets and dark mode toggling.
     *
     * QML Usage:
     * @code
     * import Nandina.Theme
     * import Nandina.Core   // for NandinaType enum access
     *
     * Rectangle {
     *     color: ThemeManager.darkMode
     *         ? ThemeManager.colors.surface.shade950
     *         : ThemeManager.colors.surface.shade50
     *     radius: ThemeManager.primitives.radiusBase
     *
     *     Text {
     *         text: "Hello"
     *         // Indexed access:
     *         color: ThemeManager.colors.color(NandinaType.Primary, NandinaType.Shade500)
     *         font.family: ThemeManager.primitives.baseFont.fontFamily
     *     }
     * }
     *
     * // Switch theme at runtime (enum-based, type-safe)
     * ThemeManager.currentTheme = NandinaType.Cerberus
     * ThemeManager.darkMode = true
     *
     * // Or by name (convenience)
     * ThemeManager.setThemeByName("catppuccin")
     * @endcode
     */
    class NANDINA_THEME_EXPORT ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        Q_PROPERTY(Nandina::Types::ThemePreset currentTheme READ currentTheme WRITE setCurrentTheme NOTIFY
                           currentThemeChanged)
        Q_PROPERTY(QString currentThemeName READ currentThemeName NOTIFY currentThemeChanged)
        Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)

        Q_PROPERTY(Nandina::Core::Color::ColorSchema *colors READ colors CONSTANT)
        Q_PROPERTY(Nandina::Core::Primitives::PrimitiveSchema *primitives READ primitives CONSTANT)

        Q_PROPERTY(QStringList availableThemes READ availableThemes CONSTANT)

    public:
        explicit ThemeManager(QObject *parent = nullptr);
        ~ThemeManager() override = default;

        /// Get the singleton instance (for C++ usage)
        static ThemeManager *instance();

        /// Qt6 QML_SINGLETON create function
        static ThemeManager *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

        // ── Properties ──

        [[nodiscard]] Nandina::Types::ThemePreset currentTheme() const {
            return m_currentTheme;
        }
        [[nodiscard]] QString currentThemeName() const {
            return Nandina::Types::themePresetName(m_currentTheme);
        }
        [[nodiscard]] bool darkMode() const {
            return m_darkMode;
        }

        [[nodiscard]] Nandina::Core::Color::ColorSchema *colors() const {
            return m_colors;
        }
        [[nodiscard]] Nandina::Core::Primitives::PrimitiveSchema *primitives() const {
            return m_primitives;
        }

        [[nodiscard]] QStringList availableThemes() const;

        void setCurrentTheme(Nandina::Types::ThemePreset theme);
        void setDarkMode(bool dark);

        /// Set theme by lowercase name string (convenience for QML).
        Q_INVOKABLE void setThemeByName(const QString &name);

        /// Get display name for a preset.
        Q_INVOKABLE QString themeName(Nandina::Types::ThemePreset preset) const;

        /// Resolve body background accounting for dark mode.
        Q_INVOKABLE QColor resolveBodyBackground() const;

    signals:
        void currentThemeChanged();
        void darkModeChanged();
        void themeApplied();

    private:
        void applyTheme();

        static ThemeManager *s_instance;

        Nandina::Types::ThemePreset m_currentTheme{Nandina::Types::ThemePreset::Cerberus};
        bool m_darkMode{false};

        Nandina::Core::Color::ColorSchema *m_colors;
        Nandina::Core::Primitives::PrimitiveSchema *m_primitives;
    };

} // namespace Nandina::Theme

#endif // NANDINA_THEME_MANAGER_HPP
