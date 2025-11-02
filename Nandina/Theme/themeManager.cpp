#include "themeManager.hpp"
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QFileInfo>

using namespace Nandina;

ThemeManager *ThemeManager::instance = nullptr;

namespace {
    ColorSet::CatppuccinType paletteTypeFromString(const QString& name) {
        if (name.compare("latte", Qt::CaseInsensitive) == 0) return ColorSet::CatppuccinType::Latte;
        if (name.compare("frappe", Qt::CaseInsensitive) == 0) return ColorSet::CatppuccinType::Frappe;
        if (name.compare("macchiato", Qt::CaseInsensitive) == 0) return ColorSet::CatppuccinType::Macchiato;
        if (name.compare("mocha", Qt::CaseInsensitive) == 0) return ColorSet::CatppuccinType::Mocha;
        return ColorSet::CatppuccinType::Latte; // Default
    }
}

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

ColorSet::CatppuccinType ThemeManager::getCurrentPaletteType() const {
    return currentPaletteType;
}

void ThemeManager::setCurrentPaletteType(const ColorSet::CatppuccinType type) {
    if (baseColors.count(type)) {
        this->currentPaletteType = type;
        this->currentBaseColors = &this->baseColors.at(type);
        emit paletteChanged(type);
    }
}

BaseColors* ThemeManager::getColor() const {
    return this->currentBaseColors;
}

// QVariant ThemeManager::getComponentStyle(const QString &stylePath) const {
//     QStringList path = stylePath.split('.');
//     if (path.isEmpty()) {
//         return {};
//     }
//
//     QJsonValue currentValue = componentStyles.value(path.first());
//     for (int i = 1; i < path.size(); ++i) {
//         if (!currentValue.isObject()) {
//             return {};
//         }
//         currentValue = currentValue.toObject().value(path[i]);
//     }
//
//     return currentValue.toVariant();
// }

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), currentPaletteType(ColorSet::CatppuccinType::Latte) {

    QDirIterator it(":/qt/qml/Nandina/Theme/Palettes", QDir::Files | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Couldn't open palette file:" << filePath;
            continue;
        }

        QJsonParseError error{};
        QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &error);
        if (jsonDoc.isNull()) {
            qWarning() << "Failed to parse palette file:" << filePath << "Error:" << error.errorString();
            continue;
        }

        QJsonObject p = jsonDoc.object();
        QFileInfo fileInfo(filePath);
        QString paletteName = fileInfo.baseName();
        ColorSet::CatppuccinType type = paletteTypeFromString(paletteName);

        // Manual population for BaseColors
        BaseColors newBaseColors;
        newBaseColors.rosewater = p["rosewater"].toString();
        newBaseColors.flamingo = p["flamingo"].toString();
        newBaseColors.pink = p["pink"].toString();
        newBaseColors.mauve = p["mauve"].toString();
        newBaseColors.red = p["red"].toString();
        newBaseColors.maroon = p["maroon"].toString();
        newBaseColors.peach = p["peach"].toString();
        newBaseColors.yellow = p["yellow"].toString();
        newBaseColors.green = p["green"].toString();
        newBaseColors.teal = p["teal"].toString();
        newBaseColors.sky = p["sky"].toString();
        newBaseColors.sapphire = p["sapphire"].toString();
        newBaseColors.blue = p["blue"].toString();
        newBaseColors.lavender = p["lavender"].toString();
        newBaseColors.text = p["text"].toString();
        newBaseColors.subtext1 = p["subtext1"].toString();
        newBaseColors.subtext0 = p["subtext0"].toString();
        newBaseColors.overlay2 = p["overlay2"].toString();
        newBaseColors.overlay1 = p["overlay1"].toString();
        newBaseColors.overlay0 = p["overlay0"].toString();
        newBaseColors.surface2 = p["surface2"].toString();
        newBaseColors.surface1 = p["surface1"].toString();
        newBaseColors.surface0 = p["surface0"].toString();
        newBaseColors.base = p["base"].toString();
        newBaseColors.mantle = p["mantle"].toString();
        newBaseColors.crust = p["crust"].toString();
        baseColors.insert(std::make_pair(type, newBaseColors));
    }

    if (!baseColors.empty()) {
        this->currentBaseColors = &this->baseColors.at(currentPaletteType);
    } else {
        qWarning() << "No palettes were loaded. ThemeManager will not function correctly.";
        this->currentBaseColors = nullptr;
    }

    loadComponentStyles();
}

void ThemeManager::loadComponentStyles() {
    QDirIterator it(":/qt/qml/Nandina/Theme/Components", QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while(it.hasNext()){
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Couldn't open component style file:" << filePath;
            continue;
        }

        QJsonParseError error{};
        QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &error);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            qWarning() << "Failed to parse component style file:" << filePath << "Error:" << error.errorString();
            continue;
        }

        QJsonObject styleObj = jsonDoc.object();
        QString target = styleObj.value("target").toString();
        if (!target.isEmpty()) {
            componentStyles[target] = styleObj;
        }
    }
}
