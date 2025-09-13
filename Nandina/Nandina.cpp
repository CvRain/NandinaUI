
#include "Nandina.hpp"

Nandina::Nandina(QObject *parent)
    : QObject(parent),
      m_latte(new CatppuccinLatte(this)),
      m_frappe(new CatppuccinFrappe(this)),
      m_macchiato(new CatppuccinMacchiato(this)),
      m_mocha(new CatppuccinMocha(this))
{
    m_theme = m_latte;
}

Nandina *Nandina::qmlAttachedProperties(QObject *object)
{
    return new Nandina(object);
}

Theme *Nandina::theme() const
{
    return m_theme;
}

void Nandina::setTheme(Theme *theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        emit themeChanged();
    }
}

Theme *Nandina::latte() const
{
    return m_latte;
}

Theme *Nandina::frappe() const
{
    return m_frappe;
}

Theme *Nandina::macchiato() const
{
    return m_macchiato;
}

Theme *Nandina::mocha() const
{
    return m_mocha;
}
