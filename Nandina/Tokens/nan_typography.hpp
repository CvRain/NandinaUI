#ifndef NAN_TYPOGRAPHY_HPP
#define NAN_TYPOGRAPHY_HPP

#include <QObject>
#include <QFont>
#include <QQmlEngine>
#include <QJSEngine>

#include "nan_singleton.hpp"

namespace Nandina::NandinaTokens {
    class NanTypography : public NandinaCore::Types::NanSingleton<NanTypography> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QFont display MEMBER display_font CONSTANT)
        Q_PROPERTY(QFont titleLarge MEMBER title_large_font CONSTANT)
        Q_PROPERTY(QFont title MEMBER title_font CONSTANT)
        Q_PROPERTY(QFont subtitle MEMBER subtitle_font CONSTANT)
        Q_PROPERTY(QFont bodyLarge MEMBER body_large_font CONSTANT)
        Q_PROPERTY(QFont body MEMBER body_font CONSTANT)
        Q_PROPERTY(QFont bodyStrong MEMBER body_strong_font CONSTANT)
        Q_PROPERTY(QFont caption MEMBER caption_font CONSTANT)

    public:
        explicit NanTypography(QObject *parent = nullptr);

        static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine) {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return getInstance();
        }

    private:
        const QFont display_font;
        const QFont title_large_font;
        const QFont title_font;
        const QFont subtitle_font;
        const QFont body_large_font;
        const QFont body_font;
        const QFont body_strong_font;
        const QFont caption_font;

        static QFont makeFont(int pixelSize, QFont::Weight weight);
    };
}

#endif // NAN_TYPOGRAPHY_HPP
