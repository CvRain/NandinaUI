//
// Created by cvrain on 2026/3/1.
//

#include "theme_manager.hpp"

#include "theme_registry.hpp"

namespace Nandina::Theme {

    ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {
        // Load bundled fonts before applying any theme, so TypographySchema
        // font family names (e.g. "LXGW WenKai") resolve correctly.
        Core::Fonts::FontManager::loadBundledFonts();

        // Create the CONSTANT schema objects — pointers never change,
        // only internal values are updated via applyCurrentTheme().
        m_colors = new Core::Color::ColorSchema(this);
        m_primitives = new Core::Primitives::PrimitiveSchema(this);

        // Apply default theme (Aurora, light mode)
        applyCurrentTheme();
    }

    // ─── Getters ───────────────────────────────────────────────────

    ThemeTypes ThemeManager::currentTheme() const {
        return m_currentTheme;
    }

    QString ThemeManager::currentThemeName() const {
        return Core::Types::ThemeRegistry::themeName(m_currentTheme);
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
        return Core::Types::ThemeRegistry::availableThemeNames();
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
        setCurrentTheme(Core::Types::ThemeRegistry::themeFromName(name));
    }

    QString ThemeManager::themeName(const Core::Types::ThemeVariant::ThemeTypes theme) {
        return Core::Types::ThemeRegistry::themeName(theme);
    }

    // ─── Internal ──────────────────────────────────────────────────

    void ThemeManager::applyCurrentTheme() {
        Core::Color::ColorFactory::applyTheme(m_currentTheme, m_darkMode, m_colors);
        Core::Primitives::PrimitiveFactory::applyTheme(m_currentTheme, m_primitives);
        emit themeApplied();
    }

} // namespace Nandina::Theme
