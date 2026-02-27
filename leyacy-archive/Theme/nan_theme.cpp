#include "nan_theme.hpp"

namespace Nandina::NandinaTheme {
    NanTheme::NanTheme(QObject *parent) : QObject(parent) {
        defaultThemeManager = new ThemeManager(this);

        defaultFont.setPixelSize(14);
        defaultFont.setWeight(QFont::Normal);
    }

    NanTheme *NanTheme::instance() {
        static NanTheme *singleton = new NanTheme();
        return singleton;
    }

    ThemeManager *NanTheme::themeManager() const {
        return defaultThemeManager;
    }

    QFont NanTheme::font() const {
        return defaultFont;
    }

    void NanTheme::setFont(const QFont &font) {
        if (defaultFont == font)
            return;

        defaultFont = font;
        emit fontChanged();
    }
} // namespace Nandina::NandinaTheme
