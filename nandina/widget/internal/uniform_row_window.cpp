#include "uniform_row_window.hpp"

#include <algorithm>
#include <cmath>

namespace nandina::widget::internal
{
    namespace
    {
        /// 把浮点行坐标夹进 [0, total-1]（total == 0 时返回 0）。
        /// 在转 size_t 之前先做范围比较，避免负数或过大浮点转整型 UB。
        auto clamp_row(const float row, const std::size_t total) -> std::size_t {
            if (total == 0 || std::isnan(row) || row <= 0.0F) {
                return 0;
            }
            const auto maximum = static_cast<long double>(total - 1);
            if (static_cast<long double>(row) >= maximum) {
                return total - 1;
            }
            const auto row_index = static_cast<std::size_t>(std::floor(row));
            return std::min(row_index, total - 1);
        }
    } // namespace

    auto UniformRowWindow::content_height() const noexcept -> float {
        return static_cast<float>(item_count_) * row_height_;
    }

    auto UniformRowWindow::maximum_scroll_offset() const noexcept -> float {
        return std::max(0.0F, content_height() - viewport_height_);
    }

    auto UniformRowWindow::clamp_offset(const float offset) const noexcept -> float {
        if (std::isnan(offset)) {
            return 0.0F;
        }
        return std::clamp(offset, 0.0F, maximum_scroll_offset());
    }

    auto UniformRowWindow::visible_window(const float offset) const noexcept -> RowWindow {
        if (item_count_ == 0) {
            return {};
        }
        const float viewport = std::max(0.0F, viewport_height_);
        if (viewport <= 0.0F) {
            return {};
        }
        const float clamped_offset = clamp_offset(offset);
        const std::size_t first = clamp_row(clamped_offset / row_height_, item_count_);
        const std::size_t last =
            clamp_row(std::ceil((clamped_offset + viewport) / row_height_) - 1.0F, item_count_);
        // 旧实现返回闭区间 {first, last}；这里统一成半开 [first, max(first,last)+1)。
        return {.first = first, .end = std::max(first, last) + 1};
    }

    auto UniformRowWindow::reveal_offset(const std::size_t index, const float offset) const noexcept
        -> std::optional<float> {
        if (index >= item_count_) {
            return std::nullopt;
        }
        if (viewport_height_ <= 0.0F) {
            return offset;
        }
        const float current = clamp_offset(offset);
        const float row_top = static_cast<float>(index) * row_height_;
        const float row_bottom = row_top + row_height_;
        float target = current;
        if (row_top < current) {
            target = row_top;
        }
        else if (row_bottom > current + viewport_height_) {
            target = row_bottom - viewport_height_;
        }
        // 旧路径经由 set_scroll_offset() 统一夹取；纯函数同样返回可用的 offset。
        return clamp_offset(target);
    }

    auto UniformRowWindow::hit_row(
        const float local_y,
        const float offset,
        const RowWindow window
    ) const noexcept -> std::optional<std::size_t> {
        if (local_y < 0.0F || local_y > viewport_height_) {
            return std::nullopt;
        }
        const float row = (clamp_offset(offset) + local_y) / row_height_;
        if (!std::isfinite(row) || row < 0.0F) {
            return std::nullopt;
        }
        const auto index = static_cast<std::size_t>(std::floor(row));
        if (index >= item_count_) {
            return std::nullopt;
        }
        // 底边（local_y == viewport_height，视口高为行高整数倍时）按未物化的下一行
        // 命中：夹回已物化窗口上界，避免点击底边选中不可见行。半可见行仍在窗口内。
        const std::size_t window_end = std::min(window.end, item_count_);
        if (!window.empty() && window.first < window_end && index >= window_end) {
            return window_end - 1;
        }
        return index;
    }
} // namespace nandina::widget::internal
