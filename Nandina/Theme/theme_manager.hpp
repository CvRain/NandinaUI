//
// Created by cvrain on 2026/2/11.
//

#ifndef NANDINA_THEME_MANAGER_HPP
#define NANDINA_THEME_MANAGER_HPP

#include <QObject>
#include <QQmlEngine>

#include "color_atla.hpp"

namespace Nandina::NandinaTheme {
    Q_NAMESPACE
    QML_ELEMENT

    class ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(NandinaColor::PaletteType currentPaletteType READ getCurrentPaletteType WRITE setCurrentPaletteType
                           NOTIFY paletteTypeChanged)

        Q_PROPERTY(NandinaColor::ColorCollection *currentColorCollection READ getCurrentColorCollection NOTIFY
                           paletteTypeChanged)

        Q_PROPERTY(NandinaColor::PaletteCollection *currentPaletteCollection READ getCurrentPaletteCollection NOTIFY
                           paletteTypeChanged)

        Q_PROPERTY(NandinaColor::ColorCollection *customColorCollection READ getCustomColorCollection WRITE
                           setCustomColorCollection NOTIFY customThemeChanged)

        Q_PROPERTY(NandinaColor::PaletteCollection *customPaletteCollection READ getCustomPaletteCollection WRITE
                           setCustomPaletteCollection NOTIFY customThemeChanged)


    public:
        explicit ThemeManager(QObject *parent = nullptr);
        Q_INVOKABLE NandinaColor::PaletteType getCurrentPaletteType() const;
        Q_INVOKABLE void setCurrentPaletteType(NandinaColor::PaletteType type);
        Q_INVOKABLE Nandina::NandinaColor::ColorCollection *getCurrentColorCollection();
        Q_INVOKABLE Nandina::NandinaColor::PaletteCollection *getCurrentPaletteCollection();
        Q_INVOKABLE Nandina::NandinaColor::ColorCollection *getCustomColorCollection();
        Q_INVOKABLE void setCustomColorCollection(Nandina::NandinaColor::ColorCollection *collection);
        Q_INVOKABLE Nandina::NandinaColor::PaletteCollection *getCustomPaletteCollection() const;
        Q_INVOKABLE void setCustomPaletteCollection(Nandina::NandinaColor::PaletteCollection *collection);

    signals:
        void paletteTypeChanged(NandinaColor::PaletteType type);
        void customThemeChanged();

    private:
        NandinaColor::NanColorAtla colorAtla{};
        NandinaColor::PaletteType currentPaletteType;

        NandinaColor::ColorCollection *currentColorCollection{nullptr};
        NandinaColor::PaletteCollection *currentPaletteCollection{nullptr};
        NandinaColor::ColorCollection *customColorCollection{nullptr};
        NandinaColor::PaletteCollection *customPaletteCollection{nullptr};

        void updateCurrentCollections();
    };

} // namespace Nandina::NandinaTheme

#endif // NANDINA_THEME_MANAGER_HPP
