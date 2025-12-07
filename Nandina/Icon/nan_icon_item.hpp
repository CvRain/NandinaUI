#ifndef NANDINA_ICON_NAN_ICON_ITEM_HPP
#define NANDINA_ICON_NAN_ICON_ITEM_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include "icon_manager.hpp"

namespace Nandina::Icon {

    class NanIconItem : public QQuickPaintedItem {
        Q_OBJECT
        Q_PROPERTY(Nandina::Icon::IconManager::Icons icon READ icon WRITE setIcon NOTIFY iconChanged)
        Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
        QML_ELEMENT

    public:
        explicit NanIconItem(QQuickItem *parent = nullptr);

        void paint(QPainter *painter) override;

        Nandina::Icon::IconManager::Icons icon() const;
        void setIcon(Nandina::Icon::IconManager::Icons icon);

        QColor color() const;
        void setColor(const QColor &color);

    signals:
        void iconChanged();
        void colorChanged();

    private:
        Nandina::Icon::IconManager::Icons m_icon = Nandina::Icon::IconManager::Icons::ICON_CLOSE;
        QColor m_color = Qt::black;
    };

} // namespace Nandina::Icon

#endif // NANDINA_ICON_NAN_ICON_ITEM_HPP
