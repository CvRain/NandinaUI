#ifndef NANDINA_STYLE_HPP
#define NANDINA_STYLE_HPP

#include <CatppuccinTheme.hpp>
#include <QObject>

#include "CatppuccinTheme.hpp"


#include <memory>

class NandinaStyle : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString rosewater READ getRosewater NOTIFY themeChanged)
    Q_PROPERTY(QString flamingo READ getFlamingo NOTIFY themeChanged)
    Q_PROPERTY(QString pink READ getPink NOTIFY themeChanged)
    Q_PROPERTY(QString mauve READ getMauve NOTIFY themeChanged)
    Q_PROPERTY(QString red READ getRed NOTIFY themeChanged)
    Q_PROPERTY(QString maroon READ getMaroon NOTIFY themeChanged)
    Q_PROPERTY(QString peach READ getPeach NOTIFY themeChanged)
    Q_PROPERTY(QString yellow READ getYellow NOTIFY themeChanged)
    Q_PROPERTY(QString green READ getGreen NOTIFY themeChanged)
    Q_PROPERTY(QString teal READ getTeal NOTIFY themeChanged)
    Q_PROPERTY(QString sky READ getSky NOTIFY themeChanged)
    Q_PROPERTY(QString sapphire READ getSapphire NOTIFY themeChanged)
    Q_PROPERTY(QString blue READ getBlue NOTIFY themeChanged)
    Q_PROPERTY(QString lavender READ getLavender NOTIFY themeChanged)
    Q_PROPERTY(QString text READ getText NOTIFY themeChanged)
    Q_PROPERTY(QString subtext1 READ getSubtext1 NOTIFY themeChanged)
    Q_PROPERTY(QString subtext0 READ getSubtext0 NOTIFY themeChanged)
    Q_PROPERTY(QString overlay2 READ getOverlay2 NOTIFY themeChanged)
    Q_PROPERTY(QString overlay1 READ getOverlay1 NOTIFY themeChanged)
    Q_PROPERTY(QString overlay0 READ getOverlay0 NOTIFY themeChanged)
    Q_PROPERTY(QString surface2 READ getSurface0 NOTIFY themeChanged)
    Q_PROPERTY(QString surface1 READ getSurface0 NOTIFY themeChanged)
    Q_PROPERTY(QString surface0 READ getSurface0 NOTIFY themeChanged)
    Q_PROPERTY(QString base READ getBase NOTIFY themeChanged)
    Q_PROPERTY(QString mantle READ getMantle NOTIFY themeChanged)
    Q_PROPERTY(QString crust READ getCrust NOTIFY themeChanged)
    Q_PROPERTY(
        NandinaType::CatppuccinThemeType currentTheme
        READ getCurrentThemeType
        WRITE setCurrentThemeType
        NOTIFY currentThemeTypeChanged)

public:
    explicit NandinaStyle(QObject *parent = nullptr);

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

    [[nodiscard]] NandinaType::CatppuccinThemeType getCurrentThemeType() const;

    void setCurrentThemeType(NandinaType::CatppuccinThemeType newCurrentThemeType);

    void updateCurrentTheme();
    
signals:
    void currentThemeTypeChanged();
    void themeChanged();

private:
    std::shared_ptr<CatppuccinTheme> currentTheme;
    NandinaType::CatppuccinThemeType currentThemeType;
};

#endif
