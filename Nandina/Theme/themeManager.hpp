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

#include "baseColors.hpp"
#include "button_style.hpp"
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

        Q_INVOKABLE Core::Types::CatppuccinSetting::CatppuccinType getCurrentPaletteType() const;

        Q_INVOKABLE void setCurrentPaletteType(Core::Types::CatppuccinSetting::CatppuccinType type);

        Q_INVOKABLE Nandina::BaseColors* getColor() const;

        Q_INVOKABLE Nandina::Theme::Components::NanButtonStyle* getButtonStyle(const QString &type);

    signals:
        void paletteChanged(Core::Types::CatppuccinSetting::CatppuccinType type);
        void stylesLoaded();

    private:
        explicit ThemeManager(QObject *parent = nullptr);

        void loadComponentStyles();
        void loadBaseColor();

        // 解析样式中的颜色变量
        QString resolveColorVariable(const QString &value) const;
        
        // 递归解析JSON对象中的所有颜色变量
        QJsonObject resolveStyleColors(const QJsonObject &styleObject) const;

        static ThemeManager *instance;
        Core::Types::CatppuccinSetting::CatppuccinType currentPaletteType;

        std::map<Core::Types::CatppuccinSetting::CatppuccinType, BaseColors> baseColors;
        std::map<QString, Theme::Components::NanButtonStyle> buttonStyles;


        BaseColors *currentBaseColors;
    };
}


#endif //NANDINAUI_THEMEMANAGER_HPP
