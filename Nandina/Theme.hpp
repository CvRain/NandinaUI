
#ifndef THEME_HPP
#define THEME_HPP

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QJSEngine>

struct Palette {
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

class Theme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rosewater READ rosewater CONSTANT)
    Q_PROPERTY(QString flamingo READ flamingo CONSTANT)
    Q_PROPERTY(QString pink READ pink CONSTANT)
    Q_PROPERTY(QString mauve READ mauve CONSTANT)
    Q_PROPERTY(QString red READ red CONSTANT)
    Q_PROPERTY(QString maroon READ maroon CONSTANT)
    Q_PROPERTY(QString peach READ peach CONSTANT)
    Q_PROPERTY(QString yellow READ yellow CONSTANT)
    Q_PROPERTY(QString green READ green CONSTANT)
    Q_PROPERTY(QString teal READ teal CONSTANT)
    Q_PROPERTY(QString sky READ sky CONSTANT)
    Q_PROPERTY(QString sapphire READ sapphire CONSTANT)
    Q_PROPERTY(QString blue READ blue CONSTANT)
    Q_PROPERTY(QString lavender READ lavender CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(QString subtext1 READ subtext1 CONSTANT)
    Q_PROPERTY(QString subtext0 READ subtext0 CONSTANT)
    Q_PROPERTY(QString overlay2 READ overlay2 CONSTANT)
    Q_PROPERTY(QString overlay1 READ overlay1 CONSTANT)
    Q_PROPERTY(QString overlay0 READ overlay0 CONSTANT)
    Q_PROPERTY(QString surface2 READ surface2 CONSTANT)
    Q_PROPERTY(QString surface1 READ surface1 CONSTANT)
    Q_PROPERTY(QString surface0 READ surface0 CONSTANT)
    Q_PROPERTY(QString base READ base CONSTANT)
    Q_PROPERTY(QString mantle READ mantle CONSTANT)
    Q_PROPERTY(QString crust READ crust CONSTANT)

public:
    explicit Theme(const Palette &palette, QObject *parent = nullptr);

    QString rosewater() const;
    QString flamingo() const;
    QString pink() const;
    QString mauve() const;
    QString red() const;
    QString maroon() const;
    QString peach() const;
    QString yellow() const;
    QString green() const;
    QString teal() const;
    QString sky() const;
    QString sapphire() const;
    QString blue() const;
    QString lavender() const;
    QString text() const;
    QString subtext1() const;
    QString subtext0() const;
    QString overlay2() const;
    QString overlay1() const;
    QString overlay0() const;
    QString surface2() const;
    QString surface1() const;
    QString surface0() const;
    QString base() const;
    QString mantle() const;
    QString crust() const;

private:
    Palette m_palette;
};

#endif // THEME_HPP
