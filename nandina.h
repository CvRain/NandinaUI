#ifndef NANDINA_H
#define NANDINA_H

#include <QtQuick/QQuickPaintedItem>

class Nandina : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY(Nandina)
public:
    explicit Nandina(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    ~Nandina() override;
};

#endif // NANDINA_H
