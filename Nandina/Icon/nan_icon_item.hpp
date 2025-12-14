#ifndef NANDINA_ICON_NAN_ICON_ITEM_HPP
#define NANDINA_ICON_NAN_ICON_ITEM_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include <QSvgRenderer>
#include "icon_manager.hpp"

namespace Nandina::Icon {

    class NanIconItem : public QQuickPaintedItem {
        Q_OBJECT
        Q_PROPERTY(Nandina::Icon::IconManager::Icons icon READ icon WRITE setIcon NOTIFY iconChanged)
        Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
        Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
        Q_PROPERTY(QString colorRole READ colorRole WRITE setColorRole NOTIFY colorRoleChanged)
        Q_PROPERTY(QString pathData READ pathData WRITE setPathData NOTIFY pathDataChanged)
        Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
        Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
        QML_ELEMENT

    public:
        explicit NanIconItem(QQuickItem *parent = nullptr);

        void paint(QPainter *painter) override;

        Nandina::Icon::IconManager::Icons icon() const;
        void setIcon(Nandina::Icon::IconManager::Icons icon);

        QString iconName() const;
        void setIconName(const QString &iconName);

        QColor color() const;
        void setColor(const QColor &color);

        QString colorRole() const;
        void setColorRole(const QString &colorRole);

        QString pathData() const;
        void setPathData(const QString &pathData);

        qreal lineWidth() const;
        void setLineWidth(qreal lineWidth);

        QColor fillColor() const;
        void setFillColor(const QColor &fillColor);

    signals:
        void iconChanged();
        void iconNameChanged();
        void colorChanged();
        void colorRoleChanged();
        void pathDataChanged();
        void lineWidthChanged();
        void fillColorChanged();

    private:
        void updateRenderer();
        void updateColorFromRole();

        Nandina::Icon::IconManager::Icons m_icon = Nandina::Icon::IconManager::Icons::ICON_NONE;
        QString m_iconName;
        QColor m_color = Qt::black;
        QString m_colorRole;
        QString m_pathData;
        qreal m_lineWidth = 1.0;
        QColor m_fillColor = Qt::transparent;
        QSvgRenderer m_renderer;
    };

} // namespace Nandina::Icon

#endif // NANDINA_ICON_NAN_ICON_ITEM_HPP
