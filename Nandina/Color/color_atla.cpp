//
// Created by cvrain on 2026/2/10.
//

#include "color_atla.hpp"

namespace Nandina::NandinaColor {

    ColorCollection generateMochaColor() {
        return ColorCollection{
                .type = PaletteType::Mocha,
                .rosewater = "#f5e0dc",
                .flamingo = "#f2cdcd",
                .pink = "#f5c2e7",
                .mauve = "#cba6f7",
                .red = "#f38ba8",
                .maroon = "#eba0ac",
                .peach = "#fab387",
                .yellow = "#f9e2af",
                .green = "#a6e3a1",
                .teal = "#94e2d5",
                .sky = "#89dceb",
                .sapphire = "#74c7ec",
                .blue = "#89b4fa",
                .lavender = "#b4befe",
                .text = "#cdd6f4",
                .subtext1 = "#bac2de",
                .subtext0 = "#a6adc8",
                .overlay2 = "#9399b2",
                .overlay1 = "#7f849c",
                .overlay0 = "#6c7086",
                .surface2 = "#585b70",
                .surface1 = "#45475a",
                .surface0 = "#313244",
                .base = "#1e1e2e",
                .mantle = "#181825",
                .crust = "#11111b",
        };
    }

    ColorCollection generateFrappeColor() {
        return ColorCollection{
                .type = PaletteType::Frappe,
                .rosewater = "#f2d5cf",
                .flamingo = "#eebebe",
                .pink = "#f4b8e4",
                .mauve = "#ca9ee6",
                .red = "#e78284",
                .maroon = "#ea999c",
                .peach = "#ef9f76",
                .yellow = "#e5c890",
                .green = "#a6d189",
                .teal = "#81c8be",
                .sky = "#99d1db",
                .sapphire = "#85c1dc",
                .blue = "#8caaee",
                .lavender = "#babbf1",
                .text = "#c6d0f5",
                .subtext1 = "#b5bfe2",
                .subtext0 = "#a5adce",
                .overlay2 = "#949cbb",
                .overlay1 = "#838ba7",
                .overlay0 = "#737994",
                .surface2 = "#626880",
                .surface1 = "#51576d",
                .surface0 = "#414559",
                .base = "#303446",
                .mantle = "#292c3c",
                .crust = "#232634",
        };
    }

    ColorCollection generateMacchiatoColor() {
        return ColorCollection{
                .type = PaletteType::Macchiato,
                .rosewater = "#f4dbd6",
                .flamingo = "#f0c6c6",
                .pink = "#f5bde6",
                .mauve = "#c6a0f6",
                .red = "#ed8796",
                .maroon = "#ee99a0",
                .peach = "#f5a97f",
                .yellow = "#eed49f",
                .green = "#a6da95",
                .teal = "#8bd5ca",
                .sky = "#91d7e3",
                .sapphire = "#7dc4e4",
                .blue = "#8aadf4",
                .lavender = "#b7bdf8",
                .text = "#cad3f5",
                .subtext1 = "#b8c0e0",
                .subtext0 = "#a5adcb",
                .overlay2 = "#939ab7",
                .overlay1 = "#8087a2",
                .overlay0 = "#6e738d",
                .surface2 = "#5b6078",
                .surface1 = "#494d64",
                .surface0 = "#363a4f",
                .base = "#24273a",
                .mantle = "#1e2030",
                .crust = "#181926",
        };
    }

    ColorCollection generateLatteColor() {
        return ColorCollection{
                .type = PaletteType::Latte,
                .rosewater = "#dc8a78",
                .flamingo = "#dd7878",
                .pink = "#ea76cb",
                .mauve = "#8839ef",
                .red = "#d20f39",
                .maroon = "#e64553",
                .peach = "#fe640b",
                .yellow = "#df8e1d",
                .green = "#40a02b",
                .teal = "#179299",
                .sky = "#04a5e5",
                .sapphire = "#209fb5",
                .blue = "#1e66f5",
                .lavender = "#7287fd",
                .text = "#4c4f69",
                .subtext1 = "#5c5f77",
                .subtext0 = "#6c6f85",
                .overlay2 = "#7c7f93",
                .overlay1 = "#8c8fa1",
                .overlay0 = "#9ca0b0",
                .surface2 = "#acb0be",
                .surface1 = "#bcc0cc",
                .surface0 = "#ccd0da",
                .base = "#eff1f5",
                .mantle = "#e6e9ef",
                .crust = "#dce0e8",
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
        colorCollections.insert(PaletteType::Frappe, std::move(generateFrappeColor()));
        colorCollections.insert(PaletteType::Latte, std::move(generateLatteColor()));
        colorCollections.insert(PaletteType::Macchiato, std::move(generateMacchiatoColor()));
        colorCollections.insert(PaletteType::Mocha, std::move(generateMochaColor()));

        paletteCollections.insert(
                PaletteType::Frappe,
                std::move(generatePaletteCollection(PaletteType::Frappe, colorCollections[PaletteType::Frappe])));

        paletteCollections.insert(
                PaletteType::Latte,
                std::move(generatePaletteCollection(PaletteType::Latte, colorCollections[PaletteType::Latte])));

        paletteCollections.insert(
                PaletteType::Macchiato,
                std::move(generatePaletteCollection(PaletteType::Macchiato, colorCollections[PaletteType::Macchiato])));

        paletteCollections.insert(
                PaletteType::Mocha,
                std::move(generatePaletteCollection(PaletteType::Mocha, colorCollections[PaletteType::Mocha])));
    };
} // namespace Nandina::NandinaColor
