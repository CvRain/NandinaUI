//
// Created by cvrain on 2026/3/2.
//

#include "color_factory.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <array>

#include "theme_registry.hpp"

namespace Nandina::Core::Color {

    namespace {

        constexpr std::array<const char *, VariantCount> kVariantNames = {
                "primary",
                "secondary",
                "tertiary",
                "success",
                "warning",
                "error",
                "surface",
        };

        constexpr std::array<int, AccentCount> kShadeOrder = {
                50,
                100,
                200,
                300,
                400,
                500,
                600,
                700,
                800,
                900,
                950,
        };

        QString cssColorTokenName(const char *variantName, const int shade) {
            return QStringLiteral("--color-%1-%2").arg(QLatin1StringView(variantName)).arg(shade);
        }

        [[noreturn]] void failThemeLoad(const QString &message) {
            qFatal("%s", qPrintable(QStringLiteral("ColorFactory theme loading failed: %1").arg(message)));
        }

        ThemeShadeArray
        parseVariantColors(const QJsonObject &colorsObject, const QString &themeName, const char *variantName) {
            ThemeShadeArray shades;
            for (int index = 0; index < AccentCount; ++index) {
                const auto tokenName = cssColorTokenName(variantName, kShadeOrder[index]);
                const auto colorValue = colorsObject.value(tokenName);
                if (!colorValue.isString()) {
                    failThemeLoad(QStringLiteral("theme '%1' is missing string token '%2'").arg(themeName, tokenName));
                }

                const auto colorName = colorValue.toString();
                const QColor color(colorName);
                if (!color.isValid()) {
                    failThemeLoad(QStringLiteral("theme '%1' token '%2' has invalid color '%3'")
                                          .arg(themeName, tokenName, colorName));
                }
                shades[index] = color;
            }

            return shades;
        }

        ThemeColorData parseThemeData(const Types::ThemeVariant::ThemeTypes theme) {
            const auto &registration = Types::ThemeRegistry::registration(theme);
            const auto themeName = QString::fromLatin1(registration.name);
            const auto resourcePath = Types::ThemeRegistry::colorResourcePath(theme);

            QFile themeFile(resourcePath);
            if (!themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                failThemeLoad(QStringLiteral("cannot open %1 for theme '%2'").arg(resourcePath, themeName));
            }

            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(themeFile.readAll(), &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                failThemeLoad(QStringLiteral("invalid JSON in %1: %2 at offset %3")
                                      .arg(resourcePath, parseError.errorString())
                                      .arg(parseError.offset));
            }

            const auto rootObject = document.object();
            const auto themeField = rootObject.value(QStringLiteral("theme"));
            if (!themeField.isString() || themeField.toString() != themeName) {
                failThemeLoad(QStringLiteral("resource %1 must declare theme '%2'").arg(resourcePath, themeName));
            }

            const auto colorsValue = rootObject.value(QStringLiteral("colors"));
            if (!colorsValue.isObject()) {
                failThemeLoad(
                        QStringLiteral("resource %1 must contain an object field named 'colors'").arg(resourcePath));
            }

            const auto colorsObject = colorsValue.toObject();
            return {
                    parseVariantColors(colorsObject, themeName, kVariantNames[0]),
                    parseVariantColors(colorsObject, themeName, kVariantNames[1]),
                    parseVariantColors(colorsObject, themeName, kVariantNames[2]),
                    parseVariantColors(colorsObject, themeName, kVariantNames[3]),
                    parseVariantColors(colorsObject, themeName, kVariantNames[4]),
                    parseVariantColors(colorsObject, themeName, kVariantNames[5]),
                    parseVariantColors(colorsObject, themeName, kVariantNames[6]),
            };
        }

        std::array<ThemeColorData, Types::ThemeVariant::ThemeCount> loadThemeDataStore() {
            std::array<ThemeColorData, Types::ThemeVariant::ThemeCount> themeData;
            for (const auto &entry: Types::ThemeRegistry::Registrations) {
                const auto theme = entry.type;
                themeData[static_cast<int>(theme)] = parseThemeData(theme);
            }
            return themeData;
        }

        const std::array<ThemeColorData, Types::ThemeVariant::ThemeCount> &themeDataStore() {
            static const auto s_themeData = loadThemeDataStore();
            return s_themeData;
        }

    } // namespace

    void ColorFactory::applyTheme(const Types::ThemeVariant::ThemeTypes theme, const bool isDark, ColorSchema *schema) {
        const auto &data = getThemeData(theme);

        for (int v = 0; v < VariantCount; ++v) {
            applyVariant(data[v], isDark, schema->palette(v));
        }
    }

    const ThemeColorData &ColorFactory::getThemeData(const Types::ThemeVariant::ThemeTypes theme) {
        return themeDataStore()[static_cast<int>(theme)];
    }

    void ColorFactory::applyVariant(const ThemeShadeArray &colors, const bool isDark, ColorPalette *palette) {
        std::array<QColor, AccentCount> shades;
        for (int i = 0; i < AccentCount; ++i) {
            // Dark mode reverses the shade order (50↔950, 100↔900, …)
            const int src = isDark ? (AccentCount - 1 - i) : i;
            shades[i] = colors[src];
        }
        palette->setAllShades(shades);
    }

} // namespace Nandina::Core::Color
