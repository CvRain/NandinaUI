//
// Created by Lingma on 2025/10/28.
//

#ifndef NANDINAUI_BASECOLORS_HPP
#define NANDINAUI_BASECOLORS_HPP

#include <QObject>
#include <QString>
#include <qqmlintegration.h>

namespace Nandina {
    class BaseColors : public QObject {
        Q_OBJECT
        QML_ELEMENT

        // Base color properties
        Q_PROPERTY(QString rosewater MEMBER rosewater CONSTANT)
        Q_PROPERTY(QString flamingo MEMBER flamingo CONSTANT)
        Q_PROPERTY(QString pink MEMBER pink CONSTANT)
        Q_PROPERTY(QString mauve MEMBER mauve CONSTANT)
        Q_PROPERTY(QString red MEMBER red CONSTANT)
        Q_PROPERTY(QString maroon MEMBER maroon CONSTANT)
        Q_PROPERTY(QString peach MEMBER peach CONSTANT)
        Q_PROPERTY(QString yellow MEMBER yellow CONSTANT)
        Q_PROPERTY(QString green MEMBER green CONSTANT)
        Q_PROPERTY(QString teal MEMBER teal CONSTANT)
        Q_PROPERTY(QString sky MEMBER sky CONSTANT)
        Q_PROPERTY(QString sapphire MEMBER sapphire CONSTANT)
        Q_PROPERTY(QString blue MEMBER blue CONSTANT)
        Q_PROPERTY(QString lavender MEMBER lavender CONSTANT)
        Q_PROPERTY(QString text MEMBER text CONSTANT)
        Q_PROPERTY(QString subtext1 MEMBER subtext1 CONSTANT)
        Q_PROPERTY(QString subtext0 MEMBER subtext0 CONSTANT)
        Q_PROPERTY(QString overlay2 MEMBER overlay2 CONSTANT)
        Q_PROPERTY(QString overlay1 MEMBER overlay1 CONSTANT)
        Q_PROPERTY(QString overlay0 MEMBER overlay0 CONSTANT)
        Q_PROPERTY(QString surface2 MEMBER surface2 CONSTANT)
        Q_PROPERTY(QString surface1 MEMBER surface1 CONSTANT)
        Q_PROPERTY(QString surface0 MEMBER surface0 CONSTANT)
        Q_PROPERTY(QString base MEMBER base CONSTANT)
        Q_PROPERTY(QString mantle MEMBER mantle CONSTANT)
        Q_PROPERTY(QString crust MEMBER crust CONSTANT)

    public:
        explicit BaseColors(QObject *parent = nullptr);

        // 拷贝构造函数：不复制父对象，避免对象树混乱
        BaseColors(const BaseColors &other);

        // 赋值运算符：只复制数据成员
        BaseColors &operator=(const BaseColors &other);

        // Color values
        QString rosewater;
        QString flamingo;
        QString pink;
        QString mauve;
        QString red;
        QString maroon;
        QString peach;
        QString yellow;
        QString green;
        QString teal;
        QString sky;
        QString sapphire;
        QString blue;
        QString lavender;
        QString text;
        QString subtext1;
        QString subtext0;
        QString overlay2;
        QString overlay1;
        QString overlay0;
        QString surface2;
        QString surface1;
        QString surface0;
        QString base;
        QString mantle;
        QString crust;
    };
} // namespace Nandina

#endif // NANDINAUI_BASECOLORS_HPP
