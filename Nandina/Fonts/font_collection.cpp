//
// Created by cvrain on 2026/2/3.
//

#include "font_collection.hpp"

Nandina::Styles::FontCollection::FontCollection(QObject *parent) : NanSingleton(parent) {
    loadFont(FontFamily::JetBrainsMonoBold, ":/qt/qml/Nandina/Fonts/JetBrainsMono/JetBrainsMonoNerdFont-Bold.ttf",
             "JetBrains Mono");
    loadFont(FontFamily::JetBrainsMonoItalic, ":/qt/qml/Nandina/Fonts/JetBrainsMono/JetBrainsMonoNerdFont-Italic.ttf",
             "JetBrains Mono");
    loadFont(FontFamily::JetBrainsMonoLight, ":/qt/qml/Nandina/Fonts/JetBrainsMono/JetBrainsMonoNerdFont-Light.ttf",
             "JetBrains Mono");
    loadFont(FontFamily::JetBrainsMonoMedium, ":/qt/qml/Nandina/Fonts/JetBrainsMono/JetBrainsMonoNerdFont-Medium.ttf",
             "JetBrains Mono");
    loadFont(FontFamily::JetBrainsMonoRegular, ":/qt/qml/Nandina/Fonts/JetBrainsMono/JetBrainsMonoNerdFont-Regular.ttf",
             "JetBrains Mono");
    loadFont(FontFamily::SarasaBold, ":/qt/qml/Nandina/Fonts/Sarasa/Sarasa-Bold.ttc", "Sarasa");
    loadFont(FontFamily::SarasaItalic, ":/qt/qml/Nandina/Fonts/Sarasa/Sarasa-Italic.ttc", "Sarasa");
    loadFont(FontFamily::SarasaLight, ":/qt/qml/Nandina/Fonts/Sarasa/Sarasa-Light.ttc", "Sarasa");
    loadFont(FontFamily::SarasaRegular, ":/qt/qml/Nandina/Fonts/Sarasa/Sarasa-Regular.ttc", "Sarasa");
}

void Nandina::Styles::FontCollection::loadFont(FontFamily family, const QString &resourcePath,
                                               const QString &fallbackFamily) {
    const int fontId = QFontDatabase::addApplicationFont(resourcePath);
    if (fontId < 0) {
        fontFamilies.insert(family, fallbackFamily);
        return;
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (!families.isEmpty()) {
        fontFamilies.insert(family, families.first());
        loadedFamilies.insert(family);
    }
    else {
        fontFamilies.insert(family, fallbackFamily);
    }
}

QString Nandina::Styles::FontCollection::family(FontFamily family) const { return fontFamilies.value(family); }

bool Nandina::Styles::FontCollection::isLoaded(FontFamily family) const { return loadedFamilies.contains(family); }

QStringList Nandina::Styles::FontCollection::availableFamilies() const {
    QStringList result;
    result.reserve(fontFamilies.size());
    for (auto it = fontFamilies.cbegin(); it != fontFamilies.cend(); ++it) {
        if (!it.value().isEmpty()) {
            result.append(it.value());
        }
    }
    result.removeDuplicates();
    return result;
}
