//
// scene/clipboard - platform-neutral text clipboard and edit command boundary.
//

#ifndef NANDINA_EXPERIMENT_SCENE_CLIPBOARD_HPP
#define NANDINA_EXPERIMENT_SCENE_CLIPBOARD_HPP

#include <optional>
#include <string>
#include <string_view>

namespace nandina::scene
{
    enum class EditCommand { select_all, copy, cut, paste, undo, redo };

    class IClipboard {
    public:
        virtual ~IClipboard() = default;

        [[nodiscard]] virtual auto read_text() const -> std::optional<std::string> = 0;
        virtual auto write_text(std::string_view text) -> bool = 0;
    };
} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_SCENE_CLIPBOARD_HPP
