#include "icon_manager.hpp"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace Nandina::Icon {

    IconManager::IconManager(QObject *parent) : NanSingleton(parent) { loadIcons(); }

    void IconManager::loadIcons() {
        QFile file(":/Nandina/Icon/icons.json");
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open icons.json";
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        QJsonObject icons = root["icons"].toObject();

        for (auto it = icons.begin(); it != icons.end(); ++it) {
            m_iconPaths.insert(it.key(), it.value().toString());
        }
    }

    QString IconManager::getPath(const QString &name) const { return m_iconPaths.value(name); }

    QString IconManager::getPathByEnum(Icons icon) const {
        switch (icon) {
            case Icons::ICON_CLOSE:
                return m_iconPaths.value("close");
            case Icons::ICON_MAXIMIZE:
                return m_iconPaths.value("maximize");
            case Icons::ICON_MINIMIZE:
                return m_iconPaths.value("minimize");
            case Icons::ICON_EXPAND:
                return m_iconPaths.value("expand");
            case Icons::ICON_BIRD:
                return m_iconPaths.value("bird");
            case Icons::ICON_BIRDHOUSE:
                return m_iconPaths.value("birdhouse");
            case Icons::ICON_BONE:
                return m_iconPaths.value("bone");
            default:
                return "";
        }
    }

} // namespace Nandina::Icon
