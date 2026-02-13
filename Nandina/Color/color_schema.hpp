#ifndef NANDINA_COLOR_SCHEMA_HPP
#define NANDINA_COLOR_SCHEMA_HPP

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QtGlobal>

#if defined(_WIN32)
#if defined(NandinaColor_EXPORTS)
#define NANDINA_COLOR_EXPORT Q_DECL_EXPORT
#else
#define NANDINA_COLOR_EXPORT Q_DECL_IMPORT
#endif
#else
#define NANDINA_COLOR_EXPORT
#endif

namespace Nandina::NandinaColor {
    class NANDINA_COLOR_EXPORT PaletteEnum : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(NandinaColor)
        QML_UNCREATABLE("NandinaColor is an enum container")

    public:
        enum PaletteType { Latte, Frappe, Macchiato, Mocha, Custom };
        Q_ENUM(PaletteType)
    };

    using PaletteType = PaletteEnum::PaletteType;

    class NANDINA_COLOR_EXPORT ColorCollection : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(PaletteType type MEMBER type NOTIFY changed)
        Q_PROPERTY(QColor rosewater MEMBER rosewater NOTIFY changed)
        Q_PROPERTY(QColor flamingo MEMBER flamingo NOTIFY changed)
        Q_PROPERTY(QColor pink MEMBER pink NOTIFY changed)
        Q_PROPERTY(QColor mauve MEMBER mauve NOTIFY changed)
        Q_PROPERTY(QColor red MEMBER red NOTIFY changed)
        Q_PROPERTY(QColor maroon MEMBER maroon NOTIFY changed)
        Q_PROPERTY(QColor peach MEMBER peach NOTIFY changed)
        Q_PROPERTY(QColor yellow MEMBER yellow NOTIFY changed)
        Q_PROPERTY(QColor green MEMBER green NOTIFY changed)
        Q_PROPERTY(QColor teal MEMBER teal NOTIFY changed)
        Q_PROPERTY(QColor sky MEMBER sky NOTIFY changed)
        Q_PROPERTY(QColor sapphire MEMBER sapphire NOTIFY changed)
        Q_PROPERTY(QColor blue MEMBER blue NOTIFY changed)
        Q_PROPERTY(QColor lavender MEMBER lavender NOTIFY changed)
        Q_PROPERTY(QColor text MEMBER text NOTIFY changed)
        Q_PROPERTY(QColor subtext1 MEMBER subtext1 NOTIFY changed)
        Q_PROPERTY(QColor subtext0 MEMBER subtext0 NOTIFY changed)
        Q_PROPERTY(QColor overlay2 MEMBER overlay2 NOTIFY changed)
        Q_PROPERTY(QColor overlay1 MEMBER overlay1 NOTIFY changed)
        Q_PROPERTY(QColor overlay0 MEMBER overlay0 NOTIFY changed)
        Q_PROPERTY(QColor surface2 MEMBER surface2 NOTIFY changed)
        Q_PROPERTY(QColor surface1 MEMBER surface1 NOTIFY changed)
        Q_PROPERTY(QColor surface0 MEMBER surface0 NOTIFY changed)
        Q_PROPERTY(QColor base MEMBER base NOTIFY changed)
        Q_PROPERTY(QColor mantle MEMBER mantle NOTIFY changed)
        Q_PROPERTY(QColor crust MEMBER crust NOTIFY changed)
    signals:
        void changed();

    public:
        explicit ColorCollection(QObject *parent = nullptr) : QObject(parent) {
        }

        explicit ColorCollection(const ColorCollection *collection, QObject *parent = nullptr) :
            ColorCollection(*collection, parent) {
        }

        ColorCollection(const ColorCollection &collection, QObject *parent = nullptr) :
            QObject(parent), rosewater(collection.rosewater), flamingo(collection.flamingo), pink(collection.pink),
            mauve(collection.mauve), red(collection.red), maroon(collection.maroon), peach(collection.peach),
            yellow(collection.yellow), green(collection.green), teal(collection.teal), sky(collection.sky),
            sapphire(collection.sapphire), blue(collection.blue), lavender(collection.lavender), text(collection.text),
            subtext1(collection.subtext1), subtext0(collection.subtext0), overlay2(collection.overlay2),
            overlay1(collection.overlay1), overlay0(collection.overlay0), surface2(collection.surface2),
            surface1(collection.surface1), surface0(collection.surface0), base(collection.base),
            mantle(collection.mantle), crust(collection.crust) {
        }

        ColorCollection &operator=(const ColorCollection &) = delete;

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

    class NANDINA_COLOR_EXPORT PaletteCollection : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QColor backgroundPane MEMBER backgroundPane NOTIFY changed)
        Q_PROPERTY(QColor secondaryPane MEMBER secondaryPane NOTIFY changed)
        Q_PROPERTY(QColor surfaceElement0 MEMBER surfaceElement0 NOTIFY changed)
        Q_PROPERTY(QColor surfaceElement2 MEMBER surfaceElement2 NOTIFY changed)
        Q_PROPERTY(QColor surfaceElement1 MEMBER surfaceElement1 NOTIFY changed)
        Q_PROPERTY(QColor overlay0 MEMBER overlay0 NOTIFY changed)
        Q_PROPERTY(QColor overlay1 MEMBER overlay1 NOTIFY changed)
        Q_PROPERTY(QColor overlay2 MEMBER overlay2 NOTIFY changed)
        Q_PROPERTY(QColor bodyCopy MEMBER bodyCopy NOTIFY changed)
        Q_PROPERTY(QColor mainHeadline MEMBER mainHeadline NOTIFY changed)
        Q_PROPERTY(QColor subHeadlines0 MEMBER subHeadlines0 NOTIFY changed)
        Q_PROPERTY(QColor subHeadlines1 MEMBER subHeadlines1 NOTIFY changed)
        Q_PROPERTY(QColor subtle MEMBER subtle NOTIFY changed)
        Q_PROPERTY(QColor onAccent MEMBER onAccent NOTIFY changed)
        Q_PROPERTY(QColor links MEMBER links NOTIFY changed)
        Q_PROPERTY(QColor success MEMBER success NOTIFY changed)
        Q_PROPERTY(QColor warning MEMBER warning NOTIFY changed)
        Q_PROPERTY(QColor error MEMBER error NOTIFY changed)
        Q_PROPERTY(QColor tags MEMBER tags NOTIFY changed)
        Q_PROPERTY(QColor selectionBackground MEMBER selectionBackground NOTIFY changed)
        Q_PROPERTY(QColor cursor MEMBER cursor NOTIFY changed)
        Q_PROPERTY(QColor cursorText MEMBER cursorText NOTIFY changed)
        Q_PROPERTY(QColor activeBorder MEMBER activeBorder NOTIFY changed)
        Q_PROPERTY(QColor inactiveBorder MEMBER inactiveBorder NOTIFY changed)
        Q_PROPERTY(QColor bellBorder MEMBER bellBorder NOTIFY changed)
        Q_PROPERTY(QColor color0 MEMBER color0 NOTIFY changed)
        Q_PROPERTY(QColor color1 MEMBER color1 NOTIFY changed)
        Q_PROPERTY(QColor color2 MEMBER color2 NOTIFY changed)
        Q_PROPERTY(QColor color3 MEMBER color3 NOTIFY changed)
        Q_PROPERTY(QColor color4 MEMBER color4 NOTIFY changed)
        Q_PROPERTY(QColor color5 MEMBER color5 NOTIFY changed)
        Q_PROPERTY(QColor color6 MEMBER color6 NOTIFY changed)
        Q_PROPERTY(QColor color7 MEMBER color7 NOTIFY changed)
        Q_PROPERTY(QColor color8 MEMBER color8 NOTIFY changed)
        Q_PROPERTY(QColor color9 MEMBER color9 NOTIFY changed)
        Q_PROPERTY(QColor color10 MEMBER color10 NOTIFY changed)
        Q_PROPERTY(QColor color11 MEMBER color11 NOTIFY changed)
        Q_PROPERTY(QColor color12 MEMBER color12 NOTIFY changed)
        Q_PROPERTY(QColor color13 MEMBER color13 NOTIFY changed)
        Q_PROPERTY(QColor color14 MEMBER color14 NOTIFY changed)
        Q_PROPERTY(QColor color15 MEMBER color15 NOTIFY changed)
        Q_PROPERTY(QColor color16 MEMBER color16 NOTIFY changed)
        Q_PROPERTY(QColor color17 MEMBER color17 NOTIFY changed)
        Q_PROPERTY(QColor mark1 MEMBER mark1 NOTIFY changed)
        Q_PROPERTY(QColor mark2 MEMBER mark2 NOTIFY changed)
        Q_PROPERTY(QColor mark3 MEMBER mark3 NOTIFY changed)
        Q_PROPERTY(QColor mark1Text MEMBER mark1Text NOTIFY changed)
        Q_PROPERTY(QColor mark2Text MEMBER mark2Text NOTIFY changed)
        Q_PROPERTY(QColor mark3Text MEMBER mark3Text NOTIFY changed)
    signals:
        void changed();

    public:
        explicit PaletteCollection(QObject *parent = nullptr) : QObject{parent} {
        }

        explicit PaletteCollection(const PaletteCollection *collection, QObject *parent = nullptr) :
            PaletteCollection{*collection, parent} {
        }

        PaletteCollection(const PaletteCollection &collection, QObject *parent = nullptr) :
            QObject(parent), backgroundPane(collection.backgroundPane), secondaryPane(collection.secondaryPane),
            surfaceElement0(collection.surfaceElement0), surfaceElement2(collection.surfaceElement2),
            surfaceElement1(collection.surfaceElement1), overlay0(collection.overlay0), overlay1(collection.overlay1),
            overlay2(collection.overlay2), bodyCopy(collection.bodyCopy), mainHeadline(collection.mainHeadline),
            subHeadlines0(collection.subHeadlines0), subHeadlines1(collection.subHeadlines1), subtle(collection.subtle),
            onAccent(collection.onAccent), links(collection.links), success(collection.success),
            warning(collection.warning), error(collection.error), tags(collection.tags),
            selectionBackground(collection.selectionBackground), cursor(collection.cursor),
            cursorText(collection.cursorText), activeBorder(collection.activeBorder),
            inactiveBorder(collection.inactiveBorder), bellBorder(collection.bellBorder), color0(collection.color0),
            color1(collection.color1), color2(collection.color2), color3(collection.color3), color4(collection.color4),
            color5(collection.color5), color6(collection.color6), color7(collection.color7), color8(collection.color8),
            color9(collection.color9), color10(collection.color10), color11(collection.color11),
            color12(collection.color12), color13(collection.color13), color14(collection.color14),
            color15(collection.color15), color16(collection.color16), color17(collection.color17),
            mark1(collection.mark1), mark2(collection.mark2), mark3(collection.mark3), mark1Text(collection.mark1Text),
            mark2Text(collection.mark2Text), mark3Text(collection.mark3Text) {
        }

        PaletteCollection &operator=(const PaletteCollection &) = delete;

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
