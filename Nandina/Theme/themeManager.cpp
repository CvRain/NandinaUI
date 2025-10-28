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

QString ThemeManager::getColorBackgroundPane() const{
    return this->palettes.at(currentPaletteType).backgroundPane;
}

QString ThemeManager::getColorSecondaryPane() {
    return this->palettes.at(currentPaletteType).secondaryPane;
}

QString ThemeManager::getColorSurfaceElements() {
    return this->palettes.at(currentPaletteType).surfaceElements;
}

QString ThemeManager::getColorOverlays() {
    return this->palettes.at(currentPaletteType).overlays;
}

QString ThemeManager::getColorBodyCopy() {
    return this->palettes.at(currentPaletteType).bodyCopy;
}

QString ThemeManager::getColorMainHeadline() {
    return this->palettes.at(currentPaletteType).mainHeadline;
}

QString ThemeManager::getColorSubHeadline() {
    return this->palettes.at(currentPaletteType).subHeadline;
}

QString ThemeManager::getColorLabel() {
    return this->palettes.at(currentPaletteType).label;
}

QString ThemeManager::getColorSubtle() {
    return this->palettes.at(currentPaletteType).subtle;
}

QString ThemeManager::getColorOnAccent() {
    return this->palettes.at(currentPaletteType).onAccent;
}

QString ThemeManager::getColorLinks() {
    return this->palettes.at(currentPaletteType).links;
}

QString ThemeManager::getColorUrls() {
    return this->palettes.at(currentPaletteType).urls;
}

QString ThemeManager::getColorTags() {
    return this->palettes.at(currentPaletteType).tags;
}

QString ThemeManager::getColorPills() {
    return this->palettes.at(currentPaletteType).pills;
}

QString ThemeManager::getColorSelectionBackground() {
    return this->palettes.at(currentPaletteType).selectionBackground;
}

QString ThemeManager::getColorCursor() {
    return this->palettes.at(currentPaletteType).cursor;
}

QString ThemeManager::getColorCursorText() {
    return this->palettes.at(currentPaletteType).cursorText;
}

QString ThemeManager::getColorActiveBorder() {
    return this->palettes.at(currentPaletteType).activeBorder;
}

QString ThemeManager::getColorInactiveBorder() {
    return this->palettes.at(currentPaletteType).inactiveBorder;
}

QString ThemeManager::getColorNellBorder() {
    return this->palettes.at(currentPaletteType).bellBorder;
}

QString ThemeManager::getColorMark1() {
    return this->palettes.at(currentPaletteType).mark1;
}

QString ThemeManager::getColorMark2() {
    return this->palettes.at(currentPaletteType).mark2;
}

QString ThemeManager::getColorMark3() {
    return this->palettes.at(currentPaletteType).mark3;
}

QString ThemeManager::getColorMark1Text() {
    return this->palettes.at(currentPaletteType).mark1Text;
}

QString ThemeManager::getColorMark2Text() {
    return this->palettes.at(currentPaletteType).mark2Text;
}

QString ThemeManager::getColorMark3Text() {
    return this->palettes.at(currentPaletteType).mark3Text;
}

QString ThemeManager::getColorWarning(){
    return this->palettes.at(currentPaletteType).warning;
}

QString ThemeManager::getColorSuccess(){
    return this->palettes.at(currentPaletteType).success;
}

QString ThemeManager::getColorError(){
    return this->palettes.at(currentPaletteType).error;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), currentPaletteType(PaletteType::Latte) {
    palettes.insert(std::make_pair(PaletteType::Frappe, NandinaUI::Tools::generateFrappePalette()));
    palettes.insert(std::make_pair(PaletteType::Latte, NandinaUI::Tools::generateLattePalette()));
    palettes.insert(std::make_pair(PaletteType::Macchiato, NandinaUI::Tools::generateMacchiatoPalette()));
    palettes.insert(std::make_pair(PaletteType::Mocha, NandinaUI::Tools::generateMochaPalette()));
}