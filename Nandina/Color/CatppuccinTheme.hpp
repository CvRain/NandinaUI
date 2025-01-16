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

class CatppuccinTheme : public QObject {
    Q_OBJECT

public:
    explicit CatppuccinTheme(QObject *parent = nullptr);
    explicit CatppuccinTheme(const CatppuccinPalette& palette, QObject *parent = nullptr);

    [[nodiscard]] QString getRosewater() const;

    [[nodiscard]] QString getFlamingo() const;

    [[nodiscard]] QString getPink() const;

    [[nodiscard]] QString getMauve() const;

    [[nodiscard]] QString getRed() const;

    [[nodiscard]] QString getMaroon() const;

    [[nodiscard]] QString getPeach() const;

    [[nodiscard]] QString getYellow() const;

    [[nodiscard]] QString getGreen() const;

    [[nodiscard]] QString getTeal() const;

    [[nodiscard]] QString getSky() const;

    [[nodiscard]] QString getSapphire() const;

    [[nodiscard]] QString getBlue() const;

    [[nodiscard]] QString getLavender() const;

    [[nodiscard]] QString getText() const;

    [[nodiscard]] QString getSubtext1() const;

    [[nodiscard]] QString getSubtext0() const;

    [[nodiscard]] QString getOverlay2() const;

    [[nodiscard]] QString getOverlay1() const;

    [[nodiscard]] QString getOverlay0() const;

    [[nodiscard]] QString getSurface2() const;

    [[nodiscard]] QString getSurface1() const;

    [[nodiscard]] QString getSurface0() const;

    [[nodiscard]] QString getBase() const;

    [[nodiscard]] QString getMantle() const;

    [[nodiscard]] QString getCrust() const;

protected:
    CatppuccinPalette palette;
};
