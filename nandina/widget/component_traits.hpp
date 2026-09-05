//
// widget/component_traits - typed authoring customization point.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_COMPONENT_TRAITS_HPP
#define NANDINA_EXPERIMENT_WIDGET_COMPONENT_TRAITS_HPP

namespace nandina::widget
{
    /**
     * Specialize beside a component's authoring adapter to teach BuildContext::make<T>()
     * how to inject ambient services and bindings. The primary template is intentionally
     * incomplete: unsupported construction produces a local compile-time diagnostic.
     */
    template<typename Component>
    struct ComponentTraits;
}

#endif // NANDINA_EXPERIMENT_WIDGET_COMPONENT_TRAITS_HPP
