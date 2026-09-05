//
// theme/motion - application and system motion preference resolution.
//

#ifndef NANDINA_EXPERIMENT_THEME_MOTION_HPP
#define NANDINA_EXPERIMENT_THEME_MOTION_HPP

#include <cstdint>

namespace nandina::theme
{
    /**
     * User-facing motion preference. `system` resolves from the latest host-supplied
     * SystemPreferences snapshot; ThemeManager falls back to full motion.
     */
    enum class MotionPreference : std::uint8_t {
        system,
        full,
        reduced,
    };

    [[nodiscard]] constexpr auto resolve_reduced_motion(
        const MotionPreference preference,
        const bool system_reduced_motion
    ) noexcept -> bool {
        switch (preference) {
            case MotionPreference::system:
                return system_reduced_motion;
            case MotionPreference::full:
                return false;
            case MotionPreference::reduced:
                return true;
        }
        return system_reduced_motion;
    }
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_MOTION_HPP
