#ifndef NAN_RADIUS_HPP
#define NAN_RADIUS_HPP

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "nan_singleton.hpp"

namespace Nandina::NandinaTokens {
    class NanRadius : public NandinaCore::Types::NanSingleton<NanRadius> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(int xs MEMBER xs_value CONSTANT)
        Q_PROPERTY(int sm MEMBER sm_value CONSTANT)
        Q_PROPERTY(int md MEMBER md_value CONSTANT)
        Q_PROPERTY(int lg MEMBER lg_value CONSTANT)
        Q_PROPERTY(int xl MEMBER xl_value CONSTANT)
        Q_PROPERTY(int full MEMBER full_value CONSTANT)

    public:
        explicit NanRadius(QObject *parent = nullptr) : NandinaCore::Types::NanSingleton<NanRadius>(parent) {
        }

        static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine) {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return getInstance();
        }

        const int xs_value = 2;
        const int sm_value = 4;
        const int md_value = 8;
        const int lg_value = 12;
        const int xl_value = 16;
        const int full_value = 9999;
    };
} // namespace Nandina::NandinaTokens

#endif // NAN_RADIUS_HPP
