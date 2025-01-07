#include "CatppuccinTheme.hpp"

CatppuccinTheme::CatppuccinTheme(CatppuccinPalette palette, QObject *parent)
    : QObject(parent), palette(std::move(palette)) {}

[[nodiscard]] QString CatppuccinTheme::getRosewater() const {
  return palette.rosewater;
}

[[nodiscard]] QString CatppuccinTheme::getFlamingo() const {
  return palette.flamingo;
}

[[nodiscard]] QString CatppuccinTheme::getPink() const { return palette.pink; }

[[nodiscard]] QString CatppuccinTheme::getMauve() const {
  return palette.mauve;
}

[[nodiscard]] QString CatppuccinTheme::getRed() const { return palette.red; }

[[nodiscard]] QString CatppuccinTheme::getMaroon() const {
  return palette.maroon;
}

[[nodiscard]] QString CatppuccinTheme::getPeach() const {
  return palette.peach;
}

[[nodiscard]] QString CatppuccinTheme::getYellow() const {
  return palette.yellow;
}

[[nodiscard]] QString CatppuccinTheme::getGreen() const {
  return palette.green;
}

[[nodiscard]] QString CatppuccinTheme::getTeal() const { return palette.teal; }

[[nodiscard]] QString CatppuccinTheme::getSky() const { return palette.sky; }

[[nodiscard]] QString CatppuccinTheme::getSapphire() const {
  return palette.sapphire;
}

[[nodiscard]] QString CatppuccinTheme::getBlue() const { return palette.blue; }

[[nodiscard]] QString CatppuccinTheme::getLavender() const {
  return palette.lavender;
}

[[nodiscard]] QString CatppuccinTheme::getText() const { return palette.text; }

[[nodiscard]] QString CatppuccinTheme::getSubtext1() const {
  return palette.subtext1;
}

[[nodiscard]] QString CatppuccinTheme::getSubtext0() const {
  return palette.subtext0;
}

[[nodiscard]] QString CatppuccinTheme::getOverlay2() const {
  return palette.overlay2;
}

[[nodiscard]] QString CatppuccinTheme::getOverlay1() const {
  return palette.overlay1;
}

[[nodiscard]] QString CatppuccinTheme::getOverlay0() const {
  return palette.overlay0;
}

[[nodiscard]] QString CatppuccinTheme::getSurface2() const {
  return palette.surface2;
}

[[nodiscard]] QString CatppuccinTheme::getSurface1() const {
  return palette.surface1;
}

[[nodiscard]] QString CatppuccinTheme::getSurface0() const {
  return palette.surface0;
}

[[nodiscard]] QString CatppuccinTheme::getBase() const { return palette.base; }

[[nodiscard]] QString CatppuccinTheme::getMantle() const {
  return palette.mantle;
}

[[nodiscard]] QString CatppuccinTheme::getCrust() const {
  return palette.crust;
}