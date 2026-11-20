//
// widget/tree_view — 1D virtualized tree view over ITreeModel + IItemDelegate.
//
// experimental public header (batch 6). View-owned expansion: the model only describes
// parent/child structure; TreeView keeps the expanded RowId set and flattens the visible
// subtree into a uniform-row list. Delegate content is reused as-is; the framework owns the
// indentation gutter and the disclosure triangle. Selection uses SelectionModel<RowId>.
//
// First version: uniform row height, own vertical viewport, expand/collapse + selection +
// indentation, no lazy child loading, no variable-height rows and no tree semantics roles.
//
// Rules and boundaries: docs/references/data_components_and_virtualization.md §10.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TREE_VIEW_HPP
#define NANDINA_EXPERIMENT_WIDGET_TREE_VIEW_HPP

#include "../scene/control.hpp"
#include "../scene/input_event.hpp"
#include "build_context.hpp"
#include "internal/data_identity.hpp"
#include "internal/item_delegate.hpp"
#include "internal/tree_model.hpp"
#include "internal/uniform_row_window.hpp"
#include "selection_model.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nandina::widget
{
    class ItemHost;

    /// 树形数据视图：只物化可见（已展开）行，行高统一，自身为纵向滚动视口。
    ///
    /// 必须以 shared_ptr 持有（create() 构造）：布局期延迟物化用 shared_from_this()
    /// 挂 post_layout 动作。行控件由 IItemDelegate 创建；框架在行左侧绘制缩进与展开器，
    /// 展开器点击优先于行选择。展开状态由视图持有（不是模型事实）。
    class TreeView final: public scene::NanControl {
    public:
        explicit TreeView(
            const widget::BuildContext& ui,
            SelectionMode selection_mode = SelectionMode::multiple,
            float row_height = 36.0F
        );
        ~TreeView() override;

        [[nodiscard]] static auto create(
            const widget::BuildContext& ui,
            SelectionMode selection_mode = SelectionMode::multiple,
            float row_height = 36.0F
        ) -> std::shared_ptr<TreeView>;

        // ─── 模型 / delegate ────────────────────────────────────────────
        void set_model(std::shared_ptr<ITreeModel> model);
        [[nodiscard]] auto model() const noexcept -> ITreeModel*;
        void set_delegate(std::shared_ptr<IItemDelegate> delegate);
        [[nodiscard]] auto delegate() const noexcept -> const std::shared_ptr<IItemDelegate>&;

        // ─── 行高 / 缩进 ────────────────────────────────────────────────
        void set_row_height(float row_height);
        [[nodiscard]] auto row_height() const noexcept -> float;
        void set_indent_width(float width);
        [[nodiscard]] auto indent_width() const noexcept -> float;
        void set_expander_gutter(float width);
        [[nodiscard]] auto expander_gutter() const noexcept -> float;

        // ─── 行查询 ─────────────────────────────────────────────────────
        /// 可见（已展开）行数；折叠子树不计入。
        [[nodiscard]] auto row_count() const noexcept -> std::size_t;
        /// 模型全部节点数（含折叠子树）。
        [[nodiscard]] auto node_count() const noexcept -> std::size_t;
        [[nodiscard]] auto item_count() const noexcept -> std::size_t;
        [[nodiscard]] auto row_node(RowId row) const -> scene::NanControl*;
        [[nodiscard]] auto display_row_for(RowId row) const -> std::optional<std::size_t>;
        [[nodiscard]] auto row_id_for_display(std::size_t display_row) const
            -> std::optional<RowId>;
        [[nodiscard]] auto depth_for_display(std::size_t display_row) const -> std::size_t;

        // ─── 展开（视图持有）──────────────────────────────────────────
        [[nodiscard]] auto is_expanded(RowId row) const -> bool;
        /// 展开有子节点的已存在行；叶子/未知身份返回 false。
        auto expand(RowId row) -> bool;
        auto collapse(RowId row) -> bool;
        auto toggle_expanded(RowId row) -> bool;
        void expand_all();
        void collapse_all();
        /// 无载荷：订阅方重新读取 is_expanded。
        [[nodiscard]] auto expansion_changed() const noexcept -> const reactive::Event<>&;
        void set_on_expansion_changed(std::function<void()> callback);

        // ─── 选择 ──────────────────────────────────────────────────────
        [[nodiscard]] auto selection() const noexcept -> const SelectionModel<RowId>&;
        /// 程序化选择：未知身份返回 false；会展开祖先使目标可见，随后滚动到该行。
        auto select(RowId row) -> bool;
        void clear_selection();
        [[nodiscard]] auto selection_changed() const noexcept -> const reactive::Event<>&;
        void set_on_selection_changed(std::function<void()> callback);

        // ─── 纵向视口 ──────────────────────────────────────────────────
        [[nodiscard]] auto scroll_offset() const noexcept -> float;
        [[nodiscard]] auto maximum_scroll_offset() const noexcept -> float;
        void set_scroll_offset(float offset);
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
        auto on_draw(render::DrawContext& ctx) -> void override;
        void on_enter_tree() override;
        void on_ready() override;
        void on_exit_tree() override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        struct VisibleRow {
            RowId row;
            std::size_t model_row = 0;
            std::size_t depth = 0;
        };

        void subscribe_model();
        void unsubscribe_model();
        [[nodiscard]] auto traversal_phase() const -> bool;
        void on_model_reset();
        void on_model_data_changed(RowRange range);
        void schedule_pending();
        void apply_pending();
        void rebuild_all();
        void apply_data_changes(const std::vector<RowRange>& ranges);

        void rebuild_node_index();
        void rebuild_visible_rows();
        [[nodiscard]] auto retain_selection_and_expansion() -> bool;
        [[nodiscard]] auto contains_node(RowId row) const -> bool;
        [[nodiscard]] auto row_has_children(RowId row) const -> bool;

        auto set_expanded(RowId row, bool expanded) -> bool;
        void schedule_structure_sync();
        void apply_structure_sync();

        [[nodiscard]] auto item_state_for(RowId row, std::size_t depth) const -> ItemState;
        void refresh_item_states();
        [[nodiscard]] auto row_at(foundation::NanPoint local) const -> std::optional<std::size_t>;
        [[nodiscard]] auto expander_at(foundation::NanPoint local, std::size_t display_row) const
            -> bool;
        auto handle_key(scene::KeyEvent& key_event) -> bool;
        void move_current_to(std::size_t display_row, bool select_follows);
        [[nodiscard]] auto visible_order() const -> std::vector<RowId>;

        [[nodiscard]] auto window_math() const noexcept -> internal::UniformRowWindow;
        [[nodiscard]] auto clamped_offset(float offset) const noexcept -> float;
        void refresh_window();
        void schedule_window_sync();
        void apply_window_sync();
        void materialize_rows();
        void window_position();
        [[nodiscard]] auto content_x(std::size_t depth) const noexcept -> float;
        [[nodiscard]] auto content_width(std::size_t depth) const noexcept -> float;

        widget::BuildContext ui_;
        std::shared_ptr<ITreeModel> model_;
        std::shared_ptr<IItemDelegate> delegate_;
        std::shared_ptr<ItemHost> host_;

        SelectionModel<RowId> selection_;
        reactive::Event<RowId> activated_;
        reactive::Event<> expansion_changed_;
        reactive::Subscription selection_sub_;
        reactive::Subscription user_selection_sub_;
        reactive::Subscription activation_sub_;
        reactive::Subscription user_expansion_sub_;

        reactive::Subscription reset_sub_;
        reactive::Subscription changed_sub_;
        bool subscribed_ = false;
        bool updating_selection_ = false;

        bool pending_reset_ = false;
        std::vector<RowRange> pending_ranges_;
        bool pending_scheduled_ = false;
        bool pending_window_sync_ = false;
        bool pending_structure_sync_ = false;
        std::optional<std::size_t> pending_scroll_row_;
        std::optional<RowId> pending_select_row_;

        std::unordered_map<RowId, std::size_t> node_rows_;
        std::unordered_set<RowId> expanded_;
        std::vector<VisibleRow> visible_rows_;
        std::unordered_map<RowId, std::size_t> visible_index_;
        std::optional<RowId> current_row_;

        internal::RowWindow visible_window_ {};
        float scroll_offset_ = 0.0F;
        float wheel_step_ = 40.0F;
        float row_height_ = 36.0F;
        float indent_width_ = 18.0F;
        float expander_gutter_ = 18.0F;
        theme::NanColor expander_color_ {};
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TREE_VIEW_HPP
