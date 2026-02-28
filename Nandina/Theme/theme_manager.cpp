//
// Created by cvrain on 2026/2/28.
//

#include "theme_manager.hpp"
#include "theme_presets.hpp"

#include <QDebug>

namespace Nandina::Theme {

    ThemeManager *ThemeManager::s_instance = nullptr;

    ThemeManager::ThemeManager(QObject *parent) :
        QObject(parent), m_colors(new Core::Color::ColorSchema(this)),
        m_primitives(new Core::Primitives::PrimitiveSchema(this)) {
        if (!s_instance) {
            s_instance = this;
        }
        applyTheme();
    }

    ThemeManager *ThemeManager::instance() {
        if (!s_instance) {
            s_instance = new ThemeManager();
        }
        return s_instance;
    }

    ThemeManager *ThemeManager::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
        Q_UNUSED(jsEngine)

        auto *inst = instance();
        if (inst->parent() == nullptr) {
            QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        }
        return inst;
    }

    QStringList ThemeManager::availableThemes() const {
        return Nandina::Types::allThemePresetNames();
    }

    void ThemeManager::setCurrentTheme(Nandina::Types::ThemePreset theme) {
        if (m_currentTheme == theme)
            return;
        m_currentTheme = theme;
        applyTheme();
        emit currentThemeChanged();
    }

    void ThemeManager::setDarkMode(bool dark) {
        if (m_darkMode == dark)
            return;
        m_darkMode = dark;
        emit darkModeChanged();
    }

    void ThemeManager::setThemeByName(const QString &name) {
        setCurrentTheme(Nandina::Types::themePresetFromName(name));
    }

    QString ThemeManager::themeName(Nandina::Types::ThemePreset preset) const {
        return Nandina::Types::themePresetName(preset);
    }

    QColor ThemeManager::resolveBodyBackground() const {
        return m_darkMode ? m_primitives->bodyBackgroundColorDark() : m_primitives->bodyBackgroundColor();
    }

    void ThemeManager::applyTheme() {
        if (!Core::Tokens::applyPreset(m_currentTheme, m_colors, m_primitives)) {
            qWarning() << "ThemeManager: Unknown theme preset:" << Nandina::Types::themePresetName(m_currentTheme);
            return;
        }
        emit themeApplied();
    }

} // namespace Nandina::Theme
