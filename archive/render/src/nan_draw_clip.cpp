//
// nan_draw_clip.cpp — Thread-local draw clip stack implementation
//

module;

#include <algorithm>
#include <optional>
#include <thorvg-1/thorvg.h>
#include <vector>

module nandina.render.draw_clip;

namespace nandina::render::draw_clip
{

    namespace
    {

        thread_local std::vector<ClipEntry> s_stack;

    } // anonymous namespace

    auto push(const geometry::NanRect& r, float rx, float ry) -> void {
        s_stack.push_back(ClipEntry {r, rx, ry});
    }

    auto pop() -> void {
            if (!s_stack.empty()) {
                s_stack.pop_back();
            }
    }

    auto active() noexcept -> std::optional<ClipEntry> {
            if (s_stack.empty()) {
                return std::nullopt;
            }

        auto combined = s_stack.front();
            for (std::size_t i = 1; i < s_stack.size(); ++i) {
                const auto& next = s_stack[i];
                combined.rect = combined.rect.intersected(next.rect);
                combined.radius_x = next.radius_x;
                combined.radius_y = next.radius_y;
            }

            if (combined.rect.width() <= 0.0f || combined.rect.height() <= 0.0f) {
                return ClipEntry {
                    geometry::NanRect {
                        geometry::NanPoint {0.0f, 0.0f},
                        geometry::NanSize {0.0f, 0.0f}
                    },
                    0.0f,
                    0.0f
                };
            }

        return combined;
    }

    auto paint_with_clip(tvg::SwCanvas& canvas, tvg::Paint* paint) -> void {
            if (const auto clip = active()) {
                auto* clipper = tvg::Shape::gen();
                clipper->appendRect(
                    clip->rect.x(),
                    clip->rect.y(),
                    clip->rect.width(),
                    clip->rect.height(),
                    clip->radius_x,
                    clip->radius_y
                );
                paint->clip(clipper);
            }
        canvas.add(paint);
    }

    ScopedClip::ScopedClip(const geometry::NanRect& r, float rx, float ry) {
            if (r.width() > 0.0f && r.height() > 0.0f) {
                s_stack.push_back(ClipEntry {r, rx, ry});
                m_active = true;
            }
    }

    ScopedClip::~ScopedClip() {
            if (m_active) {
                s_stack.pop_back();
            }
    }

} // namespace nandina::render::draw_clip
