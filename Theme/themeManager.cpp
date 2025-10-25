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

const Palette& ThemeManager::getCurrentPalette() {
    return this->palettes.at(currentPaletteType);
}

PaletteType ThemeManager::getCurrentPaletteType() const {
    return currentPaletteType;
}

void ThemeManager::setCurrentPaletteType(const PaletteType type) {
    this->currentPaletteType = type;
    emit paletteChanged(type);
}

QColor ThemeManager::getColorBackgroundPane() const{
    return QColor(this->palettes.at(currentPaletteType).backgroundPane);
}

QColor ThemeManager::getColorWarning() const{
    return QColor(this->palettes.at(currentPaletteType).warning);
}

QColor ThemeManager::getColorSuccess() const{
    return QColor(this->palettes.at(currentPaletteType).success);
}

QColor ThemeManager::getColorError() const{
    return QColor(this->palettes.at(currentPaletteType).error);
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), currentPaletteType(PaletteType::Latte) {
    palettes.insert(std::make_pair(PaletteType::Frappe, NandinaUI::Tools::generateFrappePalette()));
    palettes.insert(std::make_pair(PaletteType::Latte, NandinaUI::Tools::generateLattePalette()));
    palettes.insert(std::make_pair(PaletteType::Macchiato, NandinaUI::Tools::generateMacchiatoPalette()));
    palettes.insert(std::make_pair(PaletteType::Mocha, NandinaUI::Tools::generateMochaPalette()));
}
