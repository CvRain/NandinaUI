//
// widget/pagination - page navigation with prev/next, numbered pages and ellipsis.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PAGINATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_PAGINATION_HPP

#include "../reactive/event.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"
#include "roving_focus.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace nandina::widget
{
    /**
     * 分页器：上一页 / 下一页 + 带省略号的页码序列。
     *
     * 页码按钮是**绘制**出来的槽位（不是子控件），因此整条分页器自己持有焦点：Tab 把
     * 焦点交给分页器，方向键在可见槽位之间漫游，Enter / Space 激活当前槽位。漫游完全
     * 复用 `RovingFocus`（`focus_only` 模型 + `handle_key` / `step`），容器只负责把
     * `Intent::index` 映射成"哪个槽位带视觉焦点"，不自己实现环绕 / Home / End。
     * 省略号与到达边界时的上一页 / 下一页槽位不可聚焦，`RovingFocus` 会跳过它们。
     *
     * `current_page` 是 1-based。`page_count` 小于 1 时（或尚未设置）分页器视作"没有
     * 可翻页的内容"：测量为 0、不绘制、不可聚焦，便于条件挂载。
     */
    class Pagination: public scene::NanControl {
    public:
        explicit Pagination(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Pagination>;

        /// 总页数；负值按 0 处理。设为 &lt;1 时分页器整体塌缩为 0 尺寸。
        void set_page_count(int count);
        [[nodiscard]] auto page_count() const -> int;

        /// 静默设置当前页（1-based，自动钳制到 `[1, page_count]`；无页时为 0），不触发事件。
        void set_current_page(int page);
        [[nodiscard]] auto current_page() const -> int;

        /// 当前页左右各显示多少个页码（钳制到 &ge;0，默认 1）。
        void set_sibling_count(int siblings);
        [[nodiscard]] auto sibling_count() const -> int;

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;

        /// 用户激活路径（点击 / Enter / Space）：切换当前页并触发回调与事件。
        void go_to_page(int page);

        void set_on_page_change(std::function<void(int)> callback);
        [[nodiscard]] auto page_changed() const -> const reactive::Event<int>&;

        /// 可见槽位数（含上一页 / 省略号 / 下一页），供测试与可访问性读取。
        [[nodiscard]] auto visible_item_count() const -> std::size_t;
        /// 复用中的 `RovingFocus` 成员数，应与 `visible_item_count()` 一致（测试用）。
        [[nodiscard]] auto roving_member_count() const -> std::size_t;
        /// 当前持有漫游焦点的槽位下标（-1 = 无）。
        [[nodiscard]] auto focused_item_index() const -> int;
        /// 可见槽位是否为页码（`ellipsis` / prev / next 时为 false，测试用）。
        [[nodiscard]] auto item_is_page(std::size_t index) const -> bool;
        /// 可见槽位的页码（非页码槽位返回 0，测试用）。
        [[nodiscard]] auto item_page(std::size_t index) const -> int;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::PaginationRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::PaginationVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedPaginationStyle;

        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        void on_ready() override;
        void on_process(float dt) override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        enum class ItemKind {
            previous,
            page,
            ellipsis,
            next,
        };

        struct Item {
            ItemKind kind = ItemKind::page;
            int page = 0;
            bool enabled = true;
            foundation::NanRect rect;
            std::shared_ptr<primitives::Text> text;
        };

        /// 页码窗口：first / last / 当前页 ± siblings，中间用省略号补位。
        [[nodiscard]] auto visible_pages() const -> std::vector<int>;
        [[nodiscard]] auto clamp_page(int page) const -> int;
        /// 若模型失效则重建可见槽位、文本与 `RovingFocus` 成员。
        void ensure_model();
        void rebuild_model();
        /// 把成员表同步给共享漫游设施（成员 = 可见槽位，省略号 / 边界导航项不可聚焦）。
        void sync_roving();
        /// 按 `metrics.box_size` / `gap` 计算每个槽位的局部矩形。
        void layout_items();
        [[nodiscard]] auto slot_of_current_page() const -> int;
        [[nodiscard]] auto hit_item(float local_x, float local_y) const -> int;
        void activate_item(int index);
        void apply_text_styles();
        void relayout();

        std::vector<Item> items_;
        /// 组内键盘漫游（focus_only：方向键只移动焦点槽位，Enter / Space 才翻页）。
        RovingFocus focus_;
        int focus_index_ = -1;
        int hover_index_ = -1;
        int page_count_ = 0;
        int current_page_ = 0;
        int sibling_count_ = 1;
        bool disabled_ = false;
        bool focused_ = false;
        /// 模型（可见槽位 / 文本 / 漫游成员）是否需要重建。
        bool model_dirty_ = true;
        std::function<void(int)> on_page_change_;
        reactive::Event<int> page_changed_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::PaginationRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_PAGINATION_HPP
