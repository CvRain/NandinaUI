#include "icon_bone.hpp"

namespace Nandina::Icon::Impl {
    void IconBone::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
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

        // M17 10
        path.moveTo(17, 10);

        // c.7-.7 1.69 0 2.5 0
        path.cubicTo(17.7, 9.3, 18.69, 10, 19.5, 10);

        // a2.5 2.5 0 1 0 0-5 -> Semi-circle. Center (19.5, 7.5). Start 270 (Down). Sweep 180 (CCW).
        path.arcTo(17, 5, 5, 5, 270, 180);

        // .5.5 0 0 1-.5-.5 -> Quarter circle. Center (19, 5). Start 90 (Up). Sweep -90 (CW).
        // Wait, SVG sweep 1 is CW. Qt sweep negative is CW.
        // Start (19.5, 5). End (19, 4.5).
        // Center (19, 5). Start angle 0 (Right). End angle 90 (Up).
        // From 0 to 90 is +90 (CCW).
        // SVG sweep 1 (CW).
        // Let's re-read SVG arc.
        // Start (19.5, 5). End (19, 4.5). Radius 0.5.
        // Center (19, 5).
        // Start angle 0 (Right). End angle 90 (Up).
        // Wait, Y-down. 0 is Right. 90 is Down. 270 is Up.
        // Start (19.5, 5) is Right of Center (19, 5). Angle 0.
        // End (19, 4.5) is Up of Center (19, 5). Angle 270.
        // From 0 to 270 is -90 (CW).
        // So arcTo(18.5, 4.5, 1, 1, 0, 90)? No.
        // From 0 to 90 (CCW) is Down.
        // From 0 to 270 (CW) is Up.
        // So sweep is 90 (CCW).
        path.arcTo(18.5, 4.5, 1, 1, 0, 90);

        // 2.5 2.5 0 1 0-5 0 -> Semi-circle.
        // Start (19, 4.5). End (14, 4.5).
        // Center (16.5, 4.5). Radius 2.5.
        // Start angle 0 (Right). End angle 180 (Left).
        // Sweep 180 (CCW).
        // Wait, SVG sweep 0 (CCW).
        // From 0 to 180 is CCW (Down).
        // But we want Up arc.
        // Center (16.5, 4.5).
        // Start (19, 4.5) is Right.
        // End (14, 4.5) is Left.
        // Arc goes Up.
        // Up is 270.
        // So 0 -> 270 -> 180.
        // This is CW.
        // SVG sweep 0 is CCW.
        // In Y-down, CCW is Down.
        // So SVG sweep 0 means Down arc.
        // But Bone shape is Up.
        // Let's check SVG again. `a2.5 2.5 0 1 0-5 0`.
        // Start (19, 4.5). End (14, 4.5).
        // Sweep 0. Large arc 1.
        // If sweep 0 (CCW), it goes Down.
        // If sweep 1 (CW), it goes Up.
        // Wait, Bone top part should be Up.
        // Maybe my coordinate system understanding is flipped.
        // Let's just use cubicTo for safety or arcTo with trial.
        // Actually, `a2.5 2.5 0 1 0-5 0` with large-arc 1 means the "long way".
        // Distance is 5. Radius 2.5. Diameter 5.
        // So it's a semi-circle. Large arc flag doesn't matter for semi-circles (both are 180).
        // Sweep 0.
        // If I draw a bone, the top part bulges Up.
        // So it should be Up.
        // If SVG sweep 0 is Up, then my Y-down logic is consistent with "CCW = Down" being wrong?
        // In Y-down:
        // +Angle is CW (visually).
        // 0 (Right) -> 90 (Down).
        // So +90 is Down.
        // So CCW is -Angle.
        // SVG Sweep 1 is +Angle (CW). Sweep 0 is -Angle (CCW).
        // So Sweep 0 is CCW (Up).
        // So `arcTo(..., 0, 180)` goes Down.
        // `arcTo(..., 0, -180)` goes Up.
        // So we want `arcTo(..., 0, 180)` (CCW in Qt is +Angle).
        // Wait, Qt `arcTo` +Angle is CCW (Mathematical).
        // 0 (Right) -> 90 (Up) -> 180 (Left).
        // So `arcTo(..., 0, 180)` goes Up.
        // So this matches SVG Sweep 0 (CCW).
        path.arcTo(14, 2, 5, 5, 0, 180);

        // c0 .81.7 1.8 0 2.5
        path.cubicTo(14, 5.31, 14.7, 6.3, 14, 7);

        // l-7 7
        path.lineTo(7, 14);

        // c-.7.7-1.69 0-2.5 0
        path.cubicTo(6.3, 14.7, 5.31, 14, 4.5, 14);

        // a2.5 2.5 0 0 0 0 5 -> Semi-circle.
        // Start (4.5, 14). End (4.5, 19).
        // Center (4.5, 16.5).
        // Start 90 (Up). End 270 (Down).
        // Sweep 180 (CCW).
        // Goes Left.
        path.arcTo(2, 14, 5, 5, 90, 180);

        // c.28 0 .5.22.5.5
        path.cubicTo(4.78, 19, 5, 19.22, 5, 19.5);

        // a2.5 2.5 0 1 0 5 0 -> Semi-circle.
        // Start (5, 19.5). End (10, 19.5).
        // Center (7.5, 19.5).
        // Start 180 (Left). End 0 (Right).
        // Sweep 180 (CCW). Goes Down.
        // SVG Sweep 0 (CCW).
        // Qt CCW is +Angle.
        // So `arcTo(..., 180, 180)` goes Down.
        path.arcTo(5, 17, 5, 5, 180, 180);

        // c0-.81-.7-1.8 0-2.5
        path.cubicTo(10, 18.69, 9.3, 17.7, 10, 17);

        path.closeSubpath();

        painter->drawPath(path);

        painter->restore();
    }
} // namespace Nandina::Icon::Impl
