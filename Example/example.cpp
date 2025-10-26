#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QDir>
#include <QDirIterator>
#include <QDebug>

Q_IMPORT_QML_PLUGIN(NandinaUIPlugin)

void traverseQrcFiles()
{
    qDebug() << "Traversing qrc:/ files:";
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString filePath = it.next();
        QFileInfo fileInfo = it.fileInfo();

        // 只输出文件，不输出目录
        if (fileInfo.isFile())
        {
            qDebug() << filePath;
        }
    }
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    // 调用函数遍历qrc资源文件
    traverseQrcFiles();

    QQmlApplicationEngine engine;
    const QUrl url{QStringLiteral("qrc:/qt/qml/NandinaExampleApp/Example/Main.qml")};
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* obj, const QUrl& objUrl)
        {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(QUrl(url));

    return app.exec();
}
