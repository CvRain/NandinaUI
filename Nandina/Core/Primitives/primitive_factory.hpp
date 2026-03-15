//
// Created by cvrain on 2026/3/2.
//

#ifndef NANDINA_PRIMITIVE_FACTORY_HPP
#define NANDINA_PRIMITIVE_FACTORY_HPP

#include "primitives_export.hpp"
#include "primitive_schema.hpp"
#include "theme_type.hpp"

namespace Nandina::Core::Primitives {

    class PrimitiveFactory {
    public:
        /// Apply a theme's primitive data (layout tokens + typography) to a PrimitiveSchema.
        /// Font color references (e.g. surface-950 / primary-500) are resolved via the
        /// ColorSchema, so the caller should apply colors BEFORE primitives.
        static NANDINA_PRIMITIVES_EXPORT void applyTheme(Types::ThemeVariant::ThemeTypes theme, PrimitiveSchema *primitives);
    };

} // namespace Nandina::Core::Primitives

#endif // NANDINA_PRIMITIVE_FACTORY_HPP
