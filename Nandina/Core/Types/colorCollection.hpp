//
// Created by CvRai on 2025/11/2.
//

#ifndef TRYNANDINA_COLORCOLLECTION_HPP
#define TRYNANDINA_COLORCOLLECTION_HPP

#include <QObject>
#include <qqmlintegration.h>

namespace Nandina::Core::Types {
    QT_BEGIN_NAMESPACE
    Q_NAMESPACE

    class CatppuccinSetting : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        explicit CatppuccinSetting(QObject *parent = nullptr);

        enum class CatppuccinType {
            Latte,
            Frappe,
            Macchiato,
            Mocha
        };

        Q_ENUM(CatppuccinType)

        static QString catppuccinTypeToString(CatppuccinType type);

        static CatppuccinType stringToCatppuccinType(const QString &name);
    };


    QT_END_NAMESPACE
}

#endif //TRYNANDINA_COLORCOLLECTION_HPP
