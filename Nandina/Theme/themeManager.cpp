#include "themeManager.hpp"
#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "json_parser.hpp"
#include "Utils/file_operator.hpp"

using namespace Nandina;

ThemeManager *ThemeManager::instance = nullptr;

ThemeManager* ThemeManager::create(const QQmlEngine *qmlEngine,
                                   const QJSEngine *jsEngine) {
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

Core::Types::CatppuccinSetting::CatppuccinType
ThemeManager::getCurrentPaletteType() const {
    return currentPaletteType;
}

void ThemeManager::setCurrentPaletteType(
    const Core::Types::CatppuccinSetting::CatppuccinType type) {
    if (baseColors.contains(type)) {
        this->currentPaletteType = type;
        this->currentBaseColors = &this->baseColors.at(type);
        emit paletteChanged(type);
    }
}

BaseColors* ThemeManager::getColor() const { return this->currentBaseColors; }


ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent),
      currentPaletteType(
          Core::Types::CatppuccinSetting::CatppuccinType::Latte) {
    loadBaseColor();
    currentBaseColors = &baseColors.at(currentPaletteType);
}

void ThemeManager::loadBaseColor() {
    using namespace Nandina::Core::Types;
    const std::map<CatppuccinSetting::CatppuccinType, QString> baseColorUrl{
        {
            CatppuccinSetting::CatppuccinType::Latte,
            ":/qt/qml/Nandina/Theme/Resources/Palettes/Latte.json"
        },
        {
            CatppuccinSetting::CatppuccinType::Frappe,
            ":/qt/qml/Nandina/Theme/Resources/Palettes/Frappe.json"
        },
        {
            CatppuccinSetting::CatppuccinType::Macchiato,
            ":/qt/qml/Nandina/Theme/Resources/Palettes/Macchiato.json"
        },
        {
            CatppuccinSetting::CatppuccinType::Mocha,
            ":/qt/qml/Nandina/Theme/Resources/Palettes/Mocha.json"
        }
    };

    const auto instantiatePalette =
            [&](const CatppuccinSetting::CatppuccinType type,
                const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Couldn't open palette file:" << filePath;
            throw std::runtime_error("Couldn't open palette file");
        }

        QJsonParseError error{};
        const auto jsonDoc = QJsonDocument::fromJson(file.readAll(), &error);
        if (jsonDoc.isNull()) {
            qWarning() << "Failed to parse palette file:" << filePath
                    << "Error:" << error.errorString();
            throw std::runtime_error("Failed to parse palette file");
        }

        const auto newBaseColors = Core::Utils::JsonParser::parser<BaseColors>(jsonDoc.object());
        baseColors.insert(std::make_pair(type, newBaseColors));
    };

    for (const auto &[fst, snd]: baseColorUrl) {
        instantiatePalette(fst, snd);
    }
}
