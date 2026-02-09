#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "../Nandina/Components/component_registrar.hpp"


int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    Nandina::Components::registerAllComponents();

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/qt/qml/NandinaExample/Main.qml"_qs);

    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreated, &app,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl) {
                    QCoreApplication::exit(-1);
                }
            },
            Qt::QueuedConnection);

    engine.loadFromModule("NandinaExample", "Main");

    return app.exec();
}
