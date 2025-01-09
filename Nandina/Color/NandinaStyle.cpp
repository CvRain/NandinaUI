//
// Created by cvrain on 25-1-7.
//
#include "NandinaStyle.hpp"
#include "CatppuccinFrappe.hpp"
#include "CatppuccinLatte.hpp"
#include "CatppuccinMacchiato.hpp"
#include "CatppuccinMocha.hpp"

NandinaStyle::NandinaStyle(QObject *parent)
    : QObject(parent), currentTheme(std::make_shared<CatppuccinLatte>()),
      currentThemeType(NandinaType::CatppuccinThemeType::Latte) {}

QString NandinaStyle::getRosewater() const {
  return currentTheme->getRosewater();
}

QString NandinaStyle::getFlamingo() const {
  return currentTheme->getFlamingo();
}

QString NandinaStyle::getPink() const { return currentTheme->getPink(); }

QString NandinaStyle::getMauve() const { return currentTheme->getMauve(); }

QString NandinaStyle::getRed() const { return currentTheme->getRed(); }

QString NandinaStyle::getMaroon() const { return currentTheme->getMaroon(); }

QString NandinaStyle::getPeach() const { return currentTheme->getPeach(); }

QString NandinaStyle::getYellow() const { return currentTheme->getYellow(); }

QString NandinaStyle::getGreen() const { return currentTheme->getGreen(); }

QString NandinaStyle::getTeal() const { return currentTheme->getTeal(); }

QString NandinaStyle::getSky() const { return currentTheme->getSky(); }

QString NandinaStyle::getSapphire() const {
  return currentTheme->getSapphire();
}

QString NandinaStyle::getBlue() const { return currentTheme->getBlue(); }

QString NandinaStyle::getLavender() const {
  return currentTheme->getLavender();
}

QString NandinaStyle::getText() const { return currentTheme->getText(); }

QString NandinaStyle::getSubtext1() const {
  return currentTheme->getSubtext1();
}

QString NandinaStyle::getSubtext0() const {
  return currentTheme->getSubtext0();
}

QString NandinaStyle::getOverlay2() const {
  return currentTheme->getOverlay2();
}

QString NandinaStyle::getOverlay1() const {
  return currentTheme->getOverlay1();
}

QString NandinaStyle::getOverlay0() const {
  return currentTheme->getOverlay0();
}

QString NandinaStyle::getSurface2() const {
  return currentTheme->getSurface2();
}

QString NandinaStyle::getSurface1() const {
  return currentTheme->getSurface1();
}

QString NandinaStyle::getSurface0() const {
  return currentTheme->getSurface0();
}

QString NandinaStyle::getBase() const { return currentTheme->getBase(); }

QString NandinaStyle::getMantle() const { return currentTheme->getMantle(); }

QString NandinaStyle::getCrust() const { return currentTheme->getCrust(); }

NandinaType::CatppuccinThemeType NandinaStyle::getCurrentThemeType() const {
  return currentThemeType;
}

void NandinaStyle::setCurrentThemeType(
    const NandinaType::CatppuccinThemeType newCurrentThemeType) {
  if (currentThemeType == newCurrentThemeType) {
    return;
  }
  currentThemeType = newCurrentThemeType;
  updateCurrentTheme();
}

void NandinaStyle::updateCurrentTheme() {
  try {

    currentTheme.reset();
    switch (currentThemeType) {
    case NandinaType::CatppuccinThemeType::Frappe:
      currentTheme = std::make_shared<CatppuccinFrappe>();
      break;
    case NandinaType::CatppuccinThemeType::Latte:
      currentTheme = std::make_shared<CatppuccinLatte>();
      break;
    case NandinaType::CatppuccinThemeType::Macchiato:
      currentTheme = std::make_shared<CatppuccinMacchiato>();
      break;
    case NandinaType::CatppuccinThemeType::Mocha:
      currentTheme = std::make_shared<CatppuccinMocha>();
      break;
    default:
      throw std::runtime_error("Invalid theme type");
    }
    emit themeChanged();
    emit currentThemeTypeChanged();
  } catch (std::exception &e) {
    throw;
  }
}
