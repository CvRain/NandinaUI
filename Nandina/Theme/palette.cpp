//
// Created by cvrain on 2025/10/24.
//

#include "palette.hpp"

namespace NandinaUI {
    Palette::Palette(QObject *parent)
        : QObject(parent) {
    }

    Palette::Palette(const Palette &palette) {
        this->backgroundPane = palette.backgroundPane;
        this->secondaryPane = palette.secondaryPane;
        this->surfaceElements = palette.surfaceElements;
        this->overlays = palette.overlays;
        this->bodyCopy = palette.bodyCopy;
        this->mainHeadline = palette.mainHeadline;
        this->subHeadline = palette.subHeadline;
        this->label = palette.label;
        this->subtle = palette.subtle;
        this->onAccent = palette.onAccent;
        this->links = palette.links;
        this->urls = palette.urls;
        this->success = palette.success;
        this->warning = palette.warning;
        this->error = palette.error;
        this->tags = palette.tags;
        this->pills = palette.pills;
        this->selectionBackground = palette.selectionBackground;
        this->cursor = palette.cursor;
        this->cursorText = palette.cursorText;
        this->activeBorder = palette.activeBorder;
        this->inactiveBorder = palette.inactiveBorder;
        this->bellBorder = palette.bellBorder;
        this->mark1 = palette.mark1;
        this->mark2 = palette.mark2;
        this->mark3 = palette.mark3;
        this->mark1Text = palette.mark1Text;
        this->mark2Text = palette.mark2Text;
        this->mark3Text = palette.mark3Text;
    }

    Palette& Palette::operator=(const Palette &) {
        return *this;
    }

    Tools::Tools(QObject* parent)
        :QObject(parent){

    }

    Palette Tools::generatePalette(const PaletteType type) {
        switch (type) {
            case PaletteType::Frappe:
                return generateFrappePalette();
            case PaletteType::Latte:
                return generateLattePalette();
            case PaletteType::Macchiato:
                return generateMacchiatoPalette();
            case PaletteType::Mocha:
                return generateMochaPalette();
            default:
                return generateLattePalette();
        }
    }

    Palette Tools::generateLattePalette() {
        Palette palette;
        palette.backgroundPane = Color::Latte::Base;
        palette.secondaryPane = Color::Latte::Crust;
        palette.surfaceElements = Color::Latte::Surface0;
        palette.overlays = Color::Latte::Overlay0;
        palette.bodyCopy = Color::Latte::Text;
        palette.mainHeadline = Color::Latte::Text;
        palette.subHeadline = Color::Latte::Overlay1;
        palette.label = Color::Latte::Blue;
        palette.subtle = Color::Latte::Blue;
        palette.onAccent = Color::Latte::Base;
        palette.links = Color::Latte::Blue;
        palette.urls = Color::Latte::Blue;
        palette.success = Color::Latte::Green;
        palette.warning = Color::Latte::Yellow;
        palette.error = Color::Latte::Red;
        palette.tags = Color::Latte::Blue;
        palette.pills = Color::Latte::Blue;
        palette.selectionBackground = Color::Latte::Overlay2;
        palette.cursor = Color::Latte::Rosewater;
        palette.cursorText = Color::Latte::Rosewater;
        palette.activeBorder = Color::Latte::Base;
        palette.inactiveBorder = Color::Latte::Lavender;
        palette.bellBorder = Color::Latte::Yellow;
        palette.mark1 = Color::Latte::Lavender;
        palette.mark2 = Color::Latte::Mauve;
        palette.mark3 = Color::Latte::Sapphire;
        palette.mark1Text = Color::Latte::Base;
        palette.mark2Text = Color::Latte::Base;
        palette.mark3Text = Color::Latte::Base;


        return palette;
    }

