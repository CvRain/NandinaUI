#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QDirIterator>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/ExampleProjectApp/NandianExample/Main.qml")));

    QDirIterator it(":/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const auto &filePath = it.next();
        if(filePath.contains("qt-project.org")){
            continue;
        }
        qDebug() << filePath;
    }

    return QGuiApplication::exec();
}
