//
// Created by Lingma on 2025/10/28.
//

#include "baseColors.hpp"

namespace NandinaUI {
    BaseColors::BaseColors(QObject *parent)
        : QObject(parent) {
    }

    BaseColors::BaseColors(const BaseColors &other) 
        : QObject(other.parent()) {
        rosewater = other.rosewater;
        flamingo = other.flamingo;
        pink = other.pink;
        mauve = other.mauve;
        red = other.red;
        maroon = other.maroon;
        peach = other.peach;
        yellow = other.yellow;
        green = other.green;
        teal = other.teal;
        sky = other.sky;
        sapphire = other.sapphire;
        blue = other.blue;
        lavender = other.lavender;
        text = other.text;
        subtext1 = other.subtext1;
        subtext0 = other.subtext0;
        overlay2 = other.overlay2;
        overlay1 = other.overlay1;
        overlay0 = other.overlay0;
        surface2 = other.surface2;
        surface1 = other.surface1;
        surface0 = other.surface0;
        base = other.base;
        mantle = other.mantle;
        crust = other.crust;
    }

    BaseColors& BaseColors::operator=(const BaseColors &other) {
        if (this != &other) {
            rosewater = other.rosewater;
            flamingo = other.flamingo;
            pink = other.pink;
            mauve = other.mauve;
            red = other.red;
            maroon = other.maroon;
            peach = other.peach;
            yellow = other.yellow;
            green = other.green;
            teal = other.teal;
            sky = other.sky;
            sapphire = other.sapphire;
            blue = other.blue;
            lavender = other.lavender;
            text = other.text;
            subtext1 = other.subtext1;
            subtext0 = other.subtext0;
            overlay2 = other.overlay2;
            overlay1 = other.overlay1;
            overlay0 = other.overlay0;
            surface2 = other.surface2;
            surface1 = other.surface1;
            surface0 = other.surface0;
            base = other.base;
            mantle = other.mantle;
            crust = other.crust;
        }
        return *this;
    }

    BaseColorTools::BaseColorTools(QObject *parent)
        : QObject(parent) {
    }

    BaseColors BaseColorTools::generateLatteBaseColors() {
        BaseColors baseColors;
        baseColors.rosewater = Color::Latte::Rosewater;
        baseColors.flamingo = Color::Latte::Flamingo;
        baseColors.pink = Color::Latte::Pink;
        baseColors.mauve = Color::Latte::Mauve;
        baseColors.red = Color::Latte::Red;
        baseColors.maroon = Color::Latte::Maroon;
        baseColors.peach = Color::Latte::Peach;
        baseColors.yellow = Color::Latte::Yellow;
        baseColors.green = Color::Latte::Green;
        baseColors.teal = Color::Latte::Teal;
        baseColors.sky = Color::Latte::Sky;
        baseColors.sapphire = Color::Latte::Sapphire;
        baseColors.blue = Color::Latte::Blue;
        baseColors.lavender = Color::Latte::Lavender;
        baseColors.text = Color::Latte::Text;
        baseColors.subtext1 = Color::Latte::Subtext1;
        baseColors.subtext0 = Color::Latte::Subtext0;
        baseColors.overlay2 = Color::Latte::Overlay2;
        baseColors.overlay1 = Color::Latte::Overlay1;
        baseColors.overlay0 = Color::Latte::Overlay0;
        baseColors.surface2 = Color::Latte::Surface2;
        baseColors.surface1 = Color::Latte::Surface1;
        baseColors.surface0 = Color::Latte::Surface0;
        baseColors.base = Color::Latte::Base;
        baseColors.mantle = Color::Latte::Mantle;
        baseColors.crust = Color::Latte::Crust;
        return baseColors;
    }

    BaseColors BaseColorTools::generateFrappeBaseColors() {
        BaseColors baseColors;
        baseColors.rosewater = Color::Frappe::Rosewater;
        baseColors.flamingo = Color::Frappe::Flamingo;
        baseColors.pink = Color::Frappe::Pink;
        baseColors.mauve = Color::Frappe::Mauve;
        baseColors.red = Color::Frappe::Red;
        baseColors.maroon = Color::Frappe::Maroon;
        baseColors.peach = Color::Frappe::Peach;
        baseColors.yellow = Color::Frappe::Yellow;
        baseColors.green = Color::Frappe::Green;
        baseColors.teal = Color::Frappe::Teal;
        baseColors.sky = Color::Frappe::Sky;
        baseColors.sapphire = Color::Frappe::Sapphire;
        baseColors.blue = Color::Frappe::Blue;
        baseColors.lavender = Color::Frappe::Lavender;
        baseColors.text = Color::Frappe::Text;
        baseColors.subtext1 = Color::Frappe::Subtext1;
        baseColors.subtext0 = Color::Frappe::Subtext0;
        baseColors.overlay2 = Color::Frappe::Overlay2;
        baseColors.overlay1 = Color::Frappe::Overlay1;
        baseColors.overlay0 = Color::Frappe::Overlay0;
        baseColors.surface2 = Color::Frappe::Surface2;
        baseColors.surface1 = Color::Frappe::Surface1;
        baseColors.surface0 = Color::Frappe::Surface0;
        baseColors.base = Color::Frappe::Base;
        baseColors.mantle = Color::Frappe::Mantle;
        baseColors.crust = Color::Frappe::Crust;
        return baseColors;
    }

