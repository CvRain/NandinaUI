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
    // 使用静态 QHash 实现 O(1) 查找，只初始化一次
    static const QHash<QString, std::function<QString(const BaseColors *)>> colorGetters = {
            {"rosewater", [](const BaseColors *c) { return c->rosewater; }},
            {"flamingo", [](const BaseColors *c) { return c->flamingo; }},
            {"pink", [](const BaseColors *c) { return c->pink; }},
            {"mauve", [](const BaseColors *c) { return c->mauve; }},
            {"red", [](const BaseColors *c) { return c->red; }},
            {"maroon", [](const BaseColors *c) { return c->maroon; }},
            {"peach", [](const BaseColors *c) { return c->peach; }},
            {"yellow", [](const BaseColors *c) { return c->yellow; }},
            {"green", [](const BaseColors *c) { return c->green; }},
            {"teal", [](const BaseColors *c) { return c->teal; }},
            {"sky", [](const BaseColors *c) { return c->sky; }},
            {"sapphire", [](const BaseColors *c) { return c->sapphire; }},
            {"blue", [](const BaseColors *c) { return c->blue; }},
            {"lavender", [](const BaseColors *c) { return c->lavender; }},
            {"text", [](const BaseColors *c) { return c->text; }},
            {"subtext1", [](const BaseColors *c) { return c->subtext1; }},
            {"subtext0", [](const BaseColors *c) { return c->subtext0; }},
            {"overlay2", [](const BaseColors *c) { return c->overlay2; }},
            {"overlay1", [](const BaseColors *c) { return c->overlay1; }},
            {"overlay0", [](const BaseColors *c) { return c->overlay0; }},
            {"surface2", [](const BaseColors *c) { return c->surface2; }},
            {"surface1", [](const BaseColors *c) { return c->surface1; }},
            {"surface0", [](const BaseColors *c) { return c->surface0; }},
            {"base", [](const BaseColors *c) { return c->base; }},
            {"mantle", [](const BaseColors *c) { return c->mantle; }},
            {"crust", [](const BaseColors *c) { return c->crust; }}};

    auto it = colorGetters.find(string);
    if (it != colorGetters.end()) {
        return it.value()(getColor());
    }

    // 默认返回 base 颜色
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
