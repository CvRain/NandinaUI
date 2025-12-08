#ifndef NANDINA_ICON_IMPL_ICON_BIRD_HPP
#define NANDINA_ICON_IMPL_ICON_BIRD_HPP

#include "../base_icon.hpp"

namespace Nandina::Icon::Impl {
    class IconBird : public BaseIcon {
    public:
        void paint(QPainter *painter, const QRectF &rect, const QColor &color) override;
    };
} // namespace Nandina::Icon::Impl

#endif // NANDINA_ICON_IMPL_ICON_BIRD_HPP
