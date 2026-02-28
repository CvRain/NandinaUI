//
// Created by cvrain on 2026/2/28.
//

#ifndef NANDINA_TYPOGRAPHY_SCHEMA_HPP
#define NANDINA_TYPOGRAPHY_SCHEMA_HPP

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QString>

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
     * @brief Typography settings for a text category (base, heading, anchor).
     *
     * Maps to Skeleton's CSS typography tokens:
     * - --base-font-family, --heading-font-family, --anchor-font-family
     * - --*-font-weight, --*-font-style, --*-letter-spacing
     * - --*-font-color / --*-font-color-dark
     */
    class NANDINA_PRIMITIVES_EXPORT TypographySchema : public QObject {
        Q_OBJECT

        Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY changed)
        Q_PROPERTY(int fontWeight READ fontWeight WRITE setFontWeight NOTIFY changed)
        Q_PROPERTY(bool italic READ italic WRITE setItalic NOTIFY changed)
        Q_PROPERTY(qreal letterSpacing READ letterSpacing WRITE setLetterSpacing NOTIFY changed)
        Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor NOTIFY changed)
        Q_PROPERTY(QColor fontColorDark READ fontColorDark WRITE setFontColorDark NOTIFY changed)

    public:
        explicit TypographySchema(QObject *parent = nullptr) : QObject(parent) {
        }

        [[nodiscard]] QString fontFamily() const {
            return m_fontFamily;
        }
        [[nodiscard]] int fontWeight() const {
            return m_fontWeight;
        }
        [[nodiscard]] bool italic() const {
            return m_italic;
        }
        [[nodiscard]] qreal letterSpacing() const {
            return m_letterSpacing;
        }
        [[nodiscard]] QColor fontColor() const {
            return m_fontColor;
        }
        [[nodiscard]] QColor fontColorDark() const {
            return m_fontColorDark;
        }

        void setFontFamily(const QString &v) {
            if (m_fontFamily != v) {
                m_fontFamily = v;
                emit changed();
            }
        }
        void setFontWeight(int v) {
            if (m_fontWeight != v) {
                m_fontWeight = v;
                emit changed();
            }
        }
        void setItalic(bool v) {
            if (m_italic != v) {
                m_italic = v;
                emit changed();
            }
        }
        void setLetterSpacing(qreal v) {
            if (m_letterSpacing != v) {
                m_letterSpacing = v;
                emit changed();
            }
        }
        void setFontColor(const QColor &v) {
            if (m_fontColor != v) {
                m_fontColor = v;
                emit changed();
            }
        }
        void setFontColorDark(const QColor &v) {
            if (m_fontColorDark != v) {
                m_fontColorDark = v;
                emit changed();
            }
        }

    signals:
        void changed();

    private:
        QString m_fontFamily{QStringLiteral("system-ui")};
        int m_fontWeight{400}; // QFont::Normal = 400
        bool m_italic{false};
        qreal m_letterSpacing{0.0};
        QColor m_fontColor{Qt::black};
        QColor m_fontColorDark{Qt::white};
    };

} // namespace Nandina::Core::Primitives

#endif // NANDINA_TYPOGRAPHY_SCHEMA_HPP
