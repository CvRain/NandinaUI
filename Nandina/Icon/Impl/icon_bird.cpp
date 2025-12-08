#include "icon_bird.hpp"

namespace Nandina::Icon::Impl {
    void IconBird::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
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

        // Path 1: M16 7h.01
        path.moveTo(16, 7);
        path.lineTo(16.01, 7);

        // Path 2: M3.4 18H12a8 8 0 0 0 8-8V7a4 4 0 0 0-7.28-2.3L2 20
        path.moveTo(3.4, 18);
        path.lineTo(12, 18);
        // a8 8 0 0 0 8-8 -> Center (12, 10), Radius 8. Start (12, 18) -> 270 deg. Sweep 90 deg.
        path.arcTo(4, 2, 16, 16, 270, 90);
        path.lineTo(20, 7);
        // a4 4 0 0 0-7.28-2.3 -> Approx cubicTo
        path.cubicTo(18, 5, 15, 4, 12.72, 4.7);
        path.lineTo(2, 20);

        // Path 3: m20 7 2 .5-2 .5
        path.moveTo(20, 7);
        path.lineTo(22, 7.5);
        path.lineTo(20, 8);

        // Path 4: M10 18v3
        path.moveTo(10, 18);
        path.lineTo(10, 21);

        // Path 5: M14 17.75V21
        path.moveTo(14, 17.75);
        path.lineTo(14, 21);

        // Path 6: M7 18a6 6 0 0 0 3.84-10.61
        path.moveTo(7, 18);
        // Approx cubicTo for a6 6 0 0 0 3.84-10.61
        path.cubicTo(5, 14, 7, 9, 10.84, 7.39);

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
