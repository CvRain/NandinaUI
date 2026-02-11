//
// Created by cvrain on 2026/2/10.
//

#include "color_atla.hpp"

namespace Nandina::NandinaColor {

    ColorCollection generateMochaColor() {
        return ColorCollection{
                .type = PaletteType::Mocha,
                .rosewater = 0xf5e0dc,
                .flamingo = 0xf2cdcd,
                .pink = 0xf5c2e7,
                .mauve = 0xcba6f7,
                .red = 0xf38ba8,
                .maroon = 0xeba0ac,
                .peach = 0xfab387,
                .yellow = 0xf9e2af,
                .green = 0xa6e3a1,
                .teal = 0x94e2d5,
                .sky = 0x89dceb,
                .sapphire = 0x74c7ec,
                .blue = 0x89b4fa,
                .lavender = 0xb4befe,
                .text = 0xcdd6f4,
                .subtext1 = 0xbac2de,
                .subtext0 = 0xa6adc8,
                .overlay2 = 0x9399b2,
                .overlay1 = 0x7f849c,
                .overlay0 = 0x6c7086,
                .surface2 = 0x585b70,
                .surface1 = 0x45475a,
                .surface0 = 0x313244,
                .base = 0x1e1e2e,
                .mantle = 0x181825,
                .crust = 0x11111b,
        };
    }

    ColorCollection generateFrappeColor() {
        return ColorCollection{
                .type = PaletteType::Frappe,
                .rosewater = 0xf2d5cf,
                .flamingo = 0xeebebe,
                .pink = 0xf4b8e4,
                .mauve = 0xca9ee6,
                .red = 0xe78284,
                .maroon = 0xea999c,
                .peach = 0xef9f76,
                .yellow = 0xe5c890,
                .green = 0xa6d189,
                .teal = 0x81c8be,
                .sky = 0x99d1db,
                .sapphire = 0x85c1dc,
                .blue = 0x8caaee,
                .lavender = 0xbabbf1,
                .text = 0xc6d0f5,
                .subtext1 = 0xb5bfe2,
                .subtext0 = 0xa5adce,
                .overlay2 = 0x949cbb,
                .overlay1 = 0x838ba7,
                .overlay0 = 0x737994,
                .surface2 = 0x626880,
                .surface1 = 0x51576d,
                .surface0 = 0x414559,
                .base = 0x303446,
                .mantle = 0x292c3c,
                .crust = 0x232634,
        };
    }

    ColorCollection generateMacchiatoColor() {
        return ColorCollection{
                .type = PaletteType::Macchiato,
                .rosewater = 0xf4dbd6,
                .flamingo = 0xf0c6c6,
                .pink = 0xf5bde6,
                .mauve = 0xc6a0f6,
                .red = 0xed8796,
                .maroon = 0xee99a0,
                .peach = 0xf5a97f,
                .yellow = 0xeed49f,
                .green = 0xa6da95,
                .teal = 0x8bd5ca,
                .sky = 0x91d7e3,
                .sapphire = 0x7dc4e4,
                .blue = 0x8aadf4,
                .lavender = 0xb7bdf8,
                .text = 0xcad3f5,
                .subtext1 = 0xb8c0e0,
                .subtext0 = 0xa5adcb,
                .overlay2 = 0x939ab7,
                .overlay1 = 0x8087a2,
                .overlay0 = 0x6e738d,
                .surface2 = 0x5b6078,
                .surface1 = 0x494d64,
                .surface0 = 0x363a4f,
                .base = 0x24273a,
                .mantle = 0x1e2030,
                .crust = 0x181926,
        };
    }

