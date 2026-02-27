#ifndef NANDINA_NAN_STYLE_HPP
#define NANDINA_NAN_STYLE_HPP

#include <QFont>
#include <QObject>
#include <qqml.h>

#include "theme_manager.hpp"

namespace Nandina::NandinaTheme {
    class NanStyle : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("NanStyle is an attached property type")
        QML_ATTACHED(NanStyle)

        Q_PROPERTY(Nandina::NandinaTheme::ThemeManager *themeManager READ themeManager WRITE setThemeManager NOTIFY
                           themeManagerChanged)
        Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)

    public:
        explicit NanStyle(QObject *target = nullptr);

        static NanStyle *qmlAttachedProperties(QObject *object);

        ThemeManager *themeManager() const;
        void setThemeManager(ThemeManager *themeManager);

        QFont font() const;
        void setFont(const QFont &font);

    signals:
        void themeManagerChanged();
        void fontChanged();

    private:
        QObject *targetObject{nullptr};
        ThemeManager *explicitThemeManager{nullptr};
        QFont explicitFont;
        bool hasExplicitFont{false};

        ThemeManager *resolveFromParents() const;
        QFont resolveFontFromParents() const;
    };
} // namespace Nandina::NandinaTheme

#endif // NANDINA_NAN_STYLE_HPP
