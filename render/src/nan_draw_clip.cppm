//
// nan_draw_clip.cppm — Thread-local draw clip stack
//
// Provides a lightweight clip stack that both the direct ThorVG draw
// path (runtime/widgets) and the Scene-based path (render) can share.
// This avoids maintaining two separate clip implementations.
//

module;

#include <optional>
#include <thorvg-1/thorvg.h>
#include <vector>

export module nandina.render.draw_clip;

export import nandina.foundation.nan_rect;

export namespace nandina::render::draw_clip
{

    struct ClipEntry {
        geometry::NanRect rect {};
        float radius_x {0.0f};
        float radius_y {0.0f};
    };

    // ── Stack management ────────────────────────────────────

    /// Push a new clip entry onto the thread-local stack.
    auto push(const geometry::NanRect& r, float rx = 0.0f, float ry = 0.0f) -> void;

    /// Pop the most recent clip entry.
    auto pop() -> void;

    /// Return the intersection of all active clips, or nullopt if empty.
    [[nodiscard]] auto active() noexcept -> std::optional<ClipEntry>;

    /// Apply the active clip to a paint and add it to the canvas.
    /// Takes ownership of the paint pointer.
    auto paint_with_clip(tvg::SwCanvas& canvas, tvg::Paint* paint) -> void;

    // ── RAII guard ──────────────────────────────────────────

    class ScopedClip {
    public:
        explicit ScopedClip(const geometry::NanRect& r, float rx = 0.0f, float ry = 0.0f);
        ~ScopedClip();
        ScopedClip(const ScopedClip&) = delete;
        auto operator=(const ScopedClip&) = delete;

    private:
        bool m_active {false};
    };

} // namespace nandina::render::draw_clip
