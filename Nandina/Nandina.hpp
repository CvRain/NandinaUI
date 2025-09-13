
#ifndef NANDINA_HPP
#define NANDINA_HPP

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>
#include "CatppuccinLatte.hpp"
#include "CatppuccinFrappe.hpp"
#include "CatppuccinMacchiato.hpp"
#include "CatppuccinMocha.hpp"

class Nandina : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Theme* theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(Theme* Latte READ latte CONSTANT)
    Q_PROPERTY(Theme* Frappe READ frappe CONSTANT)
    Q_PROPERTY(Theme* Macchiato READ macchiato CONSTANT)
    Q_PROPERTY(Theme* Mocha READ mocha CONSTANT)

public:
    explicit Nandina(QObject *parent = nullptr);

    static Nandina *qmlAttachedProperties(QObject *object);
    

    Theme* theme() const;
    void setTheme(Theme *theme);

    Theme* latte() const;
    Theme* frappe() const;
    Theme* macchiato() const;
    Theme* mocha() const;

signals:
    void themeChanged();

private:
    Theme* m_theme;
    Theme* m_latte;
    Theme* m_frappe;
    Theme* m_macchiato;
    Theme* m_mocha;
};

#endif // NANDINA_HPP
