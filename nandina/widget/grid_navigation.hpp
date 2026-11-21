//
// widget/grid_navigation — 二维网格键盘导航（experimental）。
//
// 只持有显示序 ModelIndex，不持有身份、不映射到 CellId；视图负责身份映射与把 current
// 写入选择模型。空网格不产生虚假 (0,0)；空→非空不自动播种；无 current 时所有移动返回
// false；方向键不环绕；Home/End 为当前行首尾；PageUp/Down 保持列。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_GRID_NAVIGATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_GRID_NAVIGATION_HPP

#include "data_identity.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace nandina::widget
{
    enum class GridDirection : std::uint8_t { up, down, left, right };

    class GridNavigation {
    public:
        GridNavigation() = default;

        /// 设置显示态形状。0 行/0 列 → 清空 current；非空时夹取已有 current。
        /// 空网格转非空不会自动播种 (0,0)，current 保持 nullopt。
        void set_shape(std::size_t rows, std::size_t columns) noexcept;

        /// 视口可见行数（PageUp/PageDown 步长）；0 表示禁用翻页。
        void set_page_rows(std::size_t rows) noexcept;

        /// 非空网格夹取到合法范围；空网格清空 current。
        void set_current(ModelIndex index) noexcept;

        /// 显式清除键盘游标（视图在身份失效/离开网格时调用）。
        void clear_current() noexcept;

        [[nodiscard]] auto current() const noexcept -> std::optional<ModelIndex>;

        /// 方向键：上下保持列、左右保持行；到边界不环绕，未移动返回 false。
        auto move(GridDirection direction) noexcept -> bool;

        /// 当前行首/行尾：有 current 即视为已处理（即使已在端点），无 current 返回 false。
        auto home() noexcept -> bool;
        auto end() noexcept -> bool;

        /// 保持列翻页；page_rows == 0、无 current 或已到边界（无位移）返回 false。
        auto page_up() noexcept -> bool;
        auto page_down() noexcept -> bool;

        /// 需要保证可见的显示行；无 current 时 nullopt。
        [[nodiscard]] auto scroll_to_index() const noexcept -> std::optional<std::size_t>;

    private:
        [[nodiscard]] auto is_empty() const noexcept -> bool;
        [[nodiscard]] auto clamp_index(ModelIndex index) const noexcept -> ModelIndex;

        std::size_t rows_ = 0;
        std::size_t columns_ = 0;
        std::size_t page_rows_ = 0;
        std::optional<ModelIndex> current_ = std::nullopt;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_GRID_NAVIGATION_HPP