    ColorCollection generateLatteColor() {
        return ColorCollection{
                .type = PaletteType::Latte,
                .rosewater = 0xdc8a78,
                .flamingo = 0xdd7878,
                .pink = 0xea76cb,
                .mauve = 0x8839ef,
                .red = 0xd20f39,
                .maroon = 0xe64553,
                .peach = 0xfe640b,
                .yellow = 0xdf8e1d,
                .green = 0x40a02b,
                .teal = 0x179299,
                .sky = 0x04a5e5,
                .sapphire = 0x209fb5,
                .blue = 0x1e66f5,
                .lavender = 0x7287fd,
                .text = 0x4c4f69,
                .subtext1 = 0x5c5f77,
                .subtext0 = 0x6c6f85,
                .overlay2 = 0x7c7f93,
                .overlay1 = 0x8c8fa1,
                .overlay0 = 0x9ca0b0,
                .surface2 = 0xacb0be,
                .surface1 = 0xbcc0cc,
                .surface0 = 0xccd0da,
                .base = 0xeff1f5,
                .mantle = 0xe6e9ef,
                .crust = 0xdce0e8,
        };
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

    PaletteCollection generatePaletteCollection(const PaletteType &type, const auto &color) {
        return PaletteCollection{
                .backgroundPane = color.base,
                .secondaryPane = color.crust,
                .surfaceElement0 = color.surface0,
                .surfaceElement2 = color.surface1,
                .surfaceElement1 = color.surface2,
                .overlay0 = color.overlay0,
                .overlay1 = color.overlay1,
                .overlay2 = color.overlay2,
                .bodyCopy = color.text,
                .mainHeadline = color.text,
                .subHeadlines0 = color.subtext0,
                .subHeadlines1 = color.subtext1,
                .subtle = color.overlay1,
                .onAccent = color.base,
                .links = color.blue,
                .success = color.green,
                .warning = color.yellow,
                .error = color.red,
                .tags = color.blue,
                .selectionBackground = QColor{color.overlay0.red(), color.overlay0.green(), color.overlay0.blue(), 64},
                .cursor = color.rosewater,
                .cursorText = color.base,
                .activeBorder = color.lavender,
                .inactiveBorder = color.overlay0,
                .bellBorder = color.yellow,
                .color0 = color.subtext1,
                .color1 = color.red,
                .color2 = color.green,
                .color3 = color.yellow,
                .color4 = color.blue,
                .color5 = color.pink,
                .color6 = color.teal,
                .color7 = color.surface2,
                .color8 = color.subtext0,
                .color9 = brightColor(color.red, type),
                .color10 = brightColor(color.green, type),
                .color11 = brightColor(color.yellow, type),
                .color12 = brightColor(color.blue, type),
                .color13 = brightColor(color.pink, type),
                .color14 = brightColor(color.teal, type),
                .color15 = color.surface1,
                .color16 = color.peach,
                .color17 = color.rosewater,
                .mark1 = color.lavender,
                .mark2 = color.mauve,
                .mark3 = color.sapphire,
                .mark1Text = color.base,
                .mark2Text = color.base,
                .mark3Text = color.base,
        };
    }

    NanColorAtla::NanColorAtla(QObject *parent) : QObject(parent) {
        colorCollections.insert(PaletteType::Frappe, generateFrappeColor());
        colorCollections.insert(PaletteType::Latte, generateLatteColor());
        colorCollections.insert(PaletteType::Macchiato, generateMacchiatoColor());
        colorCollections.insert(PaletteType::Mocha, generateMochaColor());

        paletteCollections.insert(
                PaletteType::Frappe,
                generatePaletteCollection(PaletteType::Frappe, colorCollections[PaletteType::Frappe]));

        paletteCollections.insert(PaletteType::Latte,
                                  generatePaletteCollection(PaletteType::Latte, colorCollections[PaletteType::Latte]));

        paletteCollections.insert(
                PaletteType::Macchiato,
                generatePaletteCollection(PaletteType::Macchiato, colorCollections[PaletteType::Macchiato]));

        paletteCollections.insert(PaletteType::Mocha,
                                  generatePaletteCollection(PaletteType::Mocha, colorCollections[PaletteType::Mocha]));
    };
} // namespace Nandina::NandinaColor
