#include "nan_style.hpp"

#include <QQmlEngine>
#include <QVariant>

namespace Nandina::NandinaTheme {
    NanStyle::NanStyle(QObject *target) : QObject(target), targetObject(target) {
    }

    NanStyle *NanStyle::qmlAttachedProperties(QObject *object) {
        return new NanStyle(object);
    }

    ThemeManager *NanStyle::themeManager() const {
        if (explicitThemeManager)
            return explicitThemeManager;

        return resolveFromParents();
    }

    void NanStyle::setThemeManager(ThemeManager *themeManager) {
        if (explicitThemeManager == themeManager)
            return;

        explicitThemeManager = themeManager;
        emit themeManagerChanged();
    }

    QFont NanStyle::font() const {
        if (hasExplicitFont)
            return explicitFont;

        return resolveFontFromParents();
    }

    void NanStyle::setFont(const QFont &font) {
        if (hasExplicitFont && explicitFont == font)
            return;

        explicitFont = font;
        hasExplicitFont = true;
        emit fontChanged();
    }

    ThemeManager *NanStyle::resolveFromParents() const {
        QObject *current = targetObject ? targetObject->parent() : nullptr;

        while (current) {
            auto *attached = qobject_cast<NanStyle *>(qmlAttachedPropertiesObject<NanStyle>(current, false));
            if (attached) {
                auto *candidate = attached->themeManager();
                if (candidate)
                    return candidate;
            }

            const QVariant directThemeManager = current->property("themeManager");
            if (directThemeManager.isValid()) {
                auto *candidate = qobject_cast<ThemeManager *>(directThemeManager.value<QObject *>());
                if (candidate)
                    return candidate;
            }

            const QVariant resolvedThemeManager = current->property("resolvedThemeManager");
            if (resolvedThemeManager.isValid()) {
                auto *candidate = qobject_cast<ThemeManager *>(resolvedThemeManager.value<QObject *>());
                if (candidate)
                    return candidate;
            }

            current = current->parent();
        }

        return nullptr;
    }

    QFont NanStyle::resolveFontFromParents() const {
        QObject *current = targetObject ? targetObject->parent() : nullptr;

        while (current) {
            auto *attached = qobject_cast<NanStyle *>(qmlAttachedPropertiesObject<NanStyle>(current, false));
            if (attached) {
                const QFont candidate = attached->font();
                if (!candidate.family().isEmpty() || candidate.pixelSize() > 0 || candidate.pointSizeF() > 0)
                    return candidate;
            }

            const QVariant directFont = current->property("font");
            if (directFont.isValid() && directFont.canConvert<QFont>()) {
                const QFont candidate = directFont.value<QFont>();
                if (!candidate.family().isEmpty() || candidate.pixelSize() > 0 || candidate.pointSizeF() > 0)
                    return candidate;
            }

            const QVariant textFont = current->property("textFont");
            if (textFont.isValid() && textFont.canConvert<QFont>()) {
                const QFont candidate = textFont.value<QFont>();
                if (!candidate.family().isEmpty() || candidate.pixelSize() > 0 || candidate.pointSizeF() > 0)
                    return candidate;
            }

            current = current->parent();
        }

        return QFont();
    }
} // namespace Nandina::NandinaTheme
