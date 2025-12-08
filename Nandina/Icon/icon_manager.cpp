//
// Created by cvrain on 2025/11/20.
//

#include "icon_manager.hpp"
#include <QDebug>
#include "Impl/icon_bird.hpp"
#include "Impl/icon_birdhouse.hpp"
#include "Impl/icon_bone.hpp"
#include "Impl/icon_close.hpp"
#include "Impl/icon_expand.hpp"
#include "Impl/icon_maximize.hpp"
#include "Impl/icon_minimize.hpp"

namespace Nandina::Icon {
    IconManager::IconManager(QObject *parent) : NanSingleton(parent) {}

    IconManager::~IconManager() { m_icons.clear(); }

    BaseIcon *IconManager::getIcon(Icons icon) {
        if (m_icons.contains(icon)) {
            return m_icons.value(icon).get();
        }

        BaseIcon *newIcon = nullptr;
        switch (icon) {
            case Icons::ICON_NONE:
                return nullptr;
            case Icons::ICON_CLOSE:
                newIcon = new Impl::IconClose();
                break;
            case Icons::ICON_MAX:
                newIcon = new Impl::IconMaximize();
                break;
            case Icons::ICON_MIN:
                newIcon = new Impl::IconMinimize();
                break;
            case Icons::ICON_EXPAND:
                newIcon = new Impl::IconExpand();
                break;
            case Icons::ICON_BIRD:
                newIcon = new Impl::IconBird();
                break;
            case Icons::ICON_BIRDHOUSE:
                newIcon = new Impl::IconBirdhouse();
                break;
            case Icons::ICON_BONE:
                newIcon = new Impl::IconBone();
                break;
            default:
                qWarning() << "IconManager: Unknown icon type requested";
                break;
        }

        if (newIcon) {
            m_icons.insert(icon, std::shared_ptr<BaseIcon>(newIcon));
        }

        return newIcon;
    }
} // namespace Nandina::Icon
