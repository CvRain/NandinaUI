#include "icon_expand.hpp"

namespace Nandina::Icon::Impl {
    void IconExpand::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
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

        // Path 1: m15 15 6 6
        path.moveTo(15, 15);
        path.lineTo(21, 21);

        // Path 2: m15 9 6-6
        path.moveTo(15, 9);
        path.lineTo(21, 3);

        // Path 3: M21 16v5h-5
        path.moveTo(21, 16);
        path.lineTo(21, 21);
        path.lineTo(16, 21);

        // Path 4: M21 8V3h-5
        path.moveTo(21, 8);
        path.lineTo(21, 3);
        path.lineTo(16, 3);

        // Path 5: M3 16v5h5
        path.moveTo(3, 16);
        path.lineTo(3, 21);
        path.lineTo(8, 21);

        // Path 6: m3 21 6-6
        path.moveTo(3, 21);
        path.lineTo(9, 15);

        // Path 7: M3 8V3h5
        path.moveTo(3, 8);
        path.lineTo(3, 3);
        path.lineTo(8, 3);

        // Path 8: M9 9 3 3
        path.moveTo(9, 9);
        path.lineTo(3, 3);

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
