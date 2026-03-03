//
// Created by cvrain on 2026/3/1.
//

#ifndef NANDINA_PRIMITIVE_SCHEMA_HPP
#define NANDINA_PRIMITIVE_SCHEMA_HPP

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QString>

namespace Nandina::Core::Primitives {

    // ═══════════════════════════════════════════════════════════════
    //  TypographySchema — one font category (base / heading / anchor)
    // ═══════════════════════════════════════════════════════════════

    class TypographySchema : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString fontFamily    READ fontFamily    WRITE setFontFamily    NOTIFY changed)
        Q_PROPERTY(int     fontWeight    READ fontWeight    WRITE setFontWeight    NOTIFY changed)
        Q_PROPERTY(bool    italic        READ italic        WRITE setItalic        NOTIFY changed)
        Q_PROPERTY(qreal   letterSpacing READ letterSpacing WRITE setLetterSpacing NOTIFY changed)
        Q_PROPERTY(QColor  fontColor     READ fontColor     WRITE setFontColor     NOTIFY changed)
        Q_PROPERTY(QColor  fontColorDark READ fontColorDark WRITE setFontColorDark NOTIFY changed)

    public:
        explicit TypographySchema(QObject *parent = nullptr);

        // ── Getters ────────────────────────────────────────────────
        [[nodiscard]] QString fontFamily()    const { return m_fontFamily; }
        [[nodiscard]] int     fontWeight()    const { return m_fontWeight; }
        [[nodiscard]] bool    italic()        const { return m_italic; }
        [[nodiscard]] qreal   letterSpacing() const { return m_letterSpacing; }
        [[nodiscard]] QColor  fontColor()     const { return m_fontColor; }
        [[nodiscard]] QColor  fontColorDark() const { return m_fontColorDark; }

        // ── Setters ────────────────────────────────────────────────
        void setFontFamily(const QString &family);
        void setFontWeight(int weight);
        void setItalic(bool italic);
        void setLetterSpacing(qreal spacing);
        void setFontColor(const QColor &color);
        void setFontColorDark(const QColor &color);

    signals:
        void changed();

    private:
        QString m_fontFamily{QStringLiteral("system-ui")};
        int     m_fontWeight{400};      // QFont::Normal
        bool    m_italic{false};
        qreal   m_letterSpacing{0.0};
        QColor  m_fontColor{Qt::black};
        QColor  m_fontColorDark{Qt::white};
    };

    // ═══════════════════════════════════════════════════════════════
    //  PrimitiveSchema — global design tokens (layout + typography)
    //  Pointers to TypographySchema sub-objects are CONSTANT.
    // ═══════════════════════════════════════════════════════════════

    class PrimitiveSchema : public QObject {
        Q_OBJECT
        QML_ELEMENT

        // ── Layout tokens ──────────────────────────────────────────
        Q_PROPERTY(qreal spacing       READ spacing       WRITE setSpacing       NOTIFY changed)
        Q_PROPERTY(qreal textScaling   READ textScaling   WRITE setTextScaling   NOTIFY changed)

        // ── Radius tokens ──────────────────────────────────────────
        Q_PROPERTY(qreal radiusBase      READ radiusBase      WRITE setRadiusBase      NOTIFY changed)
        Q_PROPERTY(qreal radiusContainer READ radiusContainer WRITE setRadiusContainer NOTIFY changed)

        // ── Border tokens ──────────────────────────────────────────
        Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY changed)
        Q_PROPERTY(qreal divideWidth READ divideWidth WRITE setDivideWidth NOTIFY changed)
        Q_PROPERTY(qreal ringWidth   READ ringWidth   WRITE setRingWidth   NOTIFY changed)

        // ── Background colors ──────────────────────────────────────
        Q_PROPERTY(QColor bodyBackgroundColor     READ bodyBackgroundColor     WRITE setBodyBackgroundColor     NOTIFY changed)
        Q_PROPERTY(QColor bodyBackgroundColorDark READ bodyBackgroundColorDark WRITE setBodyBackgroundColorDark NOTIFY changed)

        // ── Typography sub-objects (CONSTANT pointers) ─────────────
        Q_PROPERTY(Nandina::Core::Primitives::TypographySchema* baseFont    READ baseFont    CONSTANT)
        Q_PROPERTY(Nandina::Core::Primitives::TypographySchema* headingFont READ headingFont CONSTANT)
        Q_PROPERTY(Nandina::Core::Primitives::TypographySchema* anchorFont  READ anchorFont  CONSTANT)

    public:
        explicit PrimitiveSchema(QObject *parent = nullptr);

        // ── Layout getters ─────────────────────────────────────────
        [[nodiscard]] qreal spacing()     const { return m_spacing; }
        [[nodiscard]] qreal textScaling() const { return m_textScaling; }

        // ── Radius getters ─────────────────────────────────────────
        [[nodiscard]] qreal radiusBase()      const { return m_radiusBase; }
        [[nodiscard]] qreal radiusContainer() const { return m_radiusContainer; }

        // ── Border getters ─────────────────────────────────────────
        [[nodiscard]] qreal borderWidth() const { return m_borderWidth; }
        [[nodiscard]] qreal divideWidth() const { return m_divideWidth; }
        [[nodiscard]] qreal ringWidth()   const { return m_ringWidth; }

        // ── Background getters ─────────────────────────────────────
        [[nodiscard]] QColor bodyBackgroundColor()     const { return m_bodyBackgroundColor; }
        [[nodiscard]] QColor bodyBackgroundColorDark() const { return m_bodyBackgroundColorDark; }

        // ── Typography getters ─────────────────────────────────────
        [[nodiscard]] TypographySchema* baseFont()    const { return m_baseFont; }
        [[nodiscard]] TypographySchema* headingFont() const { return m_headingFont; }
        [[nodiscard]] TypographySchema* anchorFont()  const { return m_anchorFont; }

        // ── Layout setters ─────────────────────────────────────────
        void setSpacing(qreal spacing);
        void setTextScaling(qreal scaling);

        // ── Radius setters ─────────────────────────────────────────
        void setRadiusBase(qreal radius);
        void setRadiusContainer(qreal radius);

        // ── Border setters ─────────────────────────────────────────
        void setBorderWidth(qreal width);
        void setDivideWidth(qreal width);
        void setRingWidth(qreal width);

        // ── Background setters ─────────────────────────────────────
        void setBodyBackgroundColor(const QColor &color);
        void setBodyBackgroundColorDark(const QColor &color);

        // ── QML convenience: resolve background by dark mode ───────
        Q_INVOKABLE QColor resolveBodyBackground(bool isDark) const;

    signals:
        void changed();

    private:
        // Layout
        qreal m_spacing{4.0};          // 0.25rem * 16 = 4px
        qreal m_textScaling{1.067};

        // Radius
        qreal m_radiusBase{4.0};       // 0.25rem * 16 = 4px
        qreal m_radiusContainer{4.0};

        // Border
        qreal m_borderWidth{1.0};
        qreal m_divideWidth{1.0};
        qreal m_ringWidth{1.0};

        // Background
        QColor m_bodyBackgroundColor{Qt::white};
        QColor m_bodyBackgroundColorDark{QColor(0x12, 0x12, 0x12)};

        // Typography (CONSTANT pointers, created in ctor)
        TypographySchema *m_baseFont{nullptr};
        TypographySchema *m_headingFont{nullptr};
        TypographySchema *m_anchorFont{nullptr};
    };

} // namespace Nandina::Core::Primitives

#endif // NANDINA_PRIMITIVE_SCHEMA_HPP
