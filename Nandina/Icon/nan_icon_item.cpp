#include "nan_icon_item.hpp"
#include <QPainter>
#include "Theme/themeManager.hpp"

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

        // 监听主题变化，当主题切换时自动更新颜色
        auto themeManager = Nandina::ThemeManager::getInstance();
        if (themeManager) {
            connect(themeManager, &Nandina::ThemeManager::paletteChanged, this, [this]() { updateColorFromRole(); });
        }

        // 立即初始化颜色（从主题获取默认的 "text" 颜色）
        updateColorFromRole();
    }

    void NanIconItem::paint(QPainter *painter) {
        painter->setRenderHint(QPainter::Antialiasing);

        if (!m_pathData.isEmpty()) {
            if (m_renderer.isValid()) {
                m_renderer.render(painter, boundingRect());
            }
        }
    }

    IconManager::Icons NanIconItem::icon() const { return m_icon; }

    void NanIconItem::setIcon(IconManager::Icons icon) {
        if (m_icon == icon)
            return;

        m_icon = icon;
        // 优先使用 Enum 获取路径，如果 Enum 没有（比如是 NONE），则不覆盖 pathData（除非我们想清空）
        // 但为了兼容性，这里我们从 IconManager 获取路径
        QString path = IconManager::getInstance()->getPathByEnum(icon);
        if (!path.isEmpty()) {
            setPathData(path);
        }
        else if (icon == IconManager::Icons::ICON_NONE) {
            // 如果设置为 NONE，是否应该清空 pathData？
            // 如果用户同时使用了 iconName，那么 icon 属性通常是默认值 NONE。
            // 所以只有当用户显式设置 icon 时才应该影响 pathData。
            // 这里简化逻辑：如果 icon 有效，就设置 pathData。
        }

        emit iconChanged();
    }

    QString NanIconItem::iconName() const { return m_iconName; }

    void NanIconItem::setIconName(const QString &iconName) {
        if (m_iconName == iconName)
            return;

        m_iconName = iconName;
        QString path = IconManager::getInstance()->getPath(iconName);
        if (!path.isEmpty()) {
            setPathData(path);
        }
        emit iconNameChanged();
    }

    QColor NanIconItem::color() const { return m_color; }

    void NanIconItem::setColor(const QColor &color) {
        if (m_color == color)
            return;
        m_color = color;
        m_colorInitialized = true; // 用户手动设置颜色，标记为已初始化
        emit colorChanged();
        updateRenderer();
        update();
    }

    QString NanIconItem::colorRole() const { return m_colorRole; }

    void NanIconItem::setColorRole(const QString &colorRole) {
        if (m_colorRole == colorRole)
            return;
        m_colorRole = colorRole;
        emit colorRoleChanged();
        updateColorFromRole();
    }

    QString NanIconItem::pathData() const { return m_pathData; }

    void NanIconItem::setPathData(const QString &pathData) {
        if (m_pathData == pathData)
            return;
        m_pathData = pathData;
        emit pathDataChanged();

        // 如果 pathData 刚设置，并且有 colorRole，且颜色还未从主题初始化，则立即初始化
        // 这处理了构造函数中 updateColorFromRole() 时 pathData 还是空的情况
        if (!m_pathData.isEmpty() && !m_colorRole.isEmpty() && !m_colorInitialized) {
            updateColorFromRole();
        }
        else {
            updateRenderer();
        }
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
        // 如果 fillColor 是透明的，使用 "none"，否则使用颜色值
        QString fillValue = (m_fillColor.alpha() == 0) ? "none" : m_fillColor.name(QColor::HexArgb);

        QString svg = QString(R"(
            <svg viewBox="0 0 24 24" version="1.1" xmlns="http://www.w3.org/2000/svg">
                <path d="%1" stroke="%2" stroke-width="%3" fill="%4" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>
        )")
                              .arg(m_pathData, m_color.name(QColor::HexArgb), QString::number(m_lineWidth), fillValue);

        m_renderer.load(svg.toUtf8());
        update();
    }

    void NanIconItem::updateColorFromRole() {
        // 如果 colorRole 为空字符串，表示用户想要手动控制颜色，不使用主题
        if (m_colorRole.isEmpty())
            return;

        auto themeManager = Nandina::ThemeManager::getInstance();
        if (themeManager) {
            QString colorStr = themeManager->getColorByString(m_colorRole);
            if (!colorStr.isEmpty()) {
                // 使用主题颜色更新图标颜色
                QColor newColor = QColor(colorStr);
                if (m_color != newColor) {
                    m_color = newColor;
                    m_colorInitialized = true; // 标记颜色已初始化
                    emit colorChanged();
                    updateRenderer();
                    update();
                }
            }
            else {
                qWarning() << "NanIconItem: colorRole" << m_colorRole << "not found in theme";
            }
        }
    }

} // namespace Nandina::Icon
