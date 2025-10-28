//
// Created by cvrain on 2025/10/24.
//

#ifndef NANDINAUI_THEMEMANAGER_HPP
#define NANDINAUI_THEMEMANAGER_HPP

#include <QQmlEngine>
#include <QColor>
#include <qqmlintegration.h>

#include <map>

#include "palette.hpp"
#include "baseColors.hpp"

namespace NandinaUI {
    class ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        //Expose the palette as a property
        Q_PROPERTY(NandinaUI::Palette* palette READ getPalette NOTIFY paletteChanged)
        //Expose base colors
        Q_PROPERTY(NandinaUI::BaseColors* color READ getColor NOTIFY paletteChanged)

    public:
        static ThemeManager* create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine);

        static ThemeManager* getInstance();

        Q_INVOKABLE PaletteType getCurrentPaletteType() const;

        Q_INVOKABLE void setCurrentPaletteType(PaletteType type);

        Q_INVOKABLE NandinaUI::Palette* getPalette();
        
        Q_INVOKABLE NandinaUI::BaseColors* getColor();

    signals:
        void paletteChanged(PaletteType type);

    private:
        explicit ThemeManager(QObject *parent = nullptr);

        static ThemeManager *instance;
        PaletteType currentPaletteType;
        std::map<PaletteType, Palette> palettes;
        std::map<PaletteType, BaseColors> baseColors;
        BaseColors* currentBaseColors;
    };
}


#endif //NANDINAUI_THEMEMANAGER_HPP