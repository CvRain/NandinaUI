//
// Created by cvrain on 2026/3/1.
//

#include "theme_manager.hpp"

namespace Nandina::Theme {

    ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {
        // Create the CONSTANT schema objects — pointers never change,
        // only internal values are updated via applyCurrentTheme().
        m_colors     = new Core::Color::ColorSchema(this);
        m_primitives = new Core::Primitives::PrimitiveSchema(this);

        // Apply default theme (cerberus, light mode)
        applyCurrentTheme();
    }

    // ─── Getters ───────────────────────────────────────────────────

    ThemeTypes ThemeManager::currentTheme() const {
        return m_currentTheme;
    }

    QString ThemeManager::currentThemeName() const {
        return Core::Types::ThemeVariant::themeTypeName(m_currentTheme);
    }

    bool ThemeManager::darkMode() const {
        return m_darkMode;
    }

    Core::Color::ColorSchema *ThemeManager::colors() const {
        return m_colors;
    }

    Core::Primitives::PrimitiveSchema *ThemeManager::primitives() const {
        return m_primitives;
    }

    QStringList ThemeManager::availableThemes() const {
        return Core::Types::ThemeVariant::allThemeTypeNames();
    }

    // ─── Setters ───────────────────────────────────────────────────

    void ThemeManager::setCurrentTheme(const ThemeTypes theme) {
        if (m_currentTheme == theme) {
            return;
        }
        m_currentTheme = theme;
        applyCurrentTheme();
        emit currentThemeChanged();
    }

    void ThemeManager::setDarkMode(const bool dark) {
        if (m_darkMode == dark) {
            return;
        }
        m_darkMode = dark;
        applyCurrentTheme();
        emit darkModeChanged();
    }

    // ─── QML convenience methods ───────────────────────────────────

    void ThemeManager::setThemeByName(const QString &name) {
        setCurrentTheme(Core::Types::ThemeVariant::themeTypeFromName(name));
    }

    QString ThemeManager::themeName(const Core::Types::ThemeVariant::ThemeTypes theme) {
        return Core::Types::ThemeVariant::themeTypeName(theme);
    }

    // ─── Internal ──────────────────────────────────────────────────

    void ThemeManager::applyCurrentTheme() {
        Core::Color::ColorFactory::applyTheme(m_currentTheme, m_darkMode, m_colors);
        Core::Primitives::PrimitiveFactory::applyTheme(m_currentTheme, m_primitives);
        emit themeApplied();
    }

} // namespace Nandina::Theme
