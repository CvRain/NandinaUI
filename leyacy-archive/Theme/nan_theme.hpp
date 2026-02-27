#ifndef NANDINA_NAN_THEME_HPP
#define NANDINA_NAN_THEME_HPP

#include <QFont>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "theme_manager.hpp"

namespace Nandina::NandinaTheme {
    class NanTheme : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        Q_PROPERTY(Nandina::NandinaTheme::ThemeManager *themeManager READ themeManager CONSTANT)
        Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)

    public:
        explicit NanTheme(QObject *parent = nullptr);
        NanTheme(const NanTheme &) = delete;
        NanTheme &operator=(const NanTheme &) = delete;

        static NanTheme *instance();

        static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine) {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return instance();
        }

        ThemeManager *themeManager() const;
        QFont font() const;
        void setFont(const QFont &font);

    signals:
        void fontChanged();

    private:
        ThemeManager *defaultThemeManager{nullptr};
        QFont defaultFont;
    };
} // namespace Nandina::NandinaTheme

#endif // NANDINA_NAN_THEME_HPP
