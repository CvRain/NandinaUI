
#ifndef NANDINASTYLEPLUGIN_HPP
#define NANDINASTYLEPLUGIN_HPP

#include <QObject>
#include <QQmlExtensionPlugin>

class NandinaPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface" FILE "nandinastyle.json")

public:
    void registerTypes(const char *uri) override;
};

#endif // NANDINASTYLEPLUGIN_HPP
