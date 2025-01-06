#pragma once

#include <QQmlEngine>
#include <QString>

class CatppuccinMacchiato : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString rosewater MEMBER rosewater)
    Q_PROPERTY(QString flamingo MEMBER flamingo)
    Q_PROPERTY(QString pink MEMBER pink)
    Q_PROPERTY(QString mauve MEMBER mauve)
    Q_PROPERTY(QString red MEMBER red)
    Q_PROPERTY(QString maroon MEMBER maroon)
    Q_PROPERTY(QString peach MEMBER peach)
    Q_PROPERTY(QString yellow MEMBER yellow)
    Q_PROPERTY(QString green MEMBER green)
    Q_PROPERTY(QString teal MEMBER teal)
    Q_PROPERTY(QString sky MEMBER sky)
    Q_PROPERTY(QString sapphire MEMBER sapphire)
    Q_PROPERTY(QString blue MEMBER blue)
    Q_PROPERTY(QString lavender MEMBER lavender)
    Q_PROPERTY(QString text MEMBER text)
    Q_PROPERTY(QString subtext1 MEMBER subtext1)
    Q_PROPERTY(QString subtext0 MEMBER subtext0)
    Q_PROPERTY(QString overlay2 MEMBER overlay2)
    Q_PROPERTY(QString overlay1 MEMBER overlay1)
    Q_PROPERTY(QString overlay0 MEMBER overlay0)
    Q_PROPERTY(QString surface2 MEMBER surface2)
    Q_PROPERTY(QString surface1 MEMBER surface1)
    Q_PROPERTY(QString surface0 MEMBER surface0)
    Q_PROPERTY(QString base MEMBER base)
    Q_PROPERTY(QString mantle MEMBER mantle)
    Q_PROPERTY(QString crust MEMBER crust)

public:
    explicit CatppuccinMacchiato(QObject *parent = nullptr);

private:
    QString rosewater = "#f4dbd6";
    QString flamingo = "#f0c6c6";
    QString pink = "#f5bde6";
    QString mauve = "#c6a0f6";
    QString red = "#ed8796";
    QString maroon = "#ee99a0";
    QString peach = "#f5a97f";
    QString yellow = "#eed49f";
    QString green = "#a6da95";
    QString teal = "#8bd5ca";
    QString sky = "#91d7e3";
    QString sapphire = "#7dc4e4";
    QString blue = "#8aadf4";
    QString lavender = "#b7bdf8";
    QString text = "#cad3f5";
    QString subtext1 = "#b8c0e0";
    QString subtext0 = "#a5adcb";
    QString overlay2 = "#939ab7";
    QString overlay1 = "#8087a2";
    QString overlay0 = "#6e738d";
    QString surface2 = "#5b6078";
    QString surface1 = "#494d64";
    QString surface0 = "#363a4f";
    QString base = "#24273a";
    QString mantle = "#1e2030";
    QString crust = "#181926";
};
