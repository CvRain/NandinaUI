//
// Created by cvrain on 2026/2/10.
//

#include "color_atla.hpp"

namespace Nandina::NandinaColor {

    ColorCollection *generateMochaColor(QObject *parent) {
        auto *collection = new ColorCollection(parent);
        collection->type = PaletteType::Mocha;
        collection->rosewater = 0xf5e0dc;
        collection->flamingo = 0xf2cdcd;
        collection->pink = 0xf5c2e7;
        collection->mauve = 0xcba6f7;
        collection->red = 0xf38ba8;
        collection->maroon = 0xeba0ac;
        collection->peach = 0xfab387;
        collection->yellow = 0xf9e2af;
        collection->green = 0xa6e3a1;
        collection->teal = 0x94e2d5;
        collection->sky = 0x89dceb;
        collection->sapphire = 0x74c7ec;
        collection->blue = 0x89b4fa;
        collection->lavender = 0xb4befe;
        collection->text = 0xcdd6f4;
        collection->subtext1 = 0xbac2de;
        collection->subtext0 = 0xa6adc8;
        collection->overlay2 = 0x9399b2;
        collection->overlay1 = 0x7f849c;
        collection->overlay0 = 0x6c7086;
        collection->surface2 = 0x585b70;
        collection->surface1 = 0x45475a;
        collection->surface0 = 0x313244;
        collection->base = 0x1e1e2e;
        collection->mantle = 0x181825;
        collection->crust = 0x11111b;
        return collection;
    }

    ColorCollection *generateFrappeColor(QObject *parent) {
        auto *collection = new ColorCollection(parent);
        collection->type = PaletteType::Frappe;
        collection->rosewater = 0xf2d5cf;
        collection->flamingo = 0xeebebe;
        collection->pink = 0xf4b8e4;
        collection->mauve = 0xca9ee6;
        collection->red = 0xe78284;
        collection->maroon = 0xea999c;
        collection->peach = 0xef9f76;
        collection->yellow = 0xe5c890;
        collection->green = 0xa6d189;
        collection->teal = 0x81c8be;
        collection->sky = 0x99d1db;
        collection->sapphire = 0x85c1dc;
        collection->blue = 0x8caaee;
        collection->lavender = 0xbabbf1;
        collection->text = 0xc6d0f5;
        collection->subtext1 = 0xb5bfe2;
        collection->subtext0 = 0xa5adce;
        collection->overlay2 = 0x949cbb;
        collection->overlay1 = 0x838ba7;
        collection->overlay0 = 0x737994;
        collection->surface2 = 0x626880;
        collection->surface1 = 0x51576d;
        collection->surface0 = 0x414559;
        collection->base = 0x303446;
        collection->mantle = 0x292c3c;
        collection->crust = 0x232634;
        return collection;
    }

    ColorCollection *generateMacchiatoColor(QObject *parent) {
        auto *collection = new ColorCollection(parent);
        collection->type = PaletteType::Macchiato;
        collection->rosewater = 0xf4dbd6;
        collection->flamingo = 0xf0c6c6;
        collection->pink = 0xf5bde6;
        collection->mauve = 0xc6a0f6;
        collection->red = 0xed8796;
        collection->maroon = 0xee99a0;
        collection->peach = 0xf5a97f;
        collection->yellow = 0xeed49f;
        collection->green = 0xa6da95;
        collection->teal = 0x8bd5ca;
        collection->sky = 0x91d7e3;
        collection->sapphire = 0x7dc4e4;
        collection->blue = 0x8aadf4;
        collection->lavender = 0xb7bdf8;
        collection->text = 0xcad3f5;
        collection->subtext1 = 0xb8c0e0;
        collection->subtext0 = 0xa5adcb;
        collection->overlay2 = 0x939ab7;
        collection->overlay1 = 0x8087a2;
        collection->overlay0 = 0x6e738d;
        collection->surface2 = 0x5b6078;
        collection->surface1 = 0x494d64;
        collection->surface0 = 0x363a4f;
        collection->base = 0x24273a;
        collection->mantle = 0x1e2030;
        collection->crust = 0x181926;
        return collection;
    }

    ColorCollection *generateLatteColor(QObject *parent) {
        auto *collection = new ColorCollection(parent);
        collection->type = PaletteType::Latte;
        collection->rosewater = 0xdc8a78;
        collection->flamingo = 0xdd7878;
        collection->pink = 0xea76cb;
        collection->mauve = 0x8839ef;
        collection->red = 0xd20f39;
        collection->maroon = 0xe64553;
        collection->peach = 0xfe640b;
        collection->yellow = 0xdf8e1d;
        collection->green = 0x40a02b;
        collection->teal = 0x179299;
        collection->sky = 0x04a5e5;
        collection->sapphire = 0x209fb5;
        collection->blue = 0x1e66f5;
        collection->lavender = 0x7287fd;
        collection->text = 0x4c4f69;
        collection->subtext1 = 0x5c5f77;
        collection->subtext0 = 0x6c6f85;
        collection->overlay2 = 0x7c7f93;
        collection->overlay1 = 0x8c8fa1;
        collection->overlay0 = 0x9ca0b0;
        collection->surface2 = 0xacb0be;
        collection->surface1 = 0xbcc0cc;
        collection->surface0 = 0xccd0da;
        collection->base = 0xeff1f5;
        collection->mantle = 0xe6e9ef;
        collection->crust = 0xdce0e8;
        return collection;
    }

