#ifndef NANDINA_ICON_BASE_ICON_HPP
#define NANDINA_ICON_BASE_ICON_HPP

#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QPainterPath>

namespace Nandina::Icon {

    /**
     * @brief 图标基类，所有自绘图标都需要继承此类并实现 paint 方法
     */
    class BaseIcon {
    public:
        virtual ~BaseIcon() = default;

        /**
         * @brief 绘制图标
         * @param painter QPainter 对象
         * @param rect 绘制区域
         * @param color 图标颜色
         */
        virtual void paint(QPainter *painter, const QRectF &rect, const QColor &color) = 0;
    };

} // namespace Nandina::Icon

#endif // NANDINA_ICON_BASE_ICON_HPP
