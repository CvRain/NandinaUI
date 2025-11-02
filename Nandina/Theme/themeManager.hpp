//
// Created by cvrain on 2025/10/24.
//

#ifndef NANDINAUI_THEMEMANAGER_HPP
#define NANDINAUI_THEMEMANAGER_HPP

#include <QQmlEngine>
#include <QColor>
#include <qqmlintegration.h>

#include <map>
#include <QJsonObject>


#include "Core/Types/colorSet.hpp"
#include "baseColors.hpp"

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

        Q_INVOKABLE ColorSet::CatppuccinType getCurrentPaletteType() const;

        Q_INVOKABLE void setCurrentPaletteType(ColorSet::CatppuccinType type);

        Q_INVOKABLE Nandina::BaseColors* getColor() const;

        //Q_INVOKABLE QVariant getComponentStyle(const QString& stylePath) const;
        template <typename T>
        T getComponentStyle() const;

    signals:
        void paletteChanged(ColorSet::CatppuccinType type);

    private:
        explicit ThemeManager(QObject *parent = nullptr);
        void loadComponentStyles();

        static ThemeManager *instance;
        ColorSet::CatppuccinType currentPaletteType;
        std::map<ColorSet::CatppuccinType, BaseColors> baseColors;
        BaseColors* currentBaseColors;
        QJsonObject componentStyles;
    };
}


#endif //NANDINAUI_THEMEMANAGER_HPP
