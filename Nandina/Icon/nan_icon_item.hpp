#ifndef NANDINA_ICON_NAN_ICON_ITEM_HPP
#define NANDINA_ICON_NAN_ICON_ITEM_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include "icon_manager.hpp"

namespace Nandina::Icon {

    class NanIconItem : public QQuickPaintedItem {
        Q_OBJECT
        Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
        Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
        QML_ELEMENT

    public:
        explicit NanIconItem(QQuickItem *parent = nullptr);

        void paint(QPainter *painter) override;

        QString iconName() const;
        void setIconName(const QString &name);

        QColor color() const;
        void setColor(const QColor &color);

    signals:
        void iconNameChanged();
        void colorChanged();

    private:
        QString m_iconName;
        QColor m_color = Qt::black;
    };

} // namespace Nandina::Icon

#endif // NANDINA_ICON_NAN_ICON_ITEM_HPP
