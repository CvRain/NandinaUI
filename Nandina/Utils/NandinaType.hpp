#pragma once 

#include <QObject>
#include <qqmlintegration.h>

class NandinaType: public QObject{
    Q_OBJECT
    QML_ELEMENT

public:
	explicit NandinaType(QObject *parent = nullptr);

    enum class AlertType: int{
        Success,
        Error,
        Warning,
        Info
    };
    Q_ENUM(AlertType)

    enum class CatppuccinThemeType: int{
        Latte,
        Frappe,
        Macchiato,
        Mocha
    };
    Q_ENUM(CatppuccinThemeType)
};