    Palette Tools::generateFrappePalette() {
        Palette palette;
        palette.backgroundPane = Color::Frappe::Base;
        palette.secondaryPane = Color::Frappe::Crust;
        palette.surfaceElements = Color::Frappe::Surface0;
        palette.overlays = Color::Frappe::Overlay0;
        palette.bodyCopy = Color::Frappe::Text;
        palette.mainHeadline = Color::Frappe::Text;
        palette.subHeadline = Color::Frappe::Overlay1;
        palette.label = Color::Frappe::Blue;
        palette.subtle = Color::Frappe::Blue;
        palette.onAccent = Color::Frappe::Base;
        palette.links = Color::Frappe::Blue;
        palette.urls = Color::Frappe::Blue;
        palette.success = Color::Frappe::Green;
        palette.warning = Color::Frappe::Yellow;
        palette.error = Color::Frappe::Red;
        palette.tags = Color::Frappe::Blue;
        palette.pills = Color::Frappe::Blue;
        palette.selectionBackground = Color::Frappe::Overlay2;
        palette.cursor = Color::Frappe::Rosewater;
        palette.cursorText = Color::Frappe::Rosewater;
        palette.activeBorder = Color::Frappe::Base;
        palette.inactiveBorder = Color::Frappe::Lavender;
        palette.bellBorder = Color::Frappe::Yellow;
        palette.mark1 = Color::Frappe::Lavender;
        palette.mark2 = Color::Frappe::Mauve;
        palette.mark3 = Color::Frappe::Sapphire;
        palette.mark1Text = Color::Frappe::Crust;
        palette.mark2Text = Color::Frappe::Crust;
        palette.mark3Text = Color::Frappe::Crust;
        return palette;
    }

    Palette Tools::generateMacchiatoPalette() {
        Palette palette;
        palette.backgroundPane = Color::Macchiato::Base;
        palette.secondaryPane = Color::Macchiato::Crust;
        palette.surfaceElements = Color::Macchiato::Surface0;
        palette.overlays = Color::Macchiato::Overlay0;
        palette.bodyCopy = Color::Macchiato::Text;
        palette.mainHeadline = Color::Macchiato::Text;
        palette.subHeadline = Color::Macchiato::Overlay1;
        palette.label = Color::Macchiato::Blue;
        palette.subtle = Color::Macchiato::Blue;
        palette.onAccent = Color::Macchiato::Base;
        palette.links = Color::Macchiato::Blue;
        palette.urls = Color::Macchiato::Blue;
        palette.success = Color::Macchiato::Green;
        palette.warning = Color::Macchiato::Yellow;
        palette.error = Color::Macchiato::Red;
        palette.tags = Color::Macchiato::Blue;
        palette.pills = Color::Macchiato::Blue;
        palette.selectionBackground = Color::Macchiato::Overlay2;
        palette.cursor = Color::Macchiato::Rosewater;
        palette.cursorText = Color::Macchiato::Rosewater;
        palette.activeBorder = Color::Macchiato::Base;
        palette.inactiveBorder = Color::Macchiato::Lavender;
        palette.bellBorder = Color::Macchiato::Yellow;
        palette.mark1 = Color::Macchiato::Crust;
        palette.mark2 = Color::Macchiato::Mauve;
        palette.mark3 = Color::Macchiato::Sapphire;
        palette.mark1Text = Color::Macchiato::Crust;
        palette.mark2Text = Color::Macchiato::Crust;
        palette.mark3Text = Color::Macchiato::Crust;
        return palette;
    }

    Palette Tools::generateMochaPalette() {
        Palette palette;
        palette.backgroundPane = Color::Mocha::Base;
        palette.secondaryPane = Color::Mocha::Crust;
        palette.surfaceElements = Color::Mocha::Surface0;
        palette.overlays = Color::Mocha::Overlay0;
        palette.bodyCopy = Color::Mocha::Text;
        palette.mainHeadline = Color::Mocha::Text;
        palette.subHeadline = Color::Mocha::Overlay1;
        palette.label = Color::Mocha::Blue;
        palette.subtle = Color::Mocha::Blue;
        palette.onAccent = Color::Mocha::Base;
        palette.links = Color::Mocha::Blue;
        palette.urls = Color::Mocha::Blue;
        palette.success = Color::Mocha::Green;
        palette.warning = Color::Mocha::Yellow;
        palette.error = Color::Mocha::Red;
        palette.tags = Color::Mocha::Blue;
        palette.pills = Color::Mocha::Blue;
        palette.selectionBackground = Color::Mocha::Overlay2;
        palette.cursor = Color::Mocha::Rosewater;
        palette.cursorText = Color::Mocha::Rosewater;
        palette.activeBorder = Color::Mocha::Base;
        palette.inactiveBorder = Color::Mocha::Lavender;
        palette.bellBorder = Color::Mocha::Yellow;
        palette.mark1 = Color::Mocha::Crust;
        palette.mark2 = Color::Mocha::Mauve;
        palette.mark3 = Color::Mocha::Sapphire;
        palette.mark1Text = Color::Mocha::Crust;
        palette.mark2Text = Color::Mocha::Crust;
        palette.mark3Text = Color::Mocha::Crust;
        return palette;
    }
}
