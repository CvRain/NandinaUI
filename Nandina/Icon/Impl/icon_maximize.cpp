#include "icon_maximize.hpp"

namespace Nandina::Icon::Impl {
    void IconMaximize::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
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

        // Path 1: M8 3H5a2 2 0 0 0-2 2v3
        path.moveTo(8, 3);
        path.lineTo(5, 3);
        // Center (5,5), Radius 2 -> Rect(3,3,4,4)
        // Start (5,3) -> 270 deg. End (3,5) -> 180 deg. Sweep -90.
        path.arcTo(3, 3, 4, 4, 270, -90);
        path.lineTo(3, 8);

        // Path 2: M21 8V5a2 2 0 0 0-2-2h-3
        path.moveTo(21, 8);
        path.lineTo(21, 5);
        // Center (19,5), Radius 2 -> Rect(17,3,4,4)
        // Start (21,5) -> 0 deg. End (19,3) -> 270 deg. Sweep -90.
        path.arcTo(17, 3, 4, 4, 0, -90);
        path.lineTo(16, 3);

        // Path 3: M3 16v3a2 2 0 0 0 2 2h3
        path.moveTo(3, 16);
        path.lineTo(3, 19);
        // Center (5,19), Radius 2 -> Rect(3,17,4,4)
        // Start (3,19) -> 180 deg. End (5,21) -> 90 deg. Sweep -90.
        path.arcTo(3, 17, 4, 4, 180, -90);
        path.lineTo(8, 21);

        // Path 4: M16 21h3a2 2 0 0 0 2-2v-3
        path.moveTo(16, 21);
        path.lineTo(19, 21);
        // Center (19,19), Radius 2 -> Rect(17,17,4,4)
        // Start (19,21) -> 90 deg. End (21,19) -> 0 deg. Sweep -90.
        path.arcTo(17, 17, 4, 4, 90, -90);
        path.lineTo(21, 16);

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