    QColor brightColor(const QColor &sourceColor, PaletteType type) {
        // 保存原始透明度
        const auto alpha = sourceColor.alpha();

        // 使用HSV颜色空间获取更直观的颜色分量
        const QColor hsvColor = sourceColor.toHsv();
        int h = hsvColor.hue();
        const int s = hsvColor.saturation();
        const int v = hsvColor.value();

        // 处理无色相的情况
        if (h < 0)
            h = 0;

        QColor resultColor;

        switch (type) {
            case PaletteType::Latte:
                // Latte风格：提高亮度，保持色调
                {
                    const int newV = qBound(0, static_cast<int>(v * 1.09), 255);
                    const int newH = (h + 2) % 360;
                    resultColor.setHsv(newH, s, newV, alpha);
                }
                break;

            default:
                // Frappe/Macchiato/Mocha风格：降低亮度，增加饱和度
                {
                    const int newV = qBound(0, static_cast<int>(v * 0.94), 255);
                    const int newS = qBound(0, s + 8, 255);
                    const int newH = (h + 2) % 360;
                    resultColor.setHsv(newH, newS, newV, alpha);
                }
                break;
        }

        // 确保颜色有效
        if (!resultColor.isValid()) {
            qWarning() << "Invalid color generated, returning original color";
            return sourceColor;
        }

        return resultColor;
    }

        PaletteCollection *generatePaletteCollection(const PaletteType &type, const ColorCollection &color,
                             QObject *parent) {
        auto *collection = new PaletteCollection(parent);
        collection->backgroundPane = color.base;
        collection->secondaryPane = color.crust;
        collection->surfaceElement0 = color.surface0;
        collection->surfaceElement2 = color.surface1;
        collection->surfaceElement1 = color.surface2;
        collection->overlay0 = color.overlay0;
        collection->overlay1 = color.overlay1;
        collection->overlay2 = color.overlay2;
        collection->bodyCopy = color.text;
        collection->mainHeadline = color.text;
        collection->subHeadlines0 = color.subtext0;
        collection->subHeadlines1 = color.subtext1;
        collection->subtle = color.overlay1;
        collection->onAccent = color.base;
        collection->links = color.blue;
        collection->success = color.green;
        collection->warning = color.yellow;
        collection->error = color.red;
        collection->tags = color.blue;
        collection->selectionBackground =
            QColor{color.overlay0.red(), color.overlay0.green(), color.overlay0.blue(), 64};
        collection->cursor = color.rosewater;
        collection->cursorText = color.base;
        collection->activeBorder = color.lavender;
        collection->inactiveBorder = color.overlay0;
        collection->bellBorder = color.yellow;
        collection->color0 = color.subtext1;
        collection->color1 = color.red;
        collection->color2 = color.green;
        collection->color3 = color.yellow;
        collection->color4 = color.blue;
        collection->color5 = color.pink;
        collection->color6 = color.teal;
        collection->color7 = color.surface2;
        collection->color8 = color.subtext0;
        collection->color9 = brightColor(color.red, type);
        collection->color10 = brightColor(color.green, type);
        collection->color11 = brightColor(color.yellow, type);
        collection->color12 = brightColor(color.blue, type);
        collection->color13 = brightColor(color.pink, type);
        collection->color14 = brightColor(color.teal, type);
        collection->color15 = color.surface1;
        collection->color16 = color.peach;
        collection->color17 = color.rosewater;
        collection->mark1 = color.lavender;
        collection->mark2 = color.mauve;
        collection->mark3 = color.sapphire;
        collection->mark1Text = color.base;
        collection->mark2Text = color.base;
        collection->mark3Text = color.base;
        return collection;
    }

    NanColorAtla::NanColorAtla(QObject *parent) : QObject(parent) {
        colorCollections.insert(PaletteType::Frappe, generateFrappeColor(this));
        colorCollections.insert(PaletteType::Latte, generateLatteColor(this));
        colorCollections.insert(PaletteType::Macchiato, generateMacchiatoColor(this));
        colorCollections.insert(PaletteType::Mocha, generateMochaColor(this));

        paletteCollections.insert(
                PaletteType::Frappe,
            generatePaletteCollection(PaletteType::Frappe, *colorCollections[PaletteType::Frappe], this));

        paletteCollections.insert(PaletteType::Latte,
                      generatePaletteCollection(PaletteType::Latte, *colorCollections[PaletteType::Latte],
                                    this));

        paletteCollections.insert(
                PaletteType::Macchiato,
            generatePaletteCollection(PaletteType::Macchiato, *colorCollections[PaletteType::Macchiato], this));

        paletteCollections.insert(PaletteType::Mocha,
                      generatePaletteCollection(PaletteType::Mocha, *colorCollections[PaletteType::Mocha],
                                    this));
    };
} // namespace Nandina::NandinaColor
