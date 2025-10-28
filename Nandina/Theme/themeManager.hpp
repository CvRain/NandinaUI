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

        Q_PROPERTY(QString backgroundPane READ getColorBackgroundPane NOTIFY paletteChanged)
        Q_PROPERTY(QString secondaryPane READ getColorSecondaryPane NOTIFY paletteChanged)
        Q_PROPERTY(QString surfaceElements READ getColorSurfaceElements NOTIFY paletteChanged)
        Q_PROPERTY(QString overlays READ getColorOverlays NOTIFY paletteChanged)
        Q_PROPERTY(QString bodyCopy READ getColorBodyCopy NOTIFY paletteChanged)
        Q_PROPERTY(QString mainHeadline READ getColorMainHeadline NOTIFY paletteChanged)
        Q_PROPERTY(QString subHeadline READ getColorSubHeadline NOTIFY paletteChanged)
        Q_PROPERTY(QString label READ getColorLabel NOTIFY paletteChanged)
        Q_PROPERTY(QString subtle READ getColorSubtle NOTIFY paletteChanged)
        Q_PROPERTY(QString onAccent READ getColorOnAccent NOTIFY paletteChanged)
        Q_PROPERTY(QString links READ getColorLinks NOTIFY paletteChanged)
        Q_PROPERTY(QString urls READ getColorUrls NOTIFY paletteChanged)
        Q_PROPERTY(QString success READ getColorSuccess NOTIFY paletteChanged)
        Q_PROPERTY(QString warning READ getColorWarning NOTIFY paletteChanged)
        Q_PROPERTY(QString error READ getColorError NOTIFY paletteChanged)
        Q_PROPERTY(QString tags READ getColorTags NOTIFY paletteChanged)
        Q_PROPERTY(QString pills READ getColorPills NOTIFY paletteChanged)
        Q_PROPERTY(QString selectionBackground READ getColorSelectionBackground NOTIFY paletteChanged)
        Q_PROPERTY(QString cursor READ getColorCursor NOTIFY paletteChanged)
        Q_PROPERTY(QString cursorText READ getColorCursorText NOTIFY paletteChanged)
        Q_PROPERTY(QString activeBorder READ getColorActiveBorder NOTIFY paletteChanged)
        Q_PROPERTY(QString inactiveBorder READ getColorInactiveBorder NOTIFY paletteChanged)
        Q_PROPERTY(QString bellBorder READ getColorNellBorder NOTIFY paletteChanged)
        Q_PROPERTY(QString mark1 READ getColorMark1 NOTIFY paletteChanged)
        Q_PROPERTY(QString mark2 READ getColorMark2 NOTIFY paletteChanged)
        Q_PROPERTY(QString mark3 READ getColorMark3 NOTIFY paletteChanged)
        Q_PROPERTY(QString mark1Text READ getColorMark1Text NOTIFY paletteChanged)
        Q_PROPERTY(QString mark2Text READ getColorMark2Text NOTIFY paletteChanged)
        Q_PROPERTY(QString mark3Text READ getColorMark3Text NOTIFY paletteChanged)

    public:
        static ThemeManager* create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine);

        static ThemeManager* getInstance();

        Q_INVOKABLE const Palette& getCurrentPalette();

        Q_INVOKABLE PaletteType getCurrentPaletteType() const;

        Q_INVOKABLE void setCurrentPaletteType(PaletteType type);

        Q_INVOKABLE QString getColorBackgroundPane() const;
        Q_INVOKABLE QString getColorSecondaryPane();
        Q_INVOKABLE QString getColorSurfaceElements();
        Q_INVOKABLE QString getColorOverlays();
        Q_INVOKABLE QString getColorBodyCopy();
        Q_INVOKABLE QString getColorMainHeadline();
        Q_INVOKABLE QString getColorSubHeadline();
        Q_INVOKABLE QString getColorLabel();
        Q_INVOKABLE QString getColorSubtle();
        Q_INVOKABLE QString getColorOnAccent();
        Q_INVOKABLE QString getColorLinks();
        Q_INVOKABLE QString getColorUrls();
        Q_INVOKABLE QString getColorTags();
        Q_INVOKABLE QString getColorPills();
        Q_INVOKABLE QString getColorSelectionBackground();
        Q_INVOKABLE QString getColorCursor();
        Q_INVOKABLE QString getColorCursorText();
        Q_INVOKABLE QString getColorActiveBorder();
        Q_INVOKABLE QString getColorInactiveBorder();
        Q_INVOKABLE QString getColorNellBorder();
        Q_INVOKABLE QString getColorMark1();
        Q_INVOKABLE QString getColorMark2();
        Q_INVOKABLE QString getColorMark3();
        Q_INVOKABLE QString getColorMark1Text();
        Q_INVOKABLE QString getColorMark2Text();
        Q_INVOKABLE QString getColorMark3Text();
        Q_INVOKABLE QString getColorSuccess();
        Q_INVOKABLE QString getColorWarning();
        Q_INVOKABLE QString getColorError();

    signals:
        void paletteChanged(PaletteType type);

    private:
        explicit ThemeManager(QObject *parent = nullptr);

        static ThemeManager *instance;
        PaletteType currentPaletteType;
        std::map<PaletteType, Palette> palettes;
    };
}


#endif //NANDINAUI_THEMEMANAGER_HPP
