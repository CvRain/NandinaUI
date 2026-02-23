#ifndef NAN_MOTION_HPP
#define NAN_MOTION_HPP

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "nan_singleton.hpp"

namespace Nandina::NandinaTokens {
    class NanMotion : public NandinaCore::Types::NanSingleton<NanMotion> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(int instant MEMBER instant_value CONSTANT)
        Q_PROPERTY(int fast MEMBER fast_value CONSTANT)
        Q_PROPERTY(int normal MEMBER normal_value CONSTANT)
        Q_PROPERTY(int slow MEMBER slow_value CONSTANT)
        Q_PROPERTY(int bounceIn MEMBER bounce_in_value CONSTANT)
        Q_PROPERTY(int bounceOut MEMBER bounce_out_value CONSTANT)

    public:
        explicit NanMotion(QObject *parent = nullptr) : NandinaCore::Types::NanSingleton<NanMotion>(parent) {
        }

        static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine) {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return getInstance();
        }

        const int instant_value = 0;
        const int fast_value = 120;
        const int normal_value = 180;
        const int slow_value = 260;
        const int bounce_in_value = 70;
        const int bounce_out_value = 140;
    };
}

#endif // NAN_MOTION_HPP