    BaseColors BaseColorTools::generateMacchiatoBaseColors() {
        BaseColors baseColors;
        baseColors.rosewater = Color::Macchiato::Rosewater;
        baseColors.flamingo = Color::Macchiato::Flamingo;
        baseColors.pink = Color::Macchiato::Pink;
        baseColors.mauve = Color::Macchiato::Mauve;
        baseColors.red = Color::Macchiato::Red;
        baseColors.maroon = Color::Macchiato::Maroon;
        baseColors.peach = Color::Macchiato::Peach;
        baseColors.yellow = Color::Macchiato::Yellow;
        baseColors.green = Color::Macchiato::Green;
        baseColors.teal = Color::Macchiato::Teal;
        baseColors.sky = Color::Macchiato::Sky;
        baseColors.sapphire = Color::Macchiato::Sapphire;
        baseColors.blue = Color::Macchiato::Blue;
        baseColors.lavender = Color::Macchiato::Lavender;
        baseColors.text = Color::Macchiato::Text;
        baseColors.subtext1 = Color::Macchiato::Subtext1;
        baseColors.subtext0 = Color::Macchiato::Subtext0;
        baseColors.overlay2 = Color::Macchiato::Overlay2;
        baseColors.overlay1 = Color::Macchiato::Overlay1;
        baseColors.overlay0 = Color::Macchiato::Overlay0;
        baseColors.surface2 = Color::Macchiato::Surface2;
        baseColors.surface1 = Color::Macchiato::Surface1;
        baseColors.surface0 = Color::Macchiato::Surface0;
        baseColors.base = Color::Macchiato::Base;
        baseColors.mantle = Color::Macchiato::Mantle;
        baseColors.crust = Color::Macchiato::Crust;
        return baseColors;
    }

    BaseColors BaseColorTools::generateMochaBaseColors() {
        BaseColors baseColors;
        baseColors.rosewater = Color::Mocha::Rosewater;
        baseColors.flamingo = Color::Mocha::Flamingo;
        baseColors.pink = Color::Mocha::Pink;
        baseColors.mauve = Color::Mocha::Mauve;
        baseColors.red = Color::Mocha::Red;
        baseColors.maroon = Color::Mocha::Maroon;
        baseColors.peach = Color::Mocha::Peach;
        baseColors.yellow = Color::Mocha::Yellow;
        baseColors.green = Color::Mocha::Green;
        baseColors.teal = Color::Mocha::Teal;
        baseColors.sky = Color::Mocha::Sky;
        baseColors.sapphire = Color::Mocha::Sapphire;
        baseColors.blue = Color::Mocha::Blue;
        baseColors.lavender = Color::Mocha::Lavender;
        baseColors.text = Color::Mocha::Text;
        baseColors.subtext1 = Color::Mocha::Subtext1;
        baseColors.subtext0 = Color::Mocha::Subtext0;
        baseColors.overlay2 = Color::Mocha::Overlay2;
        baseColors.overlay1 = Color::Mocha::Overlay1;
        baseColors.overlay0 = Color::Mocha::Overlay0;
        baseColors.surface2 = Color::Mocha::Surface2;
        baseColors.surface1 = Color::Mocha::Surface1;
        baseColors.surface0 = Color::Mocha::Surface0;
        baseColors.base = Color::Mocha::Base;
        baseColors.mantle = Color::Mocha::Mantle;
        baseColors.crust = Color::Mocha::Crust;
        return baseColors;
    }

    BaseColors BaseColorTools::generateBaseColors(PaletteType type) {
        switch (type) {
            case PaletteType::Frappe:
                return generateFrappeBaseColors();
            case PaletteType::Latte:
                return generateLatteBaseColors();
            case PaletteType::Macchiato:
                return generateMacchiatoBaseColors();
            case PaletteType::Mocha:
                return generateMochaBaseColors();
            default:
                return generateLatteBaseColors();
        }
    }
}