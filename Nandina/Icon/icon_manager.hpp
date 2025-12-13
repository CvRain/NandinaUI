//
// Created by cvrain on 2025/11/20.
//

#ifndef TRYNANDINA_ICON_MANAGER_HPP
#define TRYNANDINA_ICON_MANAGER_HPP

#include <QObject>
#include <qqmlintegration.h>

namespace Nandina::Icon {
    class IconManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:
        enum class Icons {
            ICON_NONE = -1, // sentinel for "no icon"
            ICON_MAXIMIZE = 0,
            ICON_MINIMIZE = 1,
            ICON_CLOSE = 2,
            ICON_EXPAND = 3,
            ICON_BIRD = 4,
            ICON_BIRDHOUSE = 5,
            ICON_BONE = 6,
        };
        Q_ENUM(Icons)

        explicit IconManager(QObject *parent = nullptr) : QObject(parent) {}
    };

} // namespace Nandina::Icon

#endif // TRYNANDINA_ICON_MANAGER_HPP
