//
// widget/internal/table_selection_model — 二维单元格选择模型（experimental）。
//
// 文件暂留在 internal/ 目录，但 3b③ 起按 experimental 公共头开放。身份用 CellId
// （RowId + ColumnId）；显示序矩形由视图展开成 extent 后传入。首批只实现
// SelectionBehavior::cells：rows/columns 构造函数立即抛 invalid_argument。选择集合按
// 「首次出现顺序」保存为有序 vector；私有成员索引保证 is_selected() O(1) 与 extent 去重。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_TABLE_SELECTION_MODEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_TABLE_SELECTION_MODEL_HPP

#include "../../reactive/event.hpp"
#include "../selection_model.hpp"
#include "data_identity.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

namespace nandina::widget
{
    /// 二维选择行为：首批只实现单元格；整行/整列随 DataTable 导航一起交付。
    enum class SelectionBehavior : std::uint8_t { cells, rows, columns };

    /// 一次选择操作。extent 由视图按显示序展开（矩形/整行/整列）。
    enum class SelectionOp : std::uint8_t { replace, toggle, extend };

    class ITableSelectionModel {
    public:
        virtual ~ITableSelectionModel() = default;

        [[nodiscard]] virtual auto behavior() const noexcept -> SelectionBehavior = 0;
        [[nodiscard]] virtual auto mode() const noexcept -> SelectionMode = 0;
        [[nodiscard]] virtual auto current() const -> std::optional<CellId> = 0;
        [[nodiscard]] virtual auto anchor() const -> std::optional<CellId> = 0;
        [[nodiscard]] virtual auto is_selected(const CellId& cell) const -> bool = 0;
        [[nodiscard]] virtual auto selected() const -> const std::vector<CellId>& = 0;
        [[nodiscard]] virtual auto changed() const -> const reactive::Event<>& = 0;

        /// 清空 selected + anchor，保留 current（键盘游标）。
        virtual void clear_selection() = 0;

        /// 只移动当前单元，不改 selected/anchor；nullopt 清除 current。
        virtual void set_current(std::optional<CellId> current) = 0;

        /// reset/列变更后裁剪 selected/current/anchor；keep 只在调用期间借用。
        /// 先算完整新状态再提交：keep 抛异常时状态不变（强异常保证）。
        virtual void retain(const std::function<bool(const CellId&)>& keep) = 0;

        /// 身份存在性由 DataTable 程序化入口负责校验；apply 只处理已验证身份。
        /// extent 为空视为 {target}；非空 extent 不含 target → invalid_argument。
        virtual void apply(SelectionOp op, CellId target, std::span<const CellId> extent) = 0;
    };

    /// 2A 默认实现：单元格选择、单选/多选、current/anchor、同步 changed 事件。
    ///
    /// 与 RowModelBase 一样禁止拷贝/移动：reactive::Event 的拷贝会共享订阅状态。
    class TableSelectionModel final: public ITableSelectionModel {
    public:
        explicit TableSelectionModel(
            SelectionMode mode = SelectionMode::single,
            SelectionBehavior behavior = SelectionBehavior::cells
        );
        ~TableSelectionModel() override = default;

        TableSelectionModel(const TableSelectionModel&) = delete;
        auto operator=(const TableSelectionModel&) -> TableSelectionModel& = delete;
        TableSelectionModel(TableSelectionModel&&) = delete;
        auto operator=(TableSelectionModel&&) -> TableSelectionModel& = delete;

        [[nodiscard]] auto behavior() const noexcept -> SelectionBehavior override;
        [[nodiscard]] auto mode() const noexcept -> SelectionMode override;
        [[nodiscard]] auto current() const -> std::optional<CellId> override;
        [[nodiscard]] auto anchor() const -> std::optional<CellId> override;
        [[nodiscard]] auto is_selected(const CellId& cell) const -> bool override;
        [[nodiscard]] auto selected() const -> const std::vector<CellId>& override;
        [[nodiscard]] auto changed() const -> const reactive::Event<>& override;

        void clear_selection() override;
        void set_current(std::optional<CellId> current) override;
        void retain(const std::function<bool(const CellId&)>& keep) override;
        void apply(SelectionOp op, CellId target, std::span<const CellId> extent) override;

    private:
        /// 提交 next 状态；只有选择集合真正变化时才重建 membership 索引并发事件。
        void commit(
            std::vector<CellId> next_selected,
            std::optional<CellId> next_current,
            std::optional<CellId> next_anchor
        );

        /// 空 extent → {target}；否则 O(n) 去重并保留首次出现顺序。
        [[nodiscard]] auto
        normalize_extent(const CellId& target, std::span<const CellId> extent) const
            -> std::vector<CellId>;

        SelectionMode mode_;
        SelectionBehavior behavior_;
        std::vector<CellId> selected_;
        std::unordered_set<CellId> selected_index_;
        std::optional<CellId> current_;
        std::optional<CellId> anchor_;
        reactive::Event<> changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_TABLE_SELECTION_MODEL_HPP
