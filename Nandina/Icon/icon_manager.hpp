//
// Created by cvrain on 2025/11/20.
//

#ifndef TRYNANDINA_ICON_MANAGER_HPP
#define TRYNANDINA_ICON_MANAGER_HPP

#include <QObject>
#include <qqmlintegration.h>

namespace Nandina::Icon {
    class IconManager: public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:
        explicit IconManager(QObject* parent = nullptr);
    };
}

#endif //TRYNANDINA_ICON_MANAGER_HPP
