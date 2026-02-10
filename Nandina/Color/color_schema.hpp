#ifndef NANDINA_COLOR_SCHEMA_HPP
#define NANDINA_COLOR_SCHEMA_HPP

#include <QColor>
#include <QObject>
#include <QQmlEngine>

namespace Nandina::NandinaColor {
    Q_NAMESPACE
    QML_ELEMENT

    enum class PaletteType { Latte, Frappe, Macchiato, Mocha, Custom };
    Q_ENUM_NS(PaletteType)

    struct ColorCollection {
        Q_GADGET
        QML_ELEMENT
        QML_NAMED_ELEMENT(colorCollection)

        Q_PROPERTY(PaletteType type MEMBER type)
        Q_PROPERTY(QColor rosewater MEMBER rosewater)
        Q_PROPERTY(QColor flamingo MEMBER flamingo)
        Q_PROPERTY(QColor pink MEMBER pink)
        Q_PROPERTY(QColor mauve MEMBER mauve)
        Q_PROPERTY(QColor red MEMBER red)
        Q_PROPERTY(QColor maroon MEMBER maroon)
        Q_PROPERTY(QColor peach MEMBER peach)
        Q_PROPERTY(QColor yellow MEMBER yellow)
        Q_PROPERTY(QColor green MEMBER green)
        Q_PROPERTY(QColor teal MEMBER teal)
        Q_PROPERTY(QColor sky MEMBER sky)
        Q_PROPERTY(QColor sapphire MEMBER sapphire)
        Q_PROPERTY(QColor blue MEMBER blue)
        Q_PROPERTY(QColor lavender MEMBER lavender)
        Q_PROPERTY(QColor text MEMBER text)
        Q_PROPERTY(QColor subtext1 MEMBER subtext1)
        Q_PROPERTY(QColor subtext0 MEMBER subtext0)
        Q_PROPERTY(QColor overlay2 MEMBER overlay2)
        Q_PROPERTY(QColor overlay1 MEMBER overlay1)
        Q_PROPERTY(QColor overlay0 MEMBER overlay0)
        Q_PROPERTY(QColor surface2 MEMBER surface2)
        Q_PROPERTY(QColor surface1 MEMBER surface1)
        Q_PROPERTY(QColor surface0 MEMBER surface0)
        Q_PROPERTY(QColor base MEMBER base)
        Q_PROPERTY(QColor mantle MEMBER mantle)
        Q_PROPERTY(QColor crust MEMBER crust)
    public:
        PaletteType type{PaletteType::Custom};
        QColor rosewater;
        QColor flamingo;
        QColor pink;
        QColor mauve;
        QColor red;
        QColor maroon;
        QColor peach;
        QColor yellow;
        QColor green;
        QColor teal;
        QColor sky;
        QColor sapphire;
        QColor blue;
        QColor lavender;
        QColor text;
        QColor subtext1;
        QColor subtext0;
        QColor overlay2;
        QColor overlay1;
        QColor overlay0;
        QColor surface2;
        QColor surface1;
        QColor surface0;
        QColor base;
        QColor mantle;
        QColor crust;
    };

    struct PaletteCollection {
        Q_GADGET
        QML_ELEMENT
        QML_NAMED_ELEMENT(paletteCollection)

