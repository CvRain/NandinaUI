//
// theme/appearance — color appearance and preference selection.
//
// Extracted from theme_manager.hpp so the design-system layer (appearance-aware
// palette resolution) does not depend on the theme manager.
//

#ifndef NANDINA_EXPERIMENT_THEME_APPEARANCE_HPP
#define NANDINA_EXPERIMENT_THEME_APPEARANCE_HPP

namespace nandina::theme
{
    /// Resolved color appearance. `system` preference is resolved to one of these.
    enum class ColorAppearance {
        light,
        dark,
    };

    /**
     * User-facing appearance preference.
     *
     * `system` resolves from the latest SystemPreferences snapshot supplied by the platform host.
     * ThemeManager does not observe the operating system itself and falls back to light.
     */
    enum class ThemePreference {
        system,
        light,
        dark,
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_APPEARANCE_HPP
