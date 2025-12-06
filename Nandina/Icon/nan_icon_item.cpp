#include "nan_icon_item.hpp"
#include <QPainter>

namespace Nandina::Icon {

    NanIconItem::NanIconItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
        // 启用抗锯齿
        setAntialiasing(true);
    }

    void NanIconItem::paint(QPainter *painter) {
        auto icon = IconManager::instance().getIcon(m_iconName);
        if (icon) {
            painter->setRenderHint(QPainter::Antialiasing);
            icon->paint(painter, boundingRect(), m_color);
        }
    }

    QString NanIconItem::iconName() const { return m_iconName; }

    void setIconName(const QString &name) {
        // ...
    }

    void NanIconItem::setIconName(const QString &name) {
        if (m_iconName == name)
            return;
        m_iconName = name;
        emit iconNameChanged();
        update(); // 触发重绘
    }

    QColor NanIconItem::color() const { return m_color; }

    void NanIconItem::setColor(const QColor &color) {
        if (m_color == color)
            return;
        m_color = color;
        emit colorChanged();
        update();
    }

} // namespace Nandina::Icon
