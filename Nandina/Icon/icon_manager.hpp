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
        // 为了兼容性保留枚举，但建议主要使用字符串 ID
        enum class Icons {
            ICON_MAX,
            ICON_MIN,
            ICON_CLOSE,
        };
        Q_ENUM(Icons)

        explicit IconManager(QObject *parent = nullptr);
        ~IconManager() override;

        /**
         * @brief 注册图标
         * @param name 图标名称 (ID)
         * @param icon 图标实例 (接管所有权)
         */
        void registerIcon(const QString &name, BaseIcon *icon);

        /**
         * @brief 获取图标实例
         * @param name 图标名称
         * @return 图标实例指针，如果不存在返回 nullptr
         */
        BaseIcon *getIcon(const QString &name) const;

        /**
         * @brief 通过枚举获取图标名称的辅助函数
         */
        QString getIconName(Icons icon) const;

    private:
        QMap<QString, std::shared_ptr<BaseIcon>> m_icons;
    };
} // namespace Nandina::Icon

#endif // TRYNANDINA_ICON_MANAGER_HPP
