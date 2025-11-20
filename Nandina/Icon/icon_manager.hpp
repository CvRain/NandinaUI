//
// Created by cvrain on 2025/11/20.
//

#ifndef TRYNANDINA_ICON_MANAGER_HPP
#define TRYNANDINA_ICON_MANAGER_HPP

#include <QObject>
#include <qqmlintegration.h>

#include "Core/Types/nan_singleton.hpp"

namespace Nandina::Icon {
    class IconManager: public Core::Types::NanSingleton<IconManager> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:

    };
}

#endif //TRYNANDINA_ICON_MANAGER_HPP
