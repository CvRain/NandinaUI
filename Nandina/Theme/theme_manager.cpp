//
// Created by cvrain on 2026/2/11.
//

#include "theme_manager.hpp"

namespace Nandina::NandinaTheme {
    ThemeManager::ThemeManager(QObject *parent) :
        QObject(parent), currentPaletteType(NandinaColor::PaletteType::Latte) {
        customColorCollection = new NandinaColor::ColorCollection(this);
        customPaletteCollection = new NandinaColor::PaletteCollection(this);
        customColorCollection->type = NandinaColor::PaletteType::Custom;
        updateCurrentCollections();
    }

    NandinaColor::PaletteType ThemeManager::getCurrentPaletteType() const {
        return currentPaletteType;
    }

    NandinaColor::ColorCollection *ThemeManager::getCurrentColorCollection() {
        if (currentColorCollection == nullptr) {
            currentColorCollection = &colorAtla.colorCollections[currentPaletteType];
        }

        return currentColorCollection;
    }

    NandinaColor::PaletteCollection *ThemeManager::getCurrentPaletteCollection() {
        if (currentPaletteCollection == nullptr) {
            currentPaletteCollection = &colorAtla.paletteCollections[currentPaletteType];
        }
        return currentPaletteCollection;
    }

    NandinaColor::ColorCollection *ThemeManager::getCustomColorCollection() {
        return customColorCollection;
    }

    void ThemeManager::setCustomColorCollection(NandinaColor::ColorCollection *collection) {
        if (collection == nullptr || collection == customColorCollection) {
            return;
        }

        delete customColorCollection;
        customColorCollection = new NandinaColor::ColorCollection(*collection, this);
        customColorCollection->type = NandinaColor::PaletteType::Custom;

        if (currentPaletteType == NandinaColor::PaletteType::Custom) {
            updateCurrentCollections();
            emit paletteTypeChanged(currentPaletteType);
        }

        emit customThemeChanged();
    }

    NandinaColor::PaletteCollection *ThemeManager::getCustomPaletteCollection() const {
        return customPaletteCollection;
    }

    void ThemeManager::setCustomPaletteCollection(NandinaColor::PaletteCollection *collection) {
        if (collection == nullptr || collection == customPaletteCollection) {
            return;
        }

        delete customPaletteCollection;
        customPaletteCollection = new NandinaColor::PaletteCollection(*collection, this);

        if (currentPaletteType == NandinaColor::PaletteType::Custom) {
            updateCurrentCollections();
            emit paletteTypeChanged(currentPaletteType);
        }

        emit customThemeChanged();
    }

    void ThemeManager::setCurrentPaletteType(const NandinaColor::PaletteType type) {
        if (this->currentPaletteType == type) {
            emit paletteTypeChanged(currentPaletteType);
            return;
        }

        currentPaletteType = type;
        updateCurrentCollections();

        emit paletteTypeChanged(currentPaletteType);
    }

    void ThemeManager::updateCurrentCollections() {
        if (currentPaletteType == NandinaColor::PaletteType::Custom) {
            currentColorCollection = customColorCollection;
            currentPaletteCollection = customPaletteCollection;
            return;
        }

        currentColorCollection = &colorAtla.colorCollections[currentPaletteType];
        currentPaletteCollection = &colorAtla.paletteCollections[currentPaletteType];
    }
} // namespace Nandina::NandinaTheme
