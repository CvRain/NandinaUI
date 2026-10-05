// foundation/layout_constraints - pure size constraints shared by layout consumers.

#ifndef NANDINA_EXPERIMENT_FOUNDATION_LAYOUT_CONSTRAINTS_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_LAYOUT_CONSTRAINTS_HPP

#include "geometry.hpp"

#include <limits>

namespace nandina::foundation
{
    /// Logical size bounds, independent of scene nodes and text backends.
    /// Public fields retain aggregate initialization and existing numeric semantics.
    struct NanLayoutConstraints {
        float min_width = 0.0F;
        float max_width = std::numeric_limits<float>::infinity();
        float min_height = 0.0F;
        float max_height = std::numeric_limits<float>::infinity();

        [[nodiscard]] static auto loose() -> NanLayoutConstraints;
        [[nodiscard]] static auto tight(NanSize size) -> NanLayoutConstraints;
        [[nodiscard]] auto constrain(NanSize size) const -> NanSize;
        [[nodiscard]] auto deflated(NanInsets insets) const -> NanLayoutConstraints;
    };
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_FOUNDATION_LAYOUT_CONSTRAINTS_HPP
