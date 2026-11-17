//
// widget/internal/data_table — 窗口化物化的 DataTable（批 2B/3a 内部实现）。
//
// 非模板类：行模型经 shared_ptr<IRowModel> 注入；列由 DataTableColumn 唯一定义；单元格
// 视觉全部由 IItemDelegate 提供（内部 HeaderHost 是普通 Label + 排序图元组合，不伪造
// model_row）。2B 做模型绑定/通知状态机、表头指针点击排序、本地/外部排序与默认
// TextItemDelegate；3a 让 DataTable 自持纵向 viewport：表头固定，body 只物化
// UniformRowWindow 可见窗口内的单元格，支持滚轮、offset 夹取、滚动到行与遍历期
// pending 窗口同步。表头键盘/作者 HeaderDelegate/主题重解析/公共导出留到 3b。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_TABLE_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_TABLE_HPP

#include "../../scene/control.hpp"
#include "../../scene/input_event.hpp"
#include "../../widget/build_context.hpp"
#include "data_identity.hpp"
#include "data_table_column.hpp"
#include "grid_navigation.hpp"
#include "item_delegate.hpp"
#include "item_host.hpp"
#include "row_model.hpp"
#include "table_selection_model.hpp"
#include "uniform_row_window.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace nandina::widget
{
    class DataTableHeaderHost;

    class DataTable final: public scene::NanControl {
    public:
        explicit DataTable(
            const widget::BuildContext& ui,
            std::vector<DataTableColumn> columns = {},
            SelectionMode selection_mode = SelectionMode::multiple
        );
        ~DataTable() override;

        [[nodiscard]] static auto create(
            const widget::BuildContext& ui,
            std::vector<DataTableColumn> columns = {},
            SelectionMode selection_mode = SelectionMode::multiple
        ) -> std::shared_ptr<DataTable>;

        // ─── 模型 / 列 ──────────────────────────────────────────────────
        /// 替换模型：先在旧状态上校验现有 columns（token/结构），失败时全部不变；
        /// 成功后才断开旧订阅、替换模型并重建。
        void set_model(std::shared_ptr<IRowModel> model);
        [[nodiscard]] auto model() const noexcept -> IRowModel*;

        /// 事务性列配置：结构/token/排序配置全部校验通过才提交；失败原状态不变。
        void set_columns(std::vector<DataTableColumn> columns);
        [[nodiscard]] auto columns() const noexcept -> const std::vector<DataTableColumn>&;
        [[nodiscard]] auto column(const ColumnId& id) const noexcept -> const DataTableColumn*;

        // ─── 排序 ───────────────────────────────────────────────────────
        /// 相同状态幂等返回 false；external 先提交状态再同步发 sort_requested；
        /// local 在临时索引上 stable_sort，比较器抛异常时不改动状态/显示序。
        auto set_sort(
            std::optional<SortSpec> sort,
            SortRequestReason reason = SortRequestReason::programmatic
        ) -> bool;
        [[nodiscard]] auto sort_spec() const noexcept -> const std::optional<SortSpec>&;
        [[nodiscard]] auto sort_requested() noexcept -> reactive::Event<SortRequest>&;
        void set_on_sort_requested(std::function<void(const SortRequest&)> callback);

        // ─── 选择 / 导航 ─────────────────────────────────────────────────
        [[nodiscard]] auto selection() const noexcept -> const TableSelectionModel&;
        [[nodiscard]] auto navigation() const noexcept -> const GridNavigation&;
        /// 程序化入口先校验行/可见列身份；任一未知身份返回 false 且不改选择状态。
        auto apply_selection(SelectionOp op, CellId target, std::span<const CellId> extent = {})
            -> bool;
        auto set_current_cell(std::optional<CellId> current) -> bool;
        void clear_selection();

        // ─── 查询 / 布局 ─────────────────────────────────────────────────
        [[nodiscard]] auto row_count() const noexcept -> std::size_t;
        [[nodiscard]] auto column_count() const noexcept -> std::size_t;
        /// 已物化的单元格数（可见窗口行 × 可见列）；窗口外的行不创建节点。
        [[nodiscard]] auto item_count() const noexcept -> std::size_t;
        /// 只对已物化单元格返回节点；窗口外返回 nullptr。
        [[nodiscard]] auto cell_node(const CellId& id) const -> scene::NanControl*;
        [[nodiscard]] auto header_node(const ColumnId& id) const -> scene::NanControl*;
        [[nodiscard]] auto model_row_for_display(std::size_t display_row) const
            -> std::optional<std::size_t>;
        [[nodiscard]] auto display_row_for(RowId row) const -> std::optional<std::size_t>;

        void set_row_height(float row_height);
        [[nodiscard]] auto row_height() const noexcept -> float;
        void set_show_header(bool show_header) noexcept;
        [[nodiscard]] auto show_header() const noexcept -> bool;

        // ─── 纵向视口（3a 内部；3b 接输入与键盘）────────────────────────
        /// 当前纵向滚动偏移（已夹取到 [0, maximum_scroll_offset()]）。
        [[nodiscard]] auto scroll_offset() const noexcept -> float;
        /// 全量内容高度超出 body 视口时允许的最大偏移；无可见列时为 0。
        [[nodiscard]] auto maximum_scroll_offset() const noexcept -> float;
        /// 设置偏移（已夹取）；遍历期延迟窗口物化到 post_layout。
        void set_scroll_offset(float offset);
        /// 滚动到让 display_row 行完整可见；首布局前（视口高度未知）挂起到 on_layout。
        void scroll_to_row(std::size_t display_row);

        // ─── 通知状态机（测试/观测）─────────────────────────────────────
        [[nodiscard]] auto has_pending_notifications() const noexcept -> bool;

    protected:
        auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_enter_tree() override;
        void on_ready() override;
        void on_exit_tree() override;

    private:
        void
        validate_columns(const std::vector<DataTableColumn>& columns, const IRowModel* model) const;
        void commit_columns(std::vector<DataTableColumn> columns);
        [[nodiscard]] auto
        build_default_delegates(const std::vector<DataTableColumn>& columns) const
            -> std::vector<std::shared_ptr<IItemDelegate>>;

        [[nodiscard]] auto find_column(const ColumnId& id) const noexcept -> const DataTableColumn*;
        [[nodiscard]] auto find_column_index(const ColumnId& id) const noexcept
            -> std::optional<std::size_t>;
        [[nodiscard]] auto visible_column_indices() const -> std::vector<std::size_t>;
        [[nodiscard]] auto column_widths(float available_width) const -> std::vector<float>;

        void subscribe_model();
        void unsubscribe_model();
        [[nodiscard]] auto traversal_phase() const -> bool;
        void on_model_reset();
        void on_model_data_changed(RowRange range);
        void schedule_pending();
        void apply_pending();
        void apply_data_changes(const std::vector<RowRange>& ranges);
        void rebuild_all();
        void apply_sort_to_display();
        void sync_derived();
        void retain_selection();
        void sync_navigation();
        [[nodiscard]] auto contains_cell(const CellId& cell) const -> bool;
        [[nodiscard]] auto item_state_for(const CellId& cell) const -> ItemState;
        void refresh_cell_states();
        void materialize_cells();
        void update_cells_in_ranges(const std::vector<RowRange>& ranges);
        void handle_header_click(const ColumnId& id);
        void emit_sort_request(std::optional<SortSpec> sort, SortRequestReason reason);

        // ─── 窗口 / 视口────────────────────────────────────────────────
        [[nodiscard]] auto header_height() const noexcept -> float;
        [[nodiscard]] auto body_viewport_height() const noexcept -> float;
        [[nodiscard]] auto window_math() const noexcept -> internal::UniformRowWindow;
        /// 无可见列时没有 body 视口：offset 恒为 0（window_math 的零视口仍会算出
        /// 内容高度，不能直接用于滚动）。
        [[nodiscard]] auto clamped_offset(float offset) const noexcept -> float;
        /// 按当前视口重算偏移/窗口并只物化窗口内单元格（idle 相位调用）。
        void refresh_window();
        /// 遍历期不能改树：挂到 post_layout 再物化窗口。
        void schedule_window_sync();
        void apply_window_sync();
        void position_window();

        widget::BuildContext ui_;
        std::vector<DataTableColumn> columns_;
        std::vector<std::shared_ptr<IItemDelegate>> default_delegates_;
        std::shared_ptr<IRowModel> model_;
        std::vector<std::size_t> display_order_;
        std::unordered_map<RowId, std::size_t, internal::RowIdHash> display_row_for_row_;
        std::vector<std::size_t> visible_columns_;
        std::vector<float> column_offsets_;

        std::shared_ptr<ItemHost> host_;
        std::shared_ptr<DataTableHeaderHost> header_;

        TableSelectionModel selection_;
        GridNavigation navigation_;
        std::optional<SortSpec> sort_;

        reactive::Event<SortRequest> sort_requested_;
        reactive::Subscription sort_callback_sub_;
        std::function<void(const SortRequest&)> user_sort_callback_;
        reactive::Subscription reset_sub_;
        reactive::Subscription changed_sub_;
        reactive::Subscription selection_sub_;
        bool subscribed_ = false;
        bool updating_selection_ = false;

        bool pending_reset_ = false;
        std::vector<RowRange> pending_ranges_;
        bool pending_scheduled_ = false;

        internal::RowWindow visible_window_ {};
        float scroll_offset_ = 0.0F;
        float wheel_step_ = 40.0F;
        bool pending_window_sync_ = false;
        std::optional<std::size_t> pending_scroll_row_;

        float row_height_ = 32.0F;
        bool show_header_ = true;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_TABLE_HPP
