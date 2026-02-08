#include "nan_card_registrar.hpp"
#include "../component_factory.hpp"
#include "../style_loader.hpp"
#include "nan_card.hpp"

namespace Nandina::Components {
    void registerNanCardFactory() {
        static bool registered = false;
        if (!registered) {
            ComponentFactoryRegistry::instance().registerFactory(
                    QStringLiteral("NanCard"), [](ComponentManager *manager, const QJsonDocument &doc) {
                        StyleLoader<NanCardStyle>::load(manager, doc);
                    });
            registered = true;
        }
    }
} // namespace Nandina::Components
