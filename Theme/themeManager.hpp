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

namespace NandinaUI {
    class ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        //Q_PROPERTY(type name READ name WRITE setName NOTIFY nameChanged FINAL)

        Q_PROPERTY(QColor backgroundPane READ getColorBackgroundPane);
        // Q_PROPERTY(QString secondaryPane);
        // Q_PROPERTY(QString surfaceElements);
        // Q_PROPERTY(QString overlays);
        // Q_PROPERTY(QString bodyCopy);
        // Q_PROPERTY(QString mainHeadline);
        // Q_PROPERTY(QString subHeadline);
        // Q_PROPERTY(QString label);
        // Q_PROPERTY(QString subtle);
        // Q_PROPERTY(QString onAccent);
        // Q_PROPERTY(QString links);
        // Q_PROPERTY(QString urls);
         Q_PROPERTY(QColor success READ getColorSuccess);
         Q_PROPERTY(QColor warning READ getColorWarning);
         Q_PROPERTY(QColor error READ getColorError);
        // Q_PROPERTY(QString tags);
        // Q_PROPERTY(QString pills);
        // Q_PROPERTY(QString selectionBackground);
        // Q_PROPERTY(QString cursor);
        // Q_PROPERTY(QString cursorText);
        // Q_PROPERTY(QString activeBorder);
        // Q_PROPERTY(QString inactiveBorder);
        // Q_PROPERTY(QString bellBorder);
        // Q_PROPERTY(QString mark1);
        // Q_PROPERTY(QString mark2);
        // Q_PROPERTY(QString mark3);
        // Q_PROPERTY(QString mark1Text);
        // Q_PROPERTY(QString mark2Text);
        // Q_PROPERTY(QString mark3Text);

    public:
        static ThemeManager* create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine);

        static ThemeManager* getInstance();

        const Palette& getCurrentPalette();

        Q_INVOKABLE PaletteType getCurrentPaletteType() const;

        Q_INVOKABLE void setCurrentPaletteType(PaletteType type);

        Q_INVOKABLE QColor getColorBackgroundPane() const;

        Q_INVOKABLE QColor getColorSuccess() const;

        Q_INVOKABLE QColor getColorWarning() const;

        Q_INVOKABLE QColor getColorError() const;

    signals:
        void paletteChanged(const PaletteType  type);

    private:
        explicit ThemeManager(QObject *parent = nullptr);

        static ThemeManager *instance;
        PaletteType currentPaletteType;
        std::map<PaletteType, Palette> palettes;
    };
}


#endif //NANDINAUI_THEMEMANAGER_HPP
