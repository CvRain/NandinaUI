//
// Created by cvrain on 2025/10/24.
//

#include "themeManager.hpp"

using namespace Nandina;

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
    palettes.insert(std::make_pair(PaletteType::Frappe, Nandina::Tools::generateFrappePalette()));
    palettes.insert(std::make_pair(PaletteType::Latte, Nandina::Tools::generateLattePalette()));
    palettes.insert(std::make_pair(PaletteType::Macchiato, Nandina::Tools::generateMacchiatoPalette()));
    palettes.insert(std::make_pair(PaletteType::Mocha, Nandina::Tools::generateMochaPalette()));
    
    baseColors.insert(std::make_pair(PaletteType::Frappe, Nandina::BaseColorTools::generateFrappeBaseColors()));
    baseColors.insert(std::make_pair(PaletteType::Latte, Nandina::BaseColorTools::generateLatteBaseColors()));
    baseColors.insert(std::make_pair(PaletteType::Macchiato, Nandina::BaseColorTools::generateMacchiatoBaseColors()));
    baseColors.insert(std::make_pair(PaletteType::Mocha, Nandina::BaseColorTools::generateMochaBaseColors()));
    
    this->currentBaseColors = &this->baseColors.at(currentPaletteType);
}