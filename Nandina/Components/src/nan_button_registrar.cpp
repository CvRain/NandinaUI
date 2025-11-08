#include "nan_button_registrar.hpp"
#include "nan_button.hpp"
#include "../component_factory.hpp"
#include "../style_loader.hpp"

namespace Nandina::Components {
    void registerNanButtonFactory()
    {
        static bool registered = false;
        if (!registered) {
            ComponentFactoryRegistry::instance().registerFactory(QStringLiteral("NanButton"),
                [](ComponentManager *manager, const QJsonDocument &doc){
                    StyleLoader<NanButtonStyle>::load(manager, doc);
                }
            );
            registered = true;
        }
    }
}
