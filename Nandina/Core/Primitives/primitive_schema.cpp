//
// Created by cvrain on 2026/3/1.
//

#include "primitive_schema.hpp"

namespace Nandina::Core::Primitives {

    // ─── TypographySchema ──────────────────────────────────────────

    TypographySchema::TypographySchema(QObject *parent) : QObject(parent) {}

    void TypographySchema::setFontFamily(const QString &family) {
        if (m_fontFamily != family) {
            m_fontFamily = family;
            emit changed();
        }
    }

    void TypographySchema::setFontWeight(int weight) {
        if (m_fontWeight != weight) {
            m_fontWeight = weight;
            emit changed();
        }
    }

    void TypographySchema::setItalic(bool italic) {
        if (m_italic != italic) {
            m_italic = italic;
            emit changed();
        }
    }

    void TypographySchema::setLetterSpacing(qreal spacing) {
        if (!qFuzzyCompare(m_letterSpacing, spacing)) {
            m_letterSpacing = spacing;
            emit changed();
        }
    }

    void TypographySchema::setFontColor(const QColor &color) {
        if (m_fontColor != color) {
            m_fontColor = color;
            emit changed();
        }
    }

    void TypographySchema::setFontColorDark(const QColor &color) {
        if (m_fontColorDark != color) {
            m_fontColorDark = color;
            emit changed();
        }
    }

    // ─── PrimitiveSchema ───────────────────────────────────────────

    PrimitiveSchema::PrimitiveSchema(QObject *parent) : QObject(parent) {
        m_baseFont    = new TypographySchema(this);
        m_headingFont = new TypographySchema(this);
        m_anchorFont  = new TypographySchema(this);

        // Bubble sub-object changes up
        connect(m_baseFont,    &TypographySchema::changed, this, &PrimitiveSchema::changed);
        connect(m_headingFont, &TypographySchema::changed, this, &PrimitiveSchema::changed);
        connect(m_anchorFont,  &TypographySchema::changed, this, &PrimitiveSchema::changed);
    }

    // ── Layout setters ─────────────────────────────────────────────

    void PrimitiveSchema::setSpacing(qreal spacing) {
        if (!qFuzzyCompare(m_spacing, spacing)) {
            m_spacing = spacing;
            emit changed();
        }
    }

    void PrimitiveSchema::setTextScaling(qreal scaling) {
        if (!qFuzzyCompare(m_textScaling, scaling)) {
            m_textScaling = scaling;
            emit changed();
        }
    }

    // ── Radius setters ─────────────────────────────────────────────

    void PrimitiveSchema::setRadiusBase(qreal radius) {
        if (!qFuzzyCompare(m_radiusBase, radius)) {
            m_radiusBase = radius;
            emit changed();
        }
    }

    void PrimitiveSchema::setRadiusContainer(qreal radius) {
        if (!qFuzzyCompare(m_radiusContainer, radius)) {
            m_radiusContainer = radius;
            emit changed();
        }
    }

    // ── Border setters ─────────────────────────────────────────────

    void PrimitiveSchema::setBorderWidth(qreal width) {
        if (!qFuzzyCompare(m_borderWidth, width)) {
            m_borderWidth = width;
            emit changed();
        }
    }

    void PrimitiveSchema::setDivideWidth(qreal width) {
        if (!qFuzzyCompare(m_divideWidth, width)) {
            m_divideWidth = width;
            emit changed();
        }
    }

    void PrimitiveSchema::setRingWidth(qreal width) {
        if (!qFuzzyCompare(m_ringWidth, width)) {
            m_ringWidth = width;
            emit changed();
        }
    }

    // ── Background setters ─────────────────────────────────────────

    void PrimitiveSchema::setBodyBackgroundColor(const QColor &color) {
        if (m_bodyBackgroundColor != color) {
            m_bodyBackgroundColor = color;
            emit changed();
        }
    }

    void PrimitiveSchema::setBodyBackgroundColorDark(const QColor &color) {
        if (m_bodyBackgroundColorDark != color) {
            m_bodyBackgroundColorDark = color;
            emit changed();
        }
    }

    // ── Focus ring setters ─────────────────────────────────────────

    void PrimitiveSchema::setFocusRingColor(const QColor &color) {
        if (m_focusRingColor != color) {
            m_focusRingColor = color;
            emit changed();
        }
    }

    void PrimitiveSchema::setFocusRingColorDark(const QColor &color) {
        if (m_focusRingColorDark != color) {
            m_focusRingColorDark = color;
            emit changed();
        }
    }

    // ── QML convenience ────────────────────────────────────────────

    QColor PrimitiveSchema::resolveBodyBackground(bool isDark) const {
        return isDark ? m_bodyBackgroundColorDark : m_bodyBackgroundColor;
    }

    QColor PrimitiveSchema::resolveFocusRingColor(bool isDark) const {
        return isDark ? m_focusRingColorDark : m_focusRingColor;
    }

} // namespace Nandina::Core::Primitives

