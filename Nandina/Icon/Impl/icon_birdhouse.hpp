#ifndef NANDINA_ICON_IMPL_ICON_BIRDHOUSE_HPP
#define NANDINA_ICON_IMPL_ICON_BIRDHOUSE_HPP

#include "../base_icon.hpp"

namespace Nandina::Icon::Impl {
    class IconBirdhouse : public BaseIcon {
    public:
        void paint(QPainter *painter, const QRectF &rect, const QColor &color) override;
    };
} // namespace Nandina::Icon::Impl

#endif // NANDINA_ICON_IMPL_ICON_BIRDHOUSE_HPP
