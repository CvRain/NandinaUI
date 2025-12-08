#include "icon_birdhouse.hpp"

namespace Nandina::Icon::Impl {
    void IconBirdhouse::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
        painter->save();

        QPen pen(color);
        pen.setWidthF(2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);

        qreal scaleX = rect.width() / 24.0;
        qreal scaleY = rect.height() / 24.0;
        painter->translate(rect.x(), rect.y());
        painter->scale(scaleX, scaleY);

        QPainterPath path;

        // Path 1: M12 18v4
        path.moveTo(12, 18);
        path.lineTo(12, 22);

        // Path 2: m17 18 1.956-11.468
        path.moveTo(17, 18);
        path.lineTo(18.956, 6.532);

        // Path 3: m3 8 7.82-5.615a2 2 0 0 1 2.36 0L21 8
        path.moveTo(3, 8);
        path.lineTo(10.82, 2.385);
        // a2 2 0 0 1 2.36 0 -> Approx quadTo
        path.quadTo(12, 1, 13.18, 2.385);
        path.lineTo(21, 8);

        // Path 4: M4 18h16
        path.moveTo(4, 18);
        path.lineTo(20, 18);

        // Path 5: M7 18 5.044 6.532
        path.moveTo(7, 18);
        path.lineTo(5.044, 6.532);

        // Circle: cx="12" cy="10" r="2"
        path.addEllipse(QPointF(12, 10), 2, 2);

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
