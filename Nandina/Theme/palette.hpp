//
// Created by cvrain on 2025/10/24.
//

#ifndef NANDINAUI_PALETTE_HPP
#define NANDINAUI_PALETTE_HPP

#include <QObject>
#include <qqmlintegration.h>
#include <QString>

namespace Nandina::Color::Latte {
    inline QString Rosewater = "#dc8a78";
    inline QString Flamingo = "#dd7878";
    inline QString Pink = "#ea76cb";
    inline QString Mauve = "#8839ef";
    inline QString Red = "#d20f39";
    inline QString Maroon = "#e64553";
    inline QString Peach = "#fe640b";
    inline QString Yellow = "#df8e1d";
    inline QString Green = "#40a02b";
    inline QString Teal = "#179299";
    inline QString Sky = "#04a5e5";
    inline QString Sapphire = "#209fb5";
    inline QString Blue = "#1e66f5";
    inline QString Lavender = "#7287fd";
    inline QString Text = "#4c4f69";
    inline QString Subtext1 = "#5c5f77";
    inline QString Subtext0 = "#6c6f85";
    inline QString Overlay2 = "#7c7f93";
    inline QString Overlay1 = "#8c8fa1";
    inline QString Overlay0 = "#9ca0b0";
    inline QString Surface2 = "#acb0be";
    inline QString Surface1 = "#bcc0cc";
    inline QString Surface0 = "#ccd0da";
    inline QString Base = "#eff1f5";
    inline QString Mantle = "#e6e9ef";
    inline QString Crust = "#dce0e8";
}

namespace Nandina::Color::Frappe {
    inline QString Rosewater = "#f2d5cf";
    inline QString Flamingo = "#eebebe";
    inline QString Pink = "#f4b8e4";
    inline QString Mauve = "#ca9ee6";
    inline QString Red = "#e78284";
    inline QString Maroon = "#ea999c";
    inline QString Peach = "#ef9f76";
    inline QString Yellow = "#e5c890";
    inline QString Green = "#a6d189";
    inline QString Teal = "#81c8be";
    inline QString Sky = "#99d1db";
    inline QString Sapphire = "#85c1dc";
    inline QString Blue = "#8caaee";
    inline QString Lavender = "#babbf1";
    inline QString Text = "#c6d0f5";
    inline QString Subtext1 = "#b5bfe2";
    inline QString Subtext0 = "#a5adce";
    inline QString Overlay2 = "#949cbb";
    inline QString Overlay1 = "#838ba7";
    inline QString Overlay0 = "#737994";
    inline QString Surface2 = "#626880";
    inline QString Surface1 = "#51576d";
    inline QString Surface0 = "#414559";
    inline QString Base = "#303446";
    inline QString Mantle = "#292c3c";
    inline QString Crust = "#232634";
}

namespace Nandina::Color::Macchiato {
    inline QString Rosewater = "#f4dbd6";
    inline QString Flamingo = "#f0c6c6";
    inline QString Pink = "#f5bde6";
    inline QString Mauve = "#c6a0f6";
    inline QString Red = "#ed8796";
    inline QString Maroon = "#ee99a0";
    inline QString Peach = "#f5a97f";
    inline QString Yellow = "#eed49f";
    inline QString Green = "#a6da95";
    inline QString Teal = "#8bd5ca";
    inline QString Sky = "#91d7e3";
    inline QString Sapphire = "#7dc4e4";
    inline QString Blue = "#8aadf4";
    inline QString Lavender = "#b7bdf8";
    inline QString Text = "#cad3f5";
    inline QString Subtext1 = "#b8c0e0";
    inline QString Subtext0 = "#a5adcb";
    inline QString Overlay2 = "#939ab7";
    inline QString Overlay1 = "#8087a2";
    inline QString Overlay0 = "#6e738d";
    inline QString Surface2 = "#5b6078";
    inline QString Surface1 = "#494d64";
    inline QString Surface0 = "#363a4f";
    inline QString Base = "#24273a";
    inline QString Mantle = "#1e2030";
    inline QString Crust = "#181926";
}

namespace Nandina::Color::Mocha {
    inline QString Rosewater = "#f5e0dc";
    inline QString Flamingo = "#f2cdcd";
    inline QString Pink = "#f5c2e7";
    inline QString Mauve = "#cba6f7";
    inline QString Red = "#f38ba8";
    inline QString Maroon = "#eba0ac";
    inline QString Peach = "#fab387";
    inline QString Yellow = "#f9e2af";
    inline QString Green = "#a6e3a1";
    inline QString Teal = "#94e2d5";
    inline QString Sky = "#89dceb";
    inline QString Sapphire = "#74c7ec";
    inline QString Blue = "#89b4fa";
    inline QString Lavender = "#b4befe";
    inline QString Text = "#cdd6f4";
    inline QString Subtext1 = "#bac2de";
    inline QString Subtext0 = "#a6adc8";
    inline QString Overlay2 = "#9399b2";
    inline QString Overlay1 = "#7f849c";
    inline QString Overlay0 = "#6c7086";
    inline QString Surface2 = "#585b70";
    inline QString Surface1 = "#45475a";
    inline QString Surface0 = "#313244";
    inline QString Base = "#1e1e2e";
    inline QString Mantle = "#181825";
    inline QString Crust = "#11111b";
}

