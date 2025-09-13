#include "NandinaPlugin.hpp"
#include <QtQml/qqml.h>
#include "Nandina.hpp"
#include "Theme.hpp"
#include "CatppuccinLatte.hpp"
#include "CatppuccinFrappe.hpp"
#include "CatppuccinMacchiato.hpp"
#include "CatppuccinMocha.hpp"
#include <QtQml/qqml.h>

void NandinaPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<Theme>(uri, 1, 0, "Theme");
    qmlRegisterType<CatppuccinLatte>(uri, 1, 0, "CatppuccinLatte");
    qmlRegisterType<CatppuccinFrappe>(uri, 1, 0, "CatppuccinFrappe");
    qmlRegisterType<CatppuccinMacchiato>(uri, 1, 0, "CatppuccinMacchiato");
    qmlRegisterType<CatppuccinMocha>(uri, 1, 0, "CatppuccinMocha");
    qmlRegisterAttachedPropertiesType<Nandina>(uri, 1, 0, "Nandina");
}