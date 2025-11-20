//
// Created by cvrain on 2025/11/20.
//

#ifndef TRYNANDINA_NAN_SINGLETON_HPP
#define TRYNANDINA_NAN_SINGLETON_HPP

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>

namespace Nandina::Core::Types {
    template<typename T>
    class NanSingleton : public QObject {
    public:
        // 禁止拷贝和移动
        NanSingleton(const NanSingleton &) = delete;
        NanSingleton &operator=(const NanSingleton &) = delete;
        NanSingleton(NanSingleton &&) = delete;
        NanSingleton &operator=(NanSingleton &&) = delete;

        // C++ 端获取实例的方法
        static T *getInstance() {
            // C++11 保证静态局部变量的初始化是线程安全的
            static T instance;
            return &instance;
        }

        // QML_SINGLETON 宏所需的回调函数
        static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
            Q_UNUSED(engine);
            Q_UNUSED(scriptEngine);
            return getInstance();
        }

    protected:
        // 构造函数设为 protected，防止外部直接创建实例
        explicit NanSingleton(QObject *parent = nullptr) : QObject(parent) {}
        ~NanSingleton() override = default; // 虚析构函数
    };
}

#endif //TRYNANDINA_NAN_SINGLETON_HPP
