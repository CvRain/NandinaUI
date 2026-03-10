//
// Created by cvrain on 2026/3/1.
//

#ifndef NANDINA_THEME_MANAGER_HPP
#define NANDINA_THEME_MANAGER_HPP

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

#include "color_factory.hpp"
#include "color_schema.hpp"
#include "font_manager.hpp"
#include "primitive_factory.hpp"
#include "primitive_schema.hpp"
#include "theme_type.hpp"

namespace Nandina::Theme {

    using ThemeTypes = Core::Types::ThemeVariant::ThemeTypes;
    using ColorVariantTypes = Core::Types::ThemeVariant::ColorVariantTypes;
    using ColorAccentTypes = Core::Types::ThemeVariant::ColorAccentTypes;

    class ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        // ── Properties ─────────────────────────────────────────────
        Q_PROPERTY(Nandina::Core::Types::ThemeVariant::ThemeTypes currentTheme READ currentTheme WRITE setCurrentTheme
                           NOTIFY currentThemeChanged)
        Q_PROPERTY(QString currentThemeName READ currentThemeName NOTIFY currentThemeChanged)
        Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
        Q_PROPERTY(Nandina::Core::Color::ColorSchema *colors READ colors CONSTANT)
        Q_PROPERTY(Nandina::Core::Primitives::PrimitiveSchema *primitives READ primitives CONSTANT)
        Q_PROPERTY(QStringList availableThemes READ availableThemes CONSTANT)

    public:
        explicit ThemeManager(QObject *parent = nullptr);

        // ── Getters ────────────────────────────────────────────────
        [[nodiscard]] ThemeTypes currentTheme() const;
        [[nodiscard]] QString currentThemeName() const;
        [[nodiscard]] bool darkMode() const;
        [[nodiscard]] Core::Color::ColorSchema *colors() const;
        [[nodiscard]] Core::Primitives::PrimitiveSchema *primitives() const;
        [[nodiscard]] QStringList availableThemes() const;

        // ── Setters ────────────────────────────────────────────────
        void setCurrentTheme(ThemeTypes theme);
        void setDarkMode(bool dark);

        // ── QML convenience methods ────────────────────────────────

        /// Switch theme by name string (e.g. "catppuccin"). Case-insensitive.
        Q_INVOKABLE void setThemeByName(const QString &name);

        /// Get the display name of a theme enum value.
        Q_INVOKABLE static QString themeName(Nandina::Core::Types::ThemeVariant::ThemeTypes theme);

    signals:
        void currentThemeChanged();
        void darkModeChanged();
        void themeApplied();

    private:
        void applyCurrentTheme();

        ThemeTypes m_currentTheme{ThemeTypes::cerberus};
        bool m_darkMode{false};
        Core::Color::ColorSchema *m_colors{nullptr};
        Core::Primitives::PrimitiveSchema *m_primitives{nullptr};
    };

} // namespace Nandina::Theme

#endif // NANDINA_THEME_MANAGER_HPP
