#include "themeManager.hpp"
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "Utils/file_operator.hpp"
#include "json_parser.hpp"

using namespace Nandina;

ThemeManager *ThemeManager::instance = nullptr;

ThemeManager *ThemeManager::create(const QQmlEngine *qmlEngine, const QJSEngine *jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    // QML 单例创建时，QQmlEngine 会负责管理其生命周期
    // 不需要指定 parent，QML 引擎会自动管理
    return getInstance(nullptr);
}

ThemeManager *ThemeManager::getInstance(QObject *parent) {
    if (instance == nullptr) {
        // 如果没有提供 parent，使用 QCoreApplication 实例作为父对象
        // 这样程序退出时会自动清理
        QObject *parentObj = parent ? parent : QCoreApplication::instance();
        instance = new ThemeManager(parentObj);
    }
    return instance;
}

QString ThemeManager::getColorByString(const QString &string) const {
    if (string == "rosewater") {
        return getColor()->rosewater;
    }
    if (string == "flamingo") {
        return getColor()->flamingo;
    }
    if (string == "pink") {
        return getColor()->pink;
    }
    if (string == "mauve") {
        return getColor()->mauve;
    }
    if (string == "red") {
        return getColor()->red;
    }
    if (string == "maroon") {
        return getColor()->maroon;
    }
    if (string == "peach") {
        return getColor()->peach;
    }
    if (string == "yellow") {
        return getColor()->yellow;
    }
    if (string == "green") {
        return getColor()->green;
    }
    if (string == "teal") {
        return getColor()->teal;
    }
    if (string == "sky") {
        return getColor()->sky;
    }
    if (string == "sapphire") {
        return getColor()->sapphire;
    }
    if (string == "blue") {
        return getColor()->blue;
    }
    if (string == "lavender") {
        return getColor()->lavender;
    }
    if (string == "text") {
        return getColor()->text;
    }
    if (string == "subtext1") {
        return getColor()->subtext1;
    }
    if (string == "subtext0") {
        return getColor()->subtext0;
    }
    if (string == "overlay2") {
        return getColor()->overlay2;
    }
    if (string == "overlay1") {
        return getColor()->overlay1;
    }
    if (string == "overlay0") {
        return getColor()->overlay0;
    }
    if (string == "surface2") {
        return getColor()->surface2;
    }
    if (string == "surface1") {
        return getColor()->surface1;
    }
    if (string == "surface0") {
        return getColor()->surface0;
    }
    if (string == "base") {
        return getColor()->base;
    }
    if (string == "mantle") {
        return getColor()->mantle;
    }
    if (string == "crust") {
        return getColor()->crust;
    }
    return getColor()->base;
}

Core::Types::CatppuccinSetting::CatppuccinType ThemeManager::getCurrentPaletteType() const {
    return currentPaletteType;
}

void ThemeManager::setCurrentPaletteType(const Core::Types::CatppuccinSetting::CatppuccinType type) {
    if (baseColors.contains(type)) {
        this->currentPaletteType = type;
        this->currentBaseColors = &this->baseColors.at(type);
        emit paletteChanged(type);
    }
}

BaseColors *ThemeManager::getColor() const { return this->currentBaseColors; }


ThemeManager::ThemeManager(QObject *parent) :
    QObject(parent), currentPaletteType(Core::Types::CatppuccinSetting::CatppuccinType::Latte) {
    loadBaseColor();
    currentBaseColors = &baseColors.at(currentPaletteType);
}

void ThemeManager::loadBaseColor() {
    using namespace Nandina::Core::Types;
    const std::map<CatppuccinSetting::CatppuccinType, QString> baseColorUrl{
            {CatppuccinSetting::CatppuccinType::Latte, ":/qt/qml/Nandina/Theme/Resources/Palettes/Latte.json"},
            {CatppuccinSetting::CatppuccinType::Frappe, ":/qt/qml/Nandina/Theme/Resources/Palettes/Frappe.json"},
            {CatppuccinSetting::CatppuccinType::Macchiato, ":/qt/qml/Nandina/Theme/Resources/Palettes/Macchiato.json"},
            {CatppuccinSetting::CatppuccinType::Mocha, ":/qt/qml/Nandina/Theme/Resources/Palettes/Mocha.json"}};

    const auto instantiatePalette = [&](const CatppuccinSetting::CatppuccinType type, const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Couldn't open palette file:" << filePath;
            throw std::runtime_error("Couldn't open palette file");
        }

        QJsonParseError error{};
        const auto jsonDoc = QJsonDocument::fromJson(file.readAll(), &error);
        if (jsonDoc.isNull()) {
            qWarning() << "Failed to parse palette file:" << filePath << "Error:" << error.errorString();
            throw std::runtime_error("Failed to parse palette file");
        }

        const auto newBaseColors = Core::Utils::JsonParser::parser<BaseColors>(jsonDoc.object());
        baseColors.insert(std::make_pair(type, newBaseColors));
    };

    for (const auto &[fst, snd]: baseColorUrl) {
        instantiatePalette(fst, snd);
    }
}
