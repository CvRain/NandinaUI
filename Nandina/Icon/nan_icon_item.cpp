#include "nan_icon_item.hpp"
#include <QPainter>

namespace Nandina::Icon {

    NanIconItem::NanIconItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
        // 启用抗锯齿
        setAntialiasing(true);
    }

    void NanIconItem::paint(QPainter *painter) {
        const auto iconInstance = IconManager::getInstance()->getIcon(m_icon);
        if (iconInstance) {
            painter->setRenderHint(QPainter::Antialiasing);
            iconInstance->paint(painter, boundingRect(), m_color);
        }
    }

    IconManager::Icons NanIconItem::icon() const { return m_icon; }

    void NanIconItem::setIcon(IconManager::Icons icon) {
        if (m_icon == icon)
            return;
        m_icon = icon;
        emit iconChanged();
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
