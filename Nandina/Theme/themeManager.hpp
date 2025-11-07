//
// Created by cvrain on 2025/10/24.
//

#ifndef NANDINAUI_THEMEMANAGER_HPP
#define NANDINAUI_THEMEMANAGER_HPP

#include <QQmlEngine>
#include <QColor>
#include <qqmlintegration.h>

#include <map>

#include "baseColors.hpp"

#include "colorCollection.hpp"

namespace Nandina {
    class ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        //Expose base colors
        Q_PROPERTY(Nandina::BaseColors* color READ getColor NOTIFY paletteChanged)

    public:
        static ThemeManager* create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine);

        static ThemeManager* getInstance();

        QString getColorByString(const QString &string) const;

        Q_INVOKABLE [[nodiscard]] Core::Types::CatppuccinSetting::CatppuccinType getCurrentPaletteType() const;

        Q_INVOKABLE void setCurrentPaletteType(Core::Types::CatppuccinSetting::CatppuccinType type);

        Q_INVOKABLE [[nodiscard]] Nandina::BaseColors* getColor() const;

    signals:
        void paletteChanged(Core::Types::CatppuccinSetting::CatppuccinType type);

        void stylesLoaded();

    private:
        explicit ThemeManager(QObject *parent = nullptr);

        void loadBaseColor();

        static ThemeManager *instance;
        Core::Types::CatppuccinSetting::CatppuccinType currentPaletteType;

        std::map<Core::Types::CatppuccinSetting::CatppuccinType, BaseColors> baseColors;

        BaseColors *currentBaseColors;
    };
}


#endif //NANDINAUI_THEMEMANAGER_HPP
