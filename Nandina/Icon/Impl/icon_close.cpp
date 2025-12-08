#include "icon_close.hpp"
#include "../icon_manager.hpp"

namespace Nandina::Icon::Impl {
    void IconClose::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
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
        // Path 1: M18 6 6 18
        path.moveTo(18, 6);
        path.lineTo(6, 18);

        // Path 2: m6 6 12 12
        path.moveTo(6, 6);
        path.lineTo(18, 18);

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
