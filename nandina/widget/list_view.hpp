//
// widget/list_view — 1D virtualized data list over IRowModel + IItemDelegate.
//
// experimental public header. Non-template: rows come from IRowModel; row visuals from
// IItemDelegate; selection uses SelectionModel<RowId>. Uniform row height; the view is its
// own vertical scroll viewport and materializes only the visible window through ItemHost.
//
// Relationship to the other list facilities:
//   * ForEach stays the declarative full-list keyed region.
//   * ListView (this class) is the virtualized data view; it does not wrap ForEach.
//
// Rules and boundaries: docs/references/data_components_and_virtualization.md §9.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_LIST_VIEW_HPP
#define NANDINA_EXPERIMENT_WIDGET_LIST_VIEW_HPP

#include "../scene/control.hpp"
#include "../scene/input_event.hpp"
#include "build_context.hpp"
#include "data_identity.hpp"
#include "internal/uniform_row_window.hpp"
#include "item_delegate.hpp"
#include "row_model.hpp"
#include "selection_model.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace nandina::widget
{
    class ItemHost;

    /// 1D 虚拟化数据列表：统一行高、自持纵向视口、只物化可见窗口。
    ///
    /// 必须以 shared_ptr 持有（create() 构造）：布局期延迟物化用 shared_from_this()
    /// 挂 post_layout 动作（与 DataTable/ScrollView 相同的共享所有权约定），栈上实例
    /// 会在首布局时触发 bad_weak_ptr。行控件由 IItemDelegate 创建，行内交互控件先
    /// 消费事件；列表只在冒泡到手时做命中→选择。
    class ListView final: public scene::NanControl {
    public:
        explicit ListView(
            const widget::BuildContext& ui,
            SelectionMode selection_mode = SelectionMode::multiple,
            float row_height = 36.0F
        );
        ~ListView() override;

        [[nodiscard]] static auto create(
            const widget::BuildContext& ui,
            SelectionMode selection_mode = SelectionMode::multiple,
            float row_height = 36.0F
        ) -> std::shared_ptr<ListView>;

        // ─── 模型 / delegate ────────────────────────────────────────────
        /// 替换模型：先校验现有 delegate 的令牌，失败时旧状态不变。
        void set_model(std::shared_ptr<IRowModel> model);
        [[nodiscard]] auto model() const noexcept -> IRowModel*;
        /// 替换行视觉：令牌不匹配抛 invalid_argument；旧 delegate 控件整体重建。
        void set_delegate(std::shared_ptr<IItemDelegate> delegate);
        [[nodiscard]] auto delegate() const noexcept -> const std::shared_ptr<IItemDelegate>&;

        // ─── 行高 / 查询 ────────────────────────────────────────────────
        /// 统一行高：必须有限且 > 0；行内自然高度不改变行高。
        void set_row_height(float row_height);
        [[nodiscard]] auto row_height() const noexcept -> float;

        /// 全量行数（含未物化的行）。
        [[nodiscard]] auto row_count() const noexcept -> std::size_t;
        /// 已物化行数（可见窗口内的行）；窗口外的行不创建节点。
        [[nodiscard]] auto item_count() const noexcept -> std::size_t;
        /// 只对已物化行返回节点；窗口外返回 nullptr。
        [[nodiscard]] auto row_node(RowId row) const -> scene::NanControl*;
        [[nodiscard]] auto display_row_for(RowId row) const -> std::optional<std::size_t>;
        [[nodiscard]] auto row_id_for_display(std::size_t display_row) const
            -> std::optional<RowId>;

        // ─── 选择 ──────────────────────────────────────────────────────
        [[nodiscard]] auto selection() const noexcept -> const SelectionModel<RowId>&;
        /// 程序化选择：身份有效返回 true（合法但已选中仍为 true），未知身份返回 false；
        /// single 替换、multiple 追加；随后滚动到该行。
        auto select(RowId row) -> bool;
        void clear_selection();
        [[nodiscard]] auto selection_changed() const noexcept -> const reactive::Event<>&;
        void set_on_selection_changed(std::function<void()> callback);

        // ─── 纵向视口 ──────────────────────────────────────────────────
        [[nodiscard]] auto scroll_offset() const noexcept -> float;
        [[nodiscard]] auto maximum_scroll_offset() const noexcept -> float;
        /// 设置偏移（已夹取）；遍历期把窗口增删物化延迟到 post_layout。
        void set_scroll_offset(float offset);
        /// 滚动到让 display_row 行完整可见；首布局前（视口高度未知）挂起到 on_layout。
        void scroll_to_row(std::size_t display_row);
        void set_wheel_step(float step);

        // ─── 激活 ──────────────────────────────────────────────────────
        [[nodiscard]] auto activated() const noexcept -> const reactive::Event<RowId>&;
        void set_on_activated(std::function<void(RowId)> callback);

        [[nodiscard]] auto has_pending_notifications() const noexcept -> bool;

    protected:
        [[nodiscard]] auto is_focusable() const -> bool override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_enter_tree() override;
        void on_ready() override;
        void on_exit_tree() override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        void subscribe_model();
        void unsubscribe_model();
        [[nodiscard]] auto traversal_phase() const -> bool;
        void on_model_reset();
        void on_model_data_changed(RowRange range);
        void schedule_pending();
        void apply_pending();
        void rebuild_all();
        void apply_data_changes(const std::vector<RowRange>& ranges);

        void retain_selection();
        [[nodiscard]] auto item_state_for(RowId row) const -> ItemState;
        void refresh_item_states();
        [[nodiscard]] auto row_at(foundation::NanPoint local) const -> std::optional<RowId>;
        auto handle_key(scene::KeyEvent& key_event) -> bool;

        [[nodiscard]] auto window_math() const noexcept -> internal::UniformRowWindow;
        [[nodiscard]] auto clamped_offset(float offset) const noexcept -> float;
        void refresh_window();
        void schedule_window_sync();
        void apply_window_sync();
        void materialize_rows();
        void window_position();

        widget::BuildContext ui_;
        std::shared_ptr<IRowModel> model_;
        std::shared_ptr<IItemDelegate> delegate_;
        std::shared_ptr<ItemHost> host_;

        SelectionModel<RowId> selection_;
        reactive::Event<RowId> activated_;
        reactive::Subscription selection_sub_;
        reactive::Subscription user_selection_sub_;
        reactive::Subscription activation_sub_;

        reactive::Subscription reset_sub_;
        reactive::Subscription changed_sub_;
        bool subscribed_ = false;
        bool updating_selection_ = false;

        bool pending_reset_ = false;
        std::vector<RowRange> pending_ranges_;
        bool pending_scheduled_ = false;
        bool pending_window_sync_ = false;
        std::optional<std::size_t> pending_scroll_row_;

        internal::RowWindow visible_window_ {};
        float scroll_offset_ = 0.0F;
        float wheel_step_ = 40.0F;
        float row_height_ = 36.0F;

        std::vector<RowId> ordered_rows_;
        std::unordered_map<RowId, std::size_t> row_index_;
        std::optional<RowId> current_row_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LIST_VIEW_HPP
