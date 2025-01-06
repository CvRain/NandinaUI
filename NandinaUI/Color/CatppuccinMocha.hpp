#pragma once

#include <QQmlEngine>
#include <QString>

class CatppuccinMocha : public QObject {
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
    explicit CatppuccinMocha(QObject *parent = nullptr): QObject(parent) {
    }

private:
    QString rosewater = "#f5e0dc";
    QString flamingo = "#f2cdcd";
    QString pink = "#f5c2e7";
    QString mauve = "#cba6f7";
    QString red = "#f38ba8";
    QString maroon = "#eba0ac";
    QString peach = "#fab387";
    QString yellow = "#f9e2af";
    QString green = "#a6e3a1";
    QString teal = "#94e2d5";
    QString sky = "#89dceb";
    QString sapphire = "#74c7ec";
    QString blue = "#89b4fa";
    QString lavender = "#b4befe";
    QString text = "#cdd6f4";
    QString subtext1 = "#bac2de";
    QString subtext0 = "#a6adc8";
    QString overlay2 = "#9399b2";
    QString overlay1 = "#7f849c";
    QString overlay0 = "#6c7086";
    QString surface2 = "#585b70";
    QString surface1 = "#45475a";
    QString surface0 = "#313244";
    QString base = "#1e1e2e";
    QString mantle = "#181825";
    QString crust = "#11111b";
};
