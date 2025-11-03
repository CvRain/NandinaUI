//
// Created by Lingma on 2025/10/28.
//

#include "baseColors.hpp"

namespace Nandina {
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
}