//
// Created by cvrain on 2026/3/2.
//

#include "primitive_factory.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <QString>
#include <array>

#include "color_utils.hpp"
#include "font_manager.hpp"
#include "theme_registry.hpp"

namespace Nandina::Core::Primitives {

    // ═══════════════════════════════════════════════════════════════
    //  Internal helper: per-theme primitive data struct
    //  Values are pre-converted from CSS rem/px to device-independent px.
    //  Conversion: 1rem = 16px (standard browser default).
    // ═══════════════════════════════════════════════════════════════

    // Font family is resolved at runtime from FontManager (not stored in
    // per-theme data) because family names depend on what QFontDatabase
    // actually registered — not on a hardcoded string.
    struct TypographyData {
        int fontWeight; // QFont::Weight compatible
        bool italic;
        qreal letterSpacing;
        uint32_t fontColorRgba; // 0xRRGGBBFF
        uint32_t fontColorDarkRgba; // 0xRRGGBBFF
    };

    struct ThemePrimitiveData {
        qreal spacing;
        qreal textScaling;
        qreal radiusBase;
        qreal radiusContainer;
        qreal borderWidth;
        qreal divideWidth;
        qreal ringWidth;
        uint32_t focusRingColorRgba;
        uint32_t focusRingColorDarkRgba;
        uint32_t bodyBackgroundColorRgba;
        uint32_t bodyBackgroundColorDarkRgba;
        TypographyData baseFont;
        TypographyData headingFont;
        TypographyData anchorFont;
    };

    // ── Helper: RGBA → QColor (delegates to Core::rgbaToQColor) ───
    using Core::rgbaToQColor;

    [[noreturn]] void failPrimitiveLoad(const QString &message) {
        qFatal("%s", qPrintable(QStringLiteral("PrimitiveFactory theme loading failed: %1").arg(message)));
    }

    [[nodiscard]] qreal requiredNumber(const QJsonObject &object, const QString &key, const QString &resourcePath) {
        const auto value = object.value(key);
        if (!value.isDouble()) {
            failPrimitiveLoad(QStringLiteral("resource %1 is missing numeric token '%2'").arg(resourcePath, key));
        }
        return value.toDouble();
    }

    [[nodiscard]] int requiredInt(const QJsonObject &object, const QString &key, const QString &resourcePath) {
        return qRound(requiredNumber(object, key, resourcePath));
    }

    [[nodiscard]] bool requiredBool(const QJsonObject &object, const QString &key, const QString &resourcePath) {
        const auto value = object.value(key);
        if (!value.isBool()) {
            failPrimitiveLoad(QStringLiteral("resource %1 is missing boolean token '%2'").arg(resourcePath, key));
        }
        return value.toBool();
    }

    [[nodiscard]] uint32_t
    requiredColorRgba(const QJsonObject &object, const QString &key, const QString &resourcePath) {
        const auto value = object.value(key);
        if (!value.isString()) {
            failPrimitiveLoad(QStringLiteral("resource %1 is missing color token '%2'").arg(resourcePath, key));
        }

        const QColor color(value.toString());
        if (!color.isValid()) {
            failPrimitiveLoad(QStringLiteral("resource %1 token '%2' has invalid color '%3'")
                                      .arg(resourcePath, key, value.toString()));
        }
        return Core::qColorToRgba(color);
    }

