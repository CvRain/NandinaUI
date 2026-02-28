//
// Created by cvrain on 2026/2/27.
// Completed with full primitive token support.
//

#ifndef NANDINA_PRIMITIVES_HPP
#define NANDINA_PRIMITIVES_HPP

#include <QColor>
#include <QObject>
#include <QQmlEngine>

#include "typography_schema.hpp"

#if defined(_WIN32)
#if defined(NandinaPrimitives_EXPORTS)
#define NANDINA_PRIMITIVES_EXPORT Q_DECL_EXPORT
#else
#define NANDINA_PRIMITIVES_EXPORT Q_DECL_IMPORT
#endif
#else
#define NANDINA_PRIMITIVES_EXPORT
#endif

namespace Nandina::Core::Primitives {

    /**
     * @brief Design primitives: spacing, radius, border, typography, background.
     *
     * Maps to Skeleton's CSS primitive tokens:
     * - --spacing, --text-scaling
     * - --radius-base, --radius-container
     * - --default-border-width, --default-divide-width, --default-ring-width
     * - --body-background-color / --body-background-color-dark
     * - Typography sub-objects: baseFont, headingFont, anchorFont
     *
     * Usage in QML:
     *   ThemeManager.primitives.spacing
     *   ThemeManager.primitives.radiusBase
     *   ThemeManager.primitives.baseFont.fontFamily
     */
    class NANDINA_PRIMITIVES_EXPORT PrimitiveSchema : public QObject {
        Q_OBJECT
        QML_ELEMENT

        // ── Layout ──
        Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY changed)
        Q_PROPERTY(qreal textScaling READ textScaling WRITE setTextScaling NOTIFY changed)

        // ── Border radius ──
        Q_PROPERTY(qreal radiusBase READ radiusBase WRITE setRadiusBase NOTIFY changed)
        Q_PROPERTY(qreal radiusContainer READ radiusContainer WRITE setRadiusContainer NOTIFY changed)

        // ── Border / ring widths ──
        Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY changed)
        Q_PROPERTY(qreal divideWidth READ divideWidth WRITE setDivideWidth NOTIFY changed)
        Q_PROPERTY(qreal ringWidth READ ringWidth WRITE setRingWidth NOTIFY changed)

        // ── Body background ──
        Q_PROPERTY(QColor bodyBackgroundColor READ bodyBackgroundColor WRITE setBodyBackgroundColor NOTIFY changed)
        Q_PROPERTY(QColor bodyBackgroundColorDark READ bodyBackgroundColorDark WRITE setBodyBackgroundColorDark NOTIFY
                           changed)

        // ── Typography ──
        Q_PROPERTY(TypographySchema *baseFont READ baseFont CONSTANT)
        Q_PROPERTY(TypographySchema *headingFont READ headingFont CONSTANT)
        Q_PROPERTY(TypographySchema *anchorFont READ anchorFont CONSTANT)

    public:
        explicit PrimitiveSchema(QObject *parent = nullptr) :
            QObject(parent), m_baseFont(new TypographySchema(this)), m_headingFont(new TypographySchema(this)),
            m_anchorFont(new TypographySchema(this)) {
            auto fwd = [this]() { emit changed(); };
            connect(m_baseFont, &TypographySchema::changed, this, fwd);
            connect(m_headingFont, &TypographySchema::changed, this, fwd);
            connect(m_anchorFont, &TypographySchema::changed, this, fwd);
        }

        // ── Layout ──
        [[nodiscard]] qreal spacing() const {
            return m_spacing;
        }
        [[nodiscard]] qreal textScaling() const {
            return m_textScaling;
        }

        void setSpacing(qreal v) {
            if (m_spacing != v) {
                m_spacing = v;
                emit changed();
            }
        }
        void setTextScaling(qreal v) {
            if (m_textScaling != v) {
                m_textScaling = v;
                emit changed();
            }
        }

        // ── Border radius ──
        [[nodiscard]] qreal radiusBase() const {
            return m_radiusBase;
        }
        [[nodiscard]] qreal radiusContainer() const {
            return m_radiusContainer;
        }

        void setRadiusBase(qreal v) {
            if (m_radiusBase != v) {
                m_radiusBase = v;
                emit changed();
            }
        }
        void setRadiusContainer(qreal v) {
            if (m_radiusContainer != v) {
                m_radiusContainer = v;
                emit changed();
            }
        }

        // ── Widths ──
        [[nodiscard]] qreal borderWidth() const {
            return m_borderWidth;
        }
        [[nodiscard]] qreal divideWidth() const {
            return m_divideWidth;
        }
        [[nodiscard]] qreal ringWidth() const {
            return m_ringWidth;
        }

        void setBorderWidth(qreal v) {
            if (m_borderWidth != v) {
                m_borderWidth = v;
                emit changed();
            }
        }
        void setDivideWidth(qreal v) {
            if (m_divideWidth != v) {
                m_divideWidth = v;
                emit changed();
            }
        }
        void setRingWidth(qreal v) {
            if (m_ringWidth != v) {
                m_ringWidth = v;
                emit changed();
            }
        }

        // ── Body background ──
        [[nodiscard]] QColor bodyBackgroundColor() const {
            return m_bodyBgColor;
        }
        [[nodiscard]] QColor bodyBackgroundColorDark() const {
            return m_bodyBgColorDark;
        }

        void setBodyBackgroundColor(const QColor &v) {
            if (m_bodyBgColor != v) {
                m_bodyBgColor = v;
                emit changed();
            }
        }
        void setBodyBackgroundColorDark(const QColor &v) {
            if (m_bodyBgColorDark != v) {
                m_bodyBgColorDark = v;
                emit changed();
            }
        }

        // ── Typography ──
        [[nodiscard]] TypographySchema *baseFont() const {
            return m_baseFont;
        }
        [[nodiscard]] TypographySchema *headingFont() const {
            return m_headingFont;
        }
        [[nodiscard]] TypographySchema *anchorFont() const {
            return m_anchorFont;
        }

    signals:
        void changed();

    private:
        qreal m_spacing{4.0}; // 0.25rem × 16 = 4px
        qreal m_textScaling{1.067};
        qreal m_radiusBase{4.0}; // 0.25rem × 16 = 4px
        qreal m_radiusContainer{4.0};
        qreal m_borderWidth{1.0};
        qreal m_divideWidth{1.0};
        qreal m_ringWidth{1.0};
        QColor m_bodyBgColor{Qt::white};
        QColor m_bodyBgColorDark{QColor("#1a1a1a")};

        TypographySchema *m_baseFont;
        TypographySchema *m_headingFont;
        TypographySchema *m_anchorFont;
    };

} // namespace Nandina::Core::Primitives

#endif // NANDINA_PRIMITIVES_HPP
