//
// Created by cvrain on 2025/11/1.
//

#ifndef TRYNANDINA_COLORSET_HPP
#define TRYNANDINA_COLORSET_HPP

#include <QObject>
#include <qqmlintegration.h>

namespace Nandina {
    class ColorSet : public QObject {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit ColorSet(QObject *parent = nullptr);

        enum class CatppuccinType {
            Latte,
            Frappe,
            Macchiato,
            Mocha
        };

        Q_ENUM(CatppuccinType)
    };
}


#endif //TRYNANDINA_COLORSET_HPP
