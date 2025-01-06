#pragma once

#include <QQmlEngine>
#include <utility>

struct CatppuccinPalette {
    QString rosewater;
    QString flamingo;
    QString pink;
    QString mauve;
    QString red;
    QString maroon;
    QString peach;
    QString yellow;
    QString green;
    QString teal;
    QString sky;
    QString sapphire;
    QString blue;
    QString lavender;
    QString text;
    QString subtext1;
    QString subtext0;
    QString overlay2;
    QString overlay1;
    QString overlay0;
    QString surface2;
    QString surface1;
    QString surface0;
    QString base;
    QString mantle;
    QString crust;
};

enum class CatppuccinThemeType {
    Latte,
    Frappe,
    Macchiato,
    Mocha
};

class CatppuccinTheme : public QObject {
    Q_OBJECT
public:
    explicit CatppuccinTheme(CatppuccinPalette palette, QObject *parent = nullptr)
        : QObject(parent),
          palette(std::move(palette)) {
    }

    [[nodiscard]] virtual QString getRosewater() const {
        return palette.rosewater;
    }

    [[nodiscard]] virtual QString getFlamingo() const {
        return palette.flamingo;
    }

    [[nodiscard]] virtual QString getPink() const {
        return palette.pink;
    }

    [[nodiscard]] virtual QString getMauve() const {
        return palette.mauve;
    }

    [[nodiscard]] virtual QString getRed() const {
        return palette.red;
    }

    [[nodiscard]] virtual QString getMaroon() const {
        return palette.maroon;
    }

    [[nodiscard]] virtual QString getPeach() const {
        return palette.peach;
    }

    [[nodiscard]] virtual QString getYellow() const {
        return palette.yellow;
    }

    [[nodiscard]] virtual QString getGreen() const {
        return palette.green;
    }

    [[nodiscard]] virtual QString getTeal() const {
        return palette.teal;
    }

    [[nodiscard]] virtual QString getSky() const {
        return palette.sky;
    }

    [[nodiscard]] virtual QString getSapphire() const {
        return palette.sapphire;
    }

    [[nodiscard]] virtual QString getBlue() const {
        return palette.blue;
    }

    [[nodiscard]] virtual QString getLavender() const {
        return palette.lavender;
    }

    [[nodiscard]] virtual QString getText() const {
        return palette.text;
    }

    [[nodiscard]] virtual QString getSubtext1() const {
        return palette.subtext1;
    }

    [[nodiscard]] virtual QString getSubtext0() const {
        return palette.subtext0;
    }

    [[nodiscard]] virtual QString getOverlay2() const {
        return palette.overlay2;
    }

    [[nodiscard]] virtual QString getOverlay1() const {
        return palette.overlay1;
    }

    [[nodiscard]] virtual QString getOverlay0() const {
        return palette.overlay0;
    }

    [[nodiscard]] virtual QString getSurface2() const {
        return palette.surface2;
    }

    [[nodiscard]] virtual QString getSurface1() const {
        return palette.surface1;
    }

    [[nodiscard]] virtual QString getSurface0() const {
        return palette.surface0;
    }

    [[nodiscard]] virtual QString getBase() const {
        return palette.base;
    }

    [[nodiscard]] virtual QString getMantle() const {
        return palette.mantle;
    }

    [[nodiscard]] virtual QString getCrust() const {
        return palette.crust;
    }

private:
    CatppuccinPalette palette;
};
