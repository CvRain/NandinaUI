#include "icon_minimize.hpp"

namespace Nandina::Icon::Impl {
    void IconMinimize::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
        painter->save();

        QPen pen(color);
        pen.setWidthF(2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);

        // Scale and translate to fit rect
        // SVG viewBox is 0 0 24 24
        qreal scaleX = rect.width() / 24.0;
        qreal scaleY = rect.height() / 24.0;
        painter->translate(rect.x(), rect.y());
        painter->scale(scaleX, scaleY);

        QPainterPath path;

        // Path 1: M8 3v3a2 2 0 0 1-2 2H3
        path.moveTo(8, 3);
        path.lineTo(8, 6);
        // arcTo(x, y, w, h, startAngle, sweepLength)
        // Center (6,6), Radius 2 -> Rect(4,4,4,4)
        // Start (8,6) -> 0 deg. End (6,8) -> 90 deg. Sweep +90.
        path.arcTo(4, 4, 4, 4, 0, 90);
        path.lineTo(3, 8);

        // Path 2: M21 8h-3a2 2 0 0 1-2-2V3
        path.moveTo(21, 8);
        path.lineTo(18, 8);
        // Center (18,6), Radius 2 -> Rect(16,4,4,4)
        // Start (18,8) -> 90 deg. End (16,6) -> 180 deg. Sweep +90.
        path.arcTo(16, 4, 4, 4, 90, 90);
        path.lineTo(16, 3);

        // Path 3: M3 16h3a2 2 0 0 1 2 2v3
        path.moveTo(3, 16);
        path.lineTo(6, 16);
        // Center (6,18), Radius 2 -> Rect(4,16,4,4)
        // Start (6,16) -> 270 deg (-90). End (8,18) -> 0 deg. Sweep +90.
        path.arcTo(4, 16, 4, 4, 270, 90);
        path.lineTo(8, 21);

        // Path 4: M16 21v-3a2 2 0 0 1 2-2h3
        path.moveTo(16, 21);
        path.lineTo(16, 18);
        // Center (18,18), Radius 2 -> Rect(16,16,4,4)
        // Start (16,18) -> 180 deg. End (18,16) -> 270 deg (-90). Sweep +90.
        path.arcTo(16, 16, 4, 4, 180, 90);
        path.lineTo(21, 16);

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
