#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDirIterator>
#include <QString>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../");
    
    const QUrl url(QStringLiteral("qrc:/qt/qml/NandinaExample/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    // QDirIterator it(":/", QDirIterator::Subdirectories);
    // while (it.hasNext()) {
    //     const auto &filePath = it.next();
    //     if(filePath.contains("qt-project.org")){
    //         continue;
    //     }
    //     qDebug() << filePath;
    // }

    return app.exec();
}
