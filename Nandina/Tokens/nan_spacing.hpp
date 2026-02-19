#ifndef NAN_SPACING_HPP
#define NAN_SPACING_HPP

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>

#include "nan_singleton.hpp"

namespace Nandina::NandinaTokens {
    class NanSpacing : public NandinaCore::Types::NanSingleton<NanSpacing> {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(int xs MEMBER xs_value CONSTANT)
        Q_PROPERTY(int sm MEMBER sm_value CONSTANT)
        Q_PROPERTY(int md MEMBER md_value CONSTANT)
        Q_PROPERTY(int lg MEMBER lg_value CONSTANT)
        Q_PROPERTY(int xl MEMBER xl_value CONSTANT)
        Q_PROPERTY(int xxl MEMBER xxl_value CONSTANT)
        Q_PROPERTY(int test MEMBER test_number CONSTANT)

    public:
        explicit NanSpacing(QObject *parent = nullptr) : NandinaCore::Types::NanSingleton<NanSpacing>(parent) {
        }

        static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine) {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return getInstance();
        }
        const int xs_value = 4;
        const int sm_value = 8;
        const int md_value = 12;
        const int lg_value = 16;
        const int xl_value = 24;
        const int xxl_value = 32;
        const int test_number = 42;
    };
} // namespace Nandina::NandinaTokens
#endif // NAN_SPACING_HPP