    [[nodiscard]] TypographyData
    parseTypography(const QJsonObject &primitivesObject, const QString &resourcePath, const QString &prefix) {
        return {
                requiredInt(primitivesObject, QStringLiteral("--%1-font-weight").arg(prefix), resourcePath),
                requiredBool(primitivesObject, QStringLiteral("--%1-font-italic").arg(prefix), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--%1-font-letter-spacing").arg(prefix), resourcePath),
                requiredColorRgba(primitivesObject, QStringLiteral("--%1-font-color").arg(prefix), resourcePath),
                requiredColorRgba(primitivesObject, QStringLiteral("--%1-font-color-dark").arg(prefix), resourcePath),
        };
    }

    [[nodiscard]] ThemePrimitiveData parseThemePrimitiveData(const Types::ThemeVariant::ThemeTypes theme) {
        const auto &registration = Types::ThemeRegistry::registration(theme);
        const auto resourcePath = Types::ThemeRegistry::primitiveResourcePath(theme);
        const auto themeName = QString::fromLatin1(registration.name);

        QFile themeFile(resourcePath);
        if (!themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            failPrimitiveLoad(QStringLiteral("cannot open %1 for theme '%2'").arg(resourcePath, themeName));
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(themeFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            failPrimitiveLoad(QStringLiteral("invalid JSON in %1: %2 at offset %3")
                                      .arg(resourcePath, parseError.errorString())
                                      .arg(parseError.offset));
        }

        const auto rootObject = document.object();
        const auto themeField = rootObject.value(QStringLiteral("theme"));
        if (!themeField.isString() || themeField.toString() != themeName) {
            failPrimitiveLoad(QStringLiteral("resource %1 must declare theme '%2'").arg(resourcePath, themeName));
        }

        const auto primitivesValue = rootObject.value(QStringLiteral("primitives"));
        if (!primitivesValue.isObject()) {
            failPrimitiveLoad(
                    QStringLiteral("resource %1 must contain an object field named 'primitives'").arg(resourcePath));
        }

        const auto primitivesObject = primitivesValue.toObject();
        return {
                requiredNumber(primitivesObject, QStringLiteral("--spacing"), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--text-scaling"), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--radius-base"), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--radius-container"), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--border-width"), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--divide-width"), resourcePath),
                requiredNumber(primitivesObject, QStringLiteral("--ring-width"), resourcePath),
                requiredColorRgba(primitivesObject, QStringLiteral("--focus-ring-color"), resourcePath),
                requiredColorRgba(primitivesObject, QStringLiteral("--focus-ring-color-dark"), resourcePath),
                requiredColorRgba(primitivesObject, QStringLiteral("--body-background-color"), resourcePath),
                requiredColorRgba(primitivesObject, QStringLiteral("--body-background-color-dark"), resourcePath),
                parseTypography(primitivesObject, resourcePath, QStringLiteral("base")),
                parseTypography(primitivesObject, resourcePath, QStringLiteral("heading")),
                parseTypography(primitivesObject, resourcePath, QStringLiteral("anchor")),
        };
    }

    [[nodiscard]] const std::array<ThemePrimitiveData, Types::ThemeVariant::ThemeCount> &themePrimitiveStore() {
        static const auto s_themePrimitives = [] {
            std::array<ThemePrimitiveData, Types::ThemeVariant::ThemeCount> themeData;
            for (const auto &entry: Types::ThemeRegistry::Registrations) {
                themeData[static_cast<int>(entry.type)] = parseThemePrimitiveData(entry.type);
            }
            return themeData;
        }();

        return s_themePrimitives;
    }

    static void applyTypography(const TypographyData &data, TypographySchema *schema, const QString &fontFamily) {
        schema->setFontFamily(fontFamily);
        schema->setFontWeight(data.fontWeight);
        schema->setItalic(data.italic);
        schema->setLetterSpacing(data.letterSpacing);
        schema->setFontColor(rgbaToQColor(data.fontColorRgba));
        schema->setFontColorDark(rgbaToQColor(data.fontColorDarkRgba));
    }

    // ═══════════════════════════════════════════════════════════════
    //  Public API
    // ═══════════════════════════════════════════════════════════════

    void PrimitiveFactory::applyTheme(const Types::ThemeVariant::ThemeTypes theme, PrimitiveSchema *primitives) {
        const auto &d = themePrimitiveStore()[static_cast<int>(theme)];

        // Layout tokens
        primitives->setSpacing(d.spacing);
        primitives->setTextScaling(d.textScaling);

        // Radius tokens
        primitives->setRadiusBase(d.radiusBase);
        primitives->setRadiusContainer(d.radiusContainer);

        // Border tokens
        primitives->setBorderWidth(d.borderWidth);
        primitives->setDivideWidth(d.divideWidth);
        primitives->setRingWidth(d.ringWidth);
        primitives->setFocusRingColor(rgbaToQColor(d.focusRingColorRgba));
        primitives->setFocusRingColorDark(rgbaToQColor(d.focusRingColorDarkRgba));

        // Background colors
        primitives->setBodyBackgroundColor(rgbaToQColor(d.bodyBackgroundColorRgba));
        primitives->setBodyBackgroundColorDark(rgbaToQColor(d.bodyBackgroundColorDarkRgba));

        // Typography — font family resolved from FontManager at runtime
        const QString defaultFamily = Fonts::FontManager::resolvedDefaultFamily();
        applyTypography(d.baseFont, primitives->baseFont(), defaultFamily);
        applyTypography(d.headingFont, primitives->headingFont(), defaultFamily);
        applyTypography(d.anchorFont, primitives->anchorFont(), defaultFamily);
    }

} // namespace Nandina::Core::Primitives
