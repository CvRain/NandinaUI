#pragma once

#include <QQmlEngine>
#include <QString>

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
    explicit CatppuccinTheme(CatppuccinPalette palette, QObject *parent = nullptr);

    [[nodiscard]] virtual QString getRosewater() const;

    [[nodiscard]] virtual QString getFlamingo() const;

    [[nodiscard]] virtual QString getPink() const;

    [[nodiscard]] virtual QString getMauve() const;

    [[nodiscard]] virtual QString getRed() const;

    [[nodiscard]] virtual QString getMaroon() const;

    [[nodiscard]] virtual QString getPeach() const;

    [[nodiscard]] virtual QString getYellow() const;

    [[nodiscard]] virtual QString getGreen() const;

    [[nodiscard]] virtual QString getTeal() const;

    [[nodiscard]] virtual QString getSky() const;

    [[nodiscard]] virtual QString getSapphire() const;

    [[nodiscard]] virtual QString getBlue() const;

    [[nodiscard]] virtual QString getLavender() const;

    [[nodiscard]] virtual QString getText() const;

    [[nodiscard]] virtual QString getSubtext1() const;

    [[nodiscard]] virtual QString getSubtext0() const;

    [[nodiscard]] virtual QString getOverlay2() const;

    [[nodiscard]] virtual QString getOverlay1() const;

    [[nodiscard]] virtual QString getOverlay0() const;

    [[nodiscard]] virtual QString getSurface2() const;

    [[nodiscard]] virtual QString getSurface1() const;

    [[nodiscard]] virtual QString getSurface0() const;

    [[nodiscard]] virtual QString getBase() const;

    [[nodiscard]] virtual QString getMantle() const;

    [[nodiscard]] virtual QString getCrust() const;

private:
    CatppuccinPalette palette;
};
