#include "themeManager.hpp"
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QFileInfo>

#include "Utils/file_operator.hpp"

using namespace Nandina;

ThemeManager *ThemeManager::instance = nullptr;

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

BaseColors* ThemeManager::getColor() const {
    return this->currentBaseColors;
}

Theme::Components::NanButtonStyle * ThemeManager::getButtonStyle(const QString &type) {
    if (buttonStyles.empty()) {
        qWarning() << "No button styles loaded!";
        return nullptr;
    }

    if (not buttonStyles.contains(type)) {
        qWarning() << "Button style not found for type:" << type;
        // 如果找不到指定类型，返回默认类型
        if (buttonStyles.contains("default")) {
            return &buttonStyles.at("default");
        }
        return nullptr;
    }

    qDebug() << "Load type: " << type << " background: " << buttonStyles.at(type).getBackground() ;

    return &buttonStyles.at(type);
}

QString ThemeManager::resolveColorVariable(const QString &value) const {
    if (value.startsWith("${color.") && value.endsWith("}")) {
        QString colorProperty = value.mid(8, value.length() - 9); // 去除 "${color." 和 "}"
        if (currentBaseColors != nullptr) {
            // 使用元对象系统动态获取属性值
            QVariant propertyValue = currentBaseColors->property(colorProperty.toUtf8().constData());
            if (propertyValue.isValid()) {
                return propertyValue.toString();
            }
        }
        qWarning() << "Failed to resolve color variable:" << value;
        return "#808080"; // 返回一个默认颜色
    }
    return value; // 如果不是变量引用，直接返回原值
}

QJsonObject ThemeManager::resolveStyleColors(const QJsonObject &styleObject) const {
    QJsonObject resolved;
    for (auto it = styleObject.begin(); it != styleObject.end(); ++it) {
        if (it.value().isString()) {
            // 解析颜色变量
            QString resolvedValue = resolveColorVariable(it.value().toString());
            resolved.insert(it.key(), resolvedValue);
        } else if (it.value().isObject()) {
            // 递归处理嵌套对象
            resolved.insert(it.key(), resolveStyleColors(it.value().toObject()));
        } else {
            // 保持其他类型的值不变
            resolved.insert(it.key(), it.value());
        }
    }
    return resolved;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), currentPaletteType(Core::Types::CatppuccinSetting::CatppuccinType::Latte) {
    loadBaseColor();
    currentBaseColors = &baseColors.at(currentPaletteType);

    loadComponentStyles();
}

void ThemeManager::loadComponentStyles() {
    const QString componentStyleDirPath = ":/qt/qml/Nandina/Theme/Resources/Styles";
    qDebug() << "Loading component styles from:" << componentStyleDirPath;
    //遍历组件样式目录，读取组件样式json
    QDirIterator it(componentStyleDirPath, QStringList() << "*.json", QDir::Files);
    while (it.hasNext()) {
        const auto filePath = it.next();
        const auto result = Core::Utils::FileOperator::readJsonFile(filePath);
        if (not result.has_value()) {
            qWarning() << "Failed to load component style:" << filePath;
            continue;
        }

        const auto& jsonObj = result.value();
        const QString target = jsonObj["target"].toString();

        // 处理NanButton的样式
        if (target == "NanButton") {
            const auto variants = jsonObj["variants"].toObject();
            for (auto it = variants.begin(); it != variants.end(); ++it) {
                const QString variantName = it.key();
                // 解析颜色变量，替换为实际的颜色值
                const QJsonObject resolvedStyle = resolveStyleColors(it.value().toObject());
                qDebug() << "Load type: " << variantName << " background: " << resolvedStyle["background"].toString();

                // 创建新的按钮样式
                Theme::Components::NanButtonStyle style;
                if (style.loadFromJson(resolvedStyle)) {
                    buttonStyles.insert(std::make_pair(variantName, style));
                } else {
                    qWarning() << "Failed to load button style for variant:" << variantName;
                }
            }
        }
        // 在这里可以添加其他组件样式的处理
    }
    emit stylesLoaded();
}

void ThemeManager::loadBaseColor() {
    using namespace Nandina::Core::Types;
    const std::map<CatppuccinSetting::CatppuccinType, QString> baseColorUrl{
        {CatppuccinSetting::CatppuccinType::Latte, ":/qt/qml/Nandina/Theme/Resources/Palettes/Latte.json"},
        {CatppuccinSetting::CatppuccinType::Frappe, ":/qt/qml/Nandina/Theme/Resources/Palettes/Frappe.json"},
        {CatppuccinSetting::CatppuccinType::Macchiato, ":/qt/qml/Nandina/Theme/Resources/Palettes/Macchiato.json"},
        {CatppuccinSetting::CatppuccinType::Mocha, ":/qt/qml/Nandina/Theme/Resources/Palettes/Mocha.json"}
    };

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

        QJsonObject p = jsonDoc.object();

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
    };

    for (const auto &[fst, snd]: baseColorUrl) {
        instantiatePalette(fst, snd);
    }
}
