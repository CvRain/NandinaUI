
#include "Theme.hpp"

Theme::Theme(const Palette &palette, QObject *parent)
    : QObject(parent), m_palette(palette)
{
}

QString Theme::rosewater() const { return m_palette.rosewater; }
QString Theme::flamingo() const { return m_palette.flamingo; }
QString Theme::pink() const { return m_palette.pink; }
QString Theme::mauve() const { return m_palette.mauve; }
QString Theme::red() const { return m_palette.red; }
QString Theme::maroon() const { return m_palette.maroon; }
QString Theme::peach() const { return m_palette.peach; }
QString Theme::yellow() const { return m_palette.yellow; }
QString Theme::green() const { return m_palette.green; }
QString Theme::teal() const { return m_palette.teal; }
QString Theme::sky() const { return m_palette.sky; }
QString Theme::sapphire() const { return m_palette.sapphire; }
QString Theme::blue() const { return m_palette.blue; }
QString Theme::lavender() const { return m_palette.lavender; }
QString Theme::text() const { return m_palette.text; }
QString Theme::subtext1() const { return m_palette.subtext1; }
QString Theme::subtext0() const { return m_palette.subtext0; }
QString Theme::overlay2() const { return m_palette.overlay2; }
QString Theme::overlay1() const { return m_palette.overlay1; }
QString Theme::overlay0() const { return m_palette.overlay0; }
QString Theme::surface2() const { return m_palette.surface2; }
QString Theme::surface1() const { return m_palette.surface1; }
QString Theme::surface0() const { return m_palette.surface0; }
QString Theme::base() const { return m_palette.base; }
QString Theme::mantle() const { return m_palette.mantle; }
QString Theme::crust() const { return m_palette.crust; }
