//
// Created by cvrain on 2025/10/24.
//

#include "themeManager.hpp"

using namespace NandinaUI;

ThemeManager *ThemeManager::instance = nullptr;


ThemeManager* ThemeManager::create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    return getInstance();
}

ThemeManager* ThemeManager::getInstance() {
    if (instance == nullptr) {
        instance = new ThemeManager();
    }
    return instance;
}

PaletteType ThemeManager::getCurrentPaletteType() const {
    return currentPaletteType;
}

void ThemeManager::setCurrentPaletteType(const PaletteType type) {
    this->currentPaletteType = type;
    this->currentBaseColors = &this->baseColors.at(type);
    emit paletteChanged(type);
}

Palette* ThemeManager::getPalette() {
    return &this->palettes.at(currentPaletteType);
}

BaseColors* ThemeManager::getColor() {
    return this->currentBaseColors;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), currentPaletteType(PaletteType::Latte) {
    palettes.insert(std::make_pair(PaletteType::Frappe, NandinaUI::Tools::generateFrappePalette()));
    palettes.insert(std::make_pair(PaletteType::Latte, NandinaUI::Tools::generateLattePalette()));
    palettes.insert(std::make_pair(PaletteType::Macchiato, NandinaUI::Tools::generateMacchiatoPalette()));
    palettes.insert(std::make_pair(PaletteType::Mocha, NandinaUI::Tools::generateMochaPalette()));
    
    baseColors.insert(std::make_pair(PaletteType::Frappe, NandinaUI::BaseColorTools::generateFrappeBaseColors()));
    baseColors.insert(std::make_pair(PaletteType::Latte, NandinaUI::BaseColorTools::generateLatteBaseColors()));
    baseColors.insert(std::make_pair(PaletteType::Macchiato, NandinaUI::BaseColorTools::generateMacchiatoBaseColors()));
    baseColors.insert(std::make_pair(PaletteType::Mocha, NandinaUI::BaseColorTools::generateMochaBaseColors()));
    
    this->currentBaseColors = &this->baseColors.at(currentPaletteType);
}