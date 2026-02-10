#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <color_schema.hpp>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qDebug() << Nandina::Color::function() ;

    QQmlApplicationEngine engine;
    const QUrl url("qrc:/qt/qml/NandinaExample/Main.qml");

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