        Q_PROPERTY(QColor backgroundPane MEMBER backgroundPane)
        Q_PROPERTY(QColor secondaryPane MEMBER secondaryPane)
        Q_PROPERTY(QColor surfaceElement0 MEMBER surfaceElement0)
        Q_PROPERTY(QColor surfaceElement2 MEMBER surfaceElement2)
        Q_PROPERTY(QColor surfaceElement1 MEMBER surfaceElement1)
        Q_PROPERTY(QColor overlay0 MEMBER overlay0)
        Q_PROPERTY(QColor overlay1 MEMBER overlay1)
        Q_PROPERTY(QColor overlay2 MEMBER overlay2)
        Q_PROPERTY(QColor bodyCopy MEMBER bodyCopy)
        Q_PROPERTY(QColor mainHeadline MEMBER mainHeadline)
        Q_PROPERTY(QColor subHeadlines0 MEMBER subHeadlines0)
        Q_PROPERTY(QColor subHeadlines1 MEMBER subHeadlines1)
        Q_PROPERTY(QColor subtle MEMBER subtle)
        Q_PROPERTY(QColor onAccent MEMBER onAccent)
        Q_PROPERTY(QColor links MEMBER links)
        Q_PROPERTY(QColor success MEMBER success)
        Q_PROPERTY(QColor warning MEMBER warning)
        Q_PROPERTY(QColor error MEMBER error)
        Q_PROPERTY(QColor tags MEMBER tags)
        Q_PROPERTY(QColor selectionBackgro MEMBER selectionBackground);
        Q_PROPERTY(QColor cursor MEMBER cursor)
        Q_PROPERTY(QColor cursorText MEMBER cursorText)
        Q_PROPERTY(QColor activeBorder MEMBER activeBorder)
        Q_PROPERTY(QColor inactiveBorder MEMBER inactiveBorder)
        Q_PROPERTY(QColor bellBorder MEMBER bellBorder)
        Q_PROPERTY(QColor color0 MEMBER color0)
        Q_PROPERTY(QColor color1 MEMBER color1)
        Q_PROPERTY(QColor color2 MEMBER color2)
        Q_PROPERTY(QColor color3 MEMBER color3)
        Q_PROPERTY(QColor color4 MEMBER color4)
        Q_PROPERTY(QColor color5 MEMBER color5)
        Q_PROPERTY(QColor color6 MEMBER color6)
        Q_PROPERTY(QColor color7 MEMBER color7)
        Q_PROPERTY(QColor color8 MEMBER color8)
        Q_PROPERTY(QColor color9 MEMBER color9)
        Q_PROPERTY(QColor color10 MEMBER color10)
        Q_PROPERTY(QColor color11 MEMBER color11)
        Q_PROPERTY(QColor color12 MEMBER color12)
        Q_PROPERTY(QColor color13 MEMBER color13)
        Q_PROPERTY(QColor color14 MEMBER color14)
        Q_PROPERTY(QColor color15 MEMBER color15)
        Q_PROPERTY(QColor color16 MEMBER color16)
        Q_PROPERTY(QColor color17 MEMBER color17)
        Q_PROPERTY(QColor mark1 MEMBER mark1)
        Q_PROPERTY(QColor mark2 MEMBER mark2)
        Q_PROPERTY(QColor mark3 MEMBER mark3)
        Q_PROPERTY(QColor mark1Text MEMBER mark1Text)
        Q_PROPERTY(QColor mark2Text MEMBER mark2Text)
        Q_PROPERTY(QColor mark3Text MEMBER mark3Text)
    public:
        QColor backgroundPane;
        QColor secondaryPane;
        QColor surfaceElement0;
        QColor surfaceElement2;
        QColor surfaceElement1;
        QColor overlay0;
        QColor overlay1;
        QColor overlay2;

        QColor bodyCopy;
        QColor mainHeadline;
        QColor subHeadlines0;
        QColor subHeadlines1;
        QColor subtle;
        QColor onAccent;
        QColor links;
        QColor success;
        QColor warning;
        QColor error;
        QColor tags;
        QColor selectionBackground;
        QColor cursor;

        QColor cursorText;
        QColor activeBorder;
        QColor inactiveBorder;
        QColor bellBorder;

        QColor color0;
        QColor color1;
        QColor color2;
        QColor color3;
        QColor color4;
        QColor color5;
        QColor color6;
        QColor color7;
        QColor color8;
        QColor color9;
        QColor color10;
        QColor color11;
        QColor color12;
        QColor color13;
        QColor color14;
        QColor color15;
        QColor color16;
        QColor color17;

        QColor mark1;
        QColor mark2;
        QColor mark3;
        QColor mark1Text;
        QColor mark2Text;
        QColor mark3Text;
    };
} // namespace Nandina::NandinaColor

#endif // NANDINA_COLOR_SCHEMA_HPP
