//
// Created by cvrain on 2025/11/20.
//

#ifndef TRYNANDINA_ICON_MANAGER_HPP
#define TRYNANDINA_ICON_MANAGER_HPP

#include <QHash>
#include <QObject>
#include <QString>
#include <qqmlintegration.h>
#include "Core/Types/nan_singleton.hpp"

namespace Nandina::Icon {
    class IconManager : public Nandina::Core::Types::NanSingleton<IconManager> {
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

        explicit IconManager(QObject *parent = nullptr);

        Q_INVOKABLE QString getPath(const QString &name) const;
        Q_INVOKABLE QString getPathByEnum(Icons icon) const;

    private:
        void loadIcons();
        QHash<QString, QString> m_iconPaths;
    };

} // namespace Nandina::Icon

#endif // TRYNANDINA_ICON_MANAGER_HPP
