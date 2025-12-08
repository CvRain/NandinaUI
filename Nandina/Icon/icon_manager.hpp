//
// Created by cvrain on 2025/11/20.
//

#ifndef TRYNANDINA_ICON_MANAGER_HPP
#define TRYNANDINA_ICON_MANAGER_HPP

#include <QMap>
#include <QObject>
#include <QString>
#include <memory>
#include <qqmlintegration.h>

#include "Core/Types/nan_singleton.hpp"
#include "base_icon.hpp"

namespace Nandina::Icon {
    class IconManager : public Nandina::Core::Types::NanSingleton<IconManager> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:
        enum class Icons {
            ICON_NONE = -1, // sentinel for "no icon"
            ICON_MAX = 0,
            ICON_MIN = 1,
            ICON_CLOSE = 2,
            ICON_EXPAND = 3,
            ICON_BIRD = 4,
            ICON_BIRDHOUSE = 5,
            ICON_BONE = 6,
        };
        Q_ENUM(Icons)

        explicit IconManager(QObject *parent = nullptr);
        ~IconManager() override;

        /**
         * @brief 获取图标实例
         * @param icon 图标枚举
         * @return 图标实例指针，如果不存在返回 nullptr
         */
        BaseIcon *getIcon(Icons icon);

    private:
        QMap<Icons, std::shared_ptr<BaseIcon>> m_icons;
    };

} // namespace Nandina::Icon

#endif // TRYNANDINA_ICON_MANAGER_HPP
