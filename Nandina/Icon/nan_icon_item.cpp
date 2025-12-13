#include "nan_icon_item.hpp"
#include <QPainter>

namespace Nandina::Icon {

    NanIconItem::NanIconItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
        // 启用抗锯齿
        setAntialiasing(true);

        // 设置默认尺寸
        setImplicitWidth(24);
        setImplicitHeight(24);

        // 监听尺寸变化以触发重绘
        connect(this, &QQuickItem::widthChanged, this, [this]() { update(); });
        connect(this, &QQuickItem::heightChanged, this, [this]() { update(); });
    }

    void NanIconItem::paint(QPainter *painter) {
        painter->setRenderHint(QPainter::Antialiasing);

        if (!m_pathData.isEmpty()) {
            if (m_renderer.isValid()) {
                m_renderer.render(painter, boundingRect());
            }
        }
    }

    static QString getPathForIcon(IconManager::Icons icon) {
        switch (icon) {
            case IconManager::Icons::ICON_CLOSE:
                return "M18 6L6 18M6 6l12 12";
            case IconManager::Icons::ICON_MAXIMIZE:
                return "M8 3H5a2 2 0 0 0-2 2v3 M21 8V5a2 2 0 0 0-2-2h-3 M3 16v3a2 2 0 0 0 2 2h3 M16 21h3a2 2 0 0 0 "
                       "2-2v-3";
            case IconManager::Icons::ICON_MINIMIZE:
                return "M8 3v3a2 2 0 0 1-2 2H3 M21 8h-3a2 2 0 0 1-2-2V3 M3 16h3a2 2 0 0 1 2 2v3 M16 21v-3a2 2 0 0 1 "
                       "2-2h3";
            case IconManager::Icons::ICON_EXPAND:
                return "M15 15 L21 21 M15 9 L21 3 M21 16 L21 21 L16 21 M21 8 L21 3 L16 3 M3 16 L3 21 L8 21 M3 21 L9 15 "
                       "M3 8 L3 3 L8 3 M9 9 L3 3";
            case IconManager::Icons::ICON_BIRD:
                return "M16 7h.01 M3.4 18H12a8 8 0 0 0 8-8V7a4 4 0 0 0-7.28-2.3L2 20 M20 7 L22 7.5 L20 8 M10 18v3 M14 "
                       "17.75V21 M7 18 C5 14 7 9 10.84 7.39";
            case IconManager::Icons::ICON_BIRDHOUSE:
                return "M12 18v4 M17 18 L18.956 6.532 M3 8 L10.82 2.385 Q12 1 13.18 2.385 L21 8 M4 18h16 M7 18 L5.044 "
                       "6.532 M14 10a2 2 0 1 1-4 0 2 2 0 0 1 4 0";
            case IconManager::Icons::ICON_BONE:
                return "M17 10 c0.7 -0.7 1.69 0 2.5 0 a2.5 2.5 0 1 0 0 -5 a0.5 0.5 0 0 1 -0.5 -0.5 a2.5 2.5 0 1 0 -5 0 "
                       "c0 0.81 0.7 1.8 0 2.5 l-3.5 3.5 c-0.7 0.7 -1.69 0 -2.5 0 a2.5 2.5 0 1 0 0 5 a0.5 0.5 0 0 1 0.5 "
                       "0.5 a2.5 2.5 0 1 0 5 0 c0 -0.81 -0.7 -1.8 0 -2.5 l3.5 -3.5 z";
            default:
                return "";
        }
    }

    IconManager::Icons NanIconItem::icon() const { return m_icon; }

    void NanIconItem::setIcon(IconManager::Icons icon) {
        if (m_icon == icon)
            return;

        m_icon = icon;
        setPathData(getPathForIcon(icon));
        emit iconChanged();
    }

    QColor NanIconItem::color() const { return m_color; }

    void NanIconItem::setColor(const QColor &color) {
        if (m_color == color)
            return;
        m_color = color;
        emit colorChanged();
        updateRenderer();
        update();
    }

    QString NanIconItem::pathData() const { return m_pathData; }

    void NanIconItem::setPathData(const QString &pathData) {
        if (m_pathData == pathData)
            return;
        m_pathData = pathData;
        emit pathDataChanged();
        updateRenderer();
    }

    qreal NanIconItem::lineWidth() const { return m_lineWidth; }

    void NanIconItem::setLineWidth(qreal lineWidth) {
        if (qFuzzyCompare(m_lineWidth, lineWidth))
            return;
        m_lineWidth = lineWidth;
        emit lineWidthChanged();
        updateRenderer();
    }

    QColor NanIconItem::fillColor() const { return m_fillColor; }

    void NanIconItem::setFillColor(const QColor &fillColor) {
        if (m_fillColor == fillColor)
            return;
        m_fillColor = fillColor;
        emit fillColorChanged();
        updateRenderer();
    }

    void NanIconItem::updateRenderer() {
        if (m_pathData.isEmpty())
            return;

        // Construct SVG XML
        // Using 24x24 viewBox as standard for icons
        QString svg = QString(R"(
            <svg viewBox="0 0 24 24" version="1.1" xmlns="http://www.w3.org/2000/svg">
                <path d="%1" stroke="%2" stroke-width="%3" fill="%4" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>
        )")
                              .arg(m_pathData, m_color.name(QColor::HexArgb), QString::number(m_lineWidth),
                                   m_fillColor.name(QColor::HexArgb));

        m_renderer.load(svg.toUtf8());
        update();
    }

} // namespace Nandina::Icon
