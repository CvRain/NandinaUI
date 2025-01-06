#pragma once

#include <QQmlEngine>
#include <QString>

class CatppuccinLatte : public QObject {
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
    explicit CatppuccinLatte(QObject *parent = nullptr) : QObject(parent) {
    }

private:
    QString rosewater = "#dc8a78";
    QString flamingo = "#dd7878";
    QString pink = "#ea76cb";
    QString mauve = "#8839ef";
    QString red = "#d20f39";
    QString maroon = "#e64553";
    QString peach = "#fe640b";
    QString yellow = "#df8e1d";
    QString green = "#40a02b";
    QString teal = "#179299";
    QString sky = "#04a5e5";
    QString sapphire = "#209fb5";
    QString blue = "#1e66f5";
    QString lavender = "#7287fd";
    QString text = "#4c4f69";
    QString subtext1 = "#5c5f77";
    QString subtext0 = "#6c6f85";
    QString overlay2 = "#7c7f93";
    QString overlay1 = "#8c8fa1";
    QString overlay0 = "#9ca0b0";
    QString surface2 = "#acb0be";
    QString surface1 = "#bcc0cc";
    QString surface0 = "#ccd0da";
    QString base = "#eff1f5";
    QString mantle = "#e6e9ef";
    QString crust = "#dce0e8";
};
