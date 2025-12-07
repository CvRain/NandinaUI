#ifndef NANDINA_ICON_IMPL_ICON_MAXIMIZE_HPP
#define NANDINA_ICON_IMPL_ICON_MAXIMIZE_HPP

#include "../base_icon.hpp"

namespace Nandina::Icon::Impl {
    class IconMaximize : public BaseIcon {
    public:
        void paint(QPainter *painter, const QRectF &rect, const QColor &color) override;
    };
} // namespace Nandina::Icon::Impl

#endif // NANDINA_ICON_IMPL_ICON_MAXIMIZE_HPP
