//
// widget/internal/uniform_row_window — 统一行高虚拟列表的纯窗口数学。
//
// 内容高度、滚动偏移夹取、半开可见窗口、scroll-to-reveal 与行命中。这里只依赖数值和
// 索引，不依赖场景树、ForEach、Key、节点或 reactive 类型；pending-scroll 挂起、数据代次
// 对账、延迟物化与控件定位由 DataTable/ListView 负责。
//
// 可见窗口统一用半开区间 [first, end)：空窗口自然表示为 first == end。
// 语义与提取前逐位一致（含视口底边命中已物化上界的行为）。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_UNIFORM_ROW_WINDOW_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_UNIFORM_ROW_WINDOW_HPP

#include <cstddef>
#include <optional>

namespace nandina::widget::internal
{
    /// 半开可见窗口 [first, end)；空窗口为 first == end，逆序区间也防御性视为空。
    struct RowWindow {
        std::size_t first = 0;
        std::size_t end = 0;

        [[nodiscard]] auto empty() const noexcept -> bool {
            return first >= end;
        }
        [[nodiscard]] auto count() const noexcept -> std::size_t {
            return empty() ? 0 : end - first;
        }
    };

    /// 统一行高、单列滚动的纯窗口数学。
    ///
    /// 调用方保证 `row_height` 有限且 > 0，`viewport_height` 有限且 >= 0；
    /// `item_count` 是数据源全量行数（窗口外的行尚未物化）。本类不拥有任何状态，构造后
    /// 只读；DataTable/ListView 每次布局/滚动时按当前几何构造一个临时实例。
    class UniformRowWindow {
    public:
        constexpr UniformRowWindow(
            float row_height,
            float viewport_height,
            std::size_t item_count
        ) noexcept:
            row_height_(row_height),
            viewport_height_(viewport_height),
            item_count_(item_count) {}

        [[nodiscard]] constexpr auto row_height() const noexcept -> float {
            return row_height_;
        }
        [[nodiscard]] constexpr auto viewport_height() const noexcept -> float {
            return viewport_height_;
        }
        [[nodiscard]] constexpr auto item_count() const noexcept -> std::size_t {
            return item_count_;
        }

        /// 全量内容高度 = 行数 × 行高（float 累积语义与旧实现一致）。
        [[nodiscard]] auto content_height() const noexcept -> float;

        /// 最大滚动偏移 = max(0, 内容高度 - 视口高度)。
        [[nodiscard]] auto maximum_scroll_offset() const noexcept -> float;

        /// 把偏移夹进 [0, maximum_scroll_offset()]。NaN 按 0 处理。
        [[nodiscard]] auto clamp_offset(float offset) const noexcept -> float;

        /// 半开可见窗口 [first, end)。输入 offset 会先夹取；空数据或
        /// 视口高度 <= 0 时返回空窗口。
        [[nodiscard]] auto visible_window(float offset) const noexcept -> RowWindow;

        /// 让 index 行完整可见所需的新 offset（已夹取）。index 越界返回 nullopt；
        /// 视口高度 <= 0 时原样返回 offset——首布局前的挂起由视图决定。
        [[nodiscard]] auto reveal_offset(std::size_t index, float offset) const noexcept
            -> std::optional<float>;

        /// 视口局部 y 命中的行。视口底边（local_y == viewport_height）按已物化窗口
        /// 上界夹取，命中最后一个已物化行而不是未物化的下一行；半行视口不受影响。
        /// offset 会先夹取；超出视口、行号越界、空数据或非有限坐标返回 nullopt。
        [[nodiscard]] auto hit_row(float local_y, float offset, RowWindow window) const noexcept
            -> std::optional<std::size_t>;

    private:
        float row_height_ = 1.0F;
        float viewport_height_ = 0.0F;
        std::size_t item_count_ = 0;
    };
} // namespace nandina::widget::internal

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_UNIFORM_ROW_WINDOW_HPP
