#include "icon_close.hpp"
#include "../icon_manager.hpp"

namespace Nandina::Icon::Impl {
    void IconClose::paint(QPainter *painter, const QRectF &rect, const QColor &color) {
        painter->save();

        QPen pen(color);
        pen.setWidthF(2.0);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);

        // 留一点边距
        qreal padding = qMin(rect.width(), rect.height()) * 0.2;
        QRectF r = rect.adjusted(padding, padding, -padding, -padding);

        painter->drawLine(r.topLeft(), r.bottomRight());
        painter->drawLine(r.topRight(), r.bottomLeft());

        painter->restore();
    }

    // 自动注册逻辑
    // 注意：静态变量的初始化顺序在不同编译单元间是不确定的。
    // 如果 IconManager 也是静态单例，需要确保它在注册时可用。
    // NanSingleton 通常是懒加载的，所以这里调用 instance() 会创建它，这是安全的。
    namespace {
        struct Register {
            Register() { IconManager::instance().registerIcon("close", new IconClose()); }
        };
        static Register reg;
    } // namespace
} // namespace Nandina::Icon::Impl