namespace Nandina {
    Q_NAMESPACE

    enum class PaletteType {
        Latte,
        Frappe,
        Macchiato,
        Mocha
    };

    Q_ENUM_NS(PaletteType)

    class Palette : public QObject {
        Q_OBJECT
        QML_ELEMENT
        
        //Background Colors
        Q_PROPERTY(QString backgroundPane MEMBER backgroundPane CONSTANT)
        Q_PROPERTY(QString secondaryPane MEMBER secondaryPane CONSTANT)
        Q_PROPERTY(QString surfaceElements MEMBER surfaceElements CONSTANT)
        Q_PROPERTY(QString overlays MEMBER overlays CONSTANT)
        
        // Typography
        Q_PROPERTY(QString bodyCopy MEMBER bodyCopy CONSTANT)
        Q_PROPERTY(QString mainHeadline MEMBER mainHeadline CONSTANT)
        Q_PROPERTY(QString subHeadline MEMBER subHeadline CONSTANT)
        Q_PROPERTY(QString label MEMBER label CONSTANT)
        Q_PROPERTY(QString subtle MEMBER subtle CONSTANT)
        Q_PROPERTY(QString onAccent MEMBER onAccent CONSTANT)
        Q_PROPERTY(QString links MEMBER links CONSTANT)
        Q_PROPERTY(QString urls MEMBER urls CONSTANT)
        Q_PROPERTY(QString success MEMBER success CONSTANT)
        Q_PROPERTY(QString warning MEMBER warning CONSTANT)
        Q_PROPERTY(QString error MEMBER error CONSTANT)
        Q_PROPERTY(QString tags MEMBER tags CONSTANT)
        Q_PROPERTY(QString pills MEMBER pills CONSTANT)
        Q_PROPERTY(QString selectionBackground MEMBER selectionBackground CONSTANT)
        Q_PROPERTY(QString cursor MEMBER cursor CONSTANT)
        
        //Window Colors
        Q_PROPERTY(QString cursorText MEMBER cursorText CONSTANT)
        Q_PROPERTY(QString activeBorder MEMBER activeBorder CONSTANT)
        Q_PROPERTY(QString inactiveBorder MEMBER inactiveBorder CONSTANT)
        Q_PROPERTY(QString bellBorder MEMBER bellBorder CONSTANT)
        
        //Syntax Colors
        Q_PROPERTY(QString mark1 MEMBER mark1 CONSTANT)
        Q_PROPERTY(QString mark2 MEMBER mark2 CONSTANT)
        Q_PROPERTY(QString mark3 MEMBER mark3 CONSTANT)
        Q_PROPERTY(QString mark1Text MEMBER mark1Text CONSTANT)
        Q_PROPERTY(QString mark2Text MEMBER mark2Text CONSTANT)
        Q_PROPERTY(QString mark3Text MEMBER mark3Text CONSTANT)

    public:
        explicit Palette(QObject *parent = nullptr);

        Palette(const Palette &);

        Palette& operator=(const Palette &);

        //Background Colors
        QString backgroundPane;
        QString secondaryPane;
        QString surfaceElements;
        QString overlays;
        // Typography
        QString bodyCopy;
        QString mainHeadline;
        QString subHeadline;
        QString label;
        QString subtle;
        QString onAccent;
        QString links;
        QString urls;
        QString success;
        QString warning;
        QString error;
        QString tags;
        QString pills;
        QString selectionBackground;
        QString cursor;
        //Window Colors
        QString cursorText;
        QString activeBorder;
        QString inactiveBorder;
        QString bellBorder;
        //Syntax Colors
        QString mark1;
        QString mark2;
        QString mark3;
        QString mark1Text;
        QString mark2Text;
        QString mark3Text;
    };

    class Tools : public QObject {
        Q_OBJECT

    public:
        explicit Tools(QObject *parent = nullptr);

        static Palette generatePalette(PaletteType type);

        static Palette generateLattePalette();

        static Palette generateFrappePalette();

        static Palette generateMacchiatoPalette();

        static Palette generateMochaPalette();
    };
}

#endif //NANDINAUI_PALETTE_HPP