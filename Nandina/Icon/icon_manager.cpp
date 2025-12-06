//
// Created by cvrain on 2025/11/20.
//

#include "icon_manager.hpp"
#include <QDebug>

namespace Nandina::Icon {
    IconManager::IconManager(QObject *parent) : NanSingleton(parent) {
        // 可以在这里注册默认图标，或者通过静态初始化注册
    }

    IconManager::~IconManager() { m_icons.clear(); }

    void IconManager::registerIcon(const QString &name, BaseIcon *icon) {
        if (!icon)
            return;
        if (m_icons.contains(name)) {
            qWarning() << "IconManager: Icon" << name << "already registered, overwriting.";
        }
        m_icons.insert(name, std::shared_ptr<BaseIcon>(icon));
    }

    BaseIcon *IconManager::getIcon(const QString &name) const { return m_icons.value(name, nullptr).get(); }

    QString IconManager::getIconName(Icons icon) const {
        switch (icon) {
            case Icons::ICON_MAX:
                return "maximize";
            case Icons::ICON_MIN:
                return "minimize";
            case Icons::ICON_CLOSE:
                return "close";
            default:
                return "";
        }
    }
} // namespace Nandina::Icon
