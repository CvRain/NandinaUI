#ifndef NANDINAUI_H
#define NANDINAUI_H

#include <QtQuick/QQuickPaintedItem>

class NandinaUI : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY(NandinaUI)
public:
    explicit NandinaUI(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    ~NandinaUI() override;
};

#endif // NANDINAUI_H
