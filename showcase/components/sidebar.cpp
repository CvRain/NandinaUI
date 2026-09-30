#include "sidebar.hpp"

#include <nandina/widget/controls.hpp> // Button / Label / Divider + 它们的 ComponentTraits
#include <nandina/widget/layout.hpp>   // Row / Column / Padding / Expanded

#include <utility>

namespace nandina::showcase
{
    namespace
    {
        constexpr float kSidebarPadding = 12.0F;
        constexpr float kSectionGap = 10.0F;
        constexpr float kItemGap = 4.0F;
    } // namespace

    SidebarShell::SidebarShell(app::ShellContext& context):
        context_(&context) {
        project_routes();
    }

    auto SidebarShell::set_width(const float width) -> SidebarShell& {
        width_ = width;
        return *this;
    }

    auto SidebarShell::set_title(std::string title) -> SidebarShell& {
        title_ = std::move(title);
        return *this;
    }

    auto SidebarShell::set_active_treatment(const theme::ButtonTreatment treatment) -> SidebarShell& {
        active_treatment_ = treatment;
        return *this;
    }

    auto SidebarShell::set_idle_treatment(const theme::ButtonTreatment treatment) -> SidebarShell& {
        idle_treatment_ = treatment;
        return *this;
    }

    auto SidebarShell::on_activate(ActivateHandler handler) -> SidebarShell& {
        on_activate_ = std::move(handler);
        return *this;
    }

    auto SidebarShell::items() const -> const std::vector<SidebarItem>& {
        return items_;
    }

    void SidebarShell::project_routes() {
        items_.clear();
        for (const auto& entry: context_->routes().entries()) {
            // 数据侧的两个准入条件，都不需要组件自己维护名单：
            //   * show_in_nav 是路由表的显式声明；
            //   * activate == nullptr 表示这一页需要构造参数、没法凭类型键进入 ——
            //     列出来只会得到一个点了没反应的死条目。
            if (!entry.options.show_in_nav || entry.activate == nullptr) {
                continue;
            }
            std::string label = entry.options.title.empty() ? entry.options.key
                                                            : entry.options.title;
            if (label.empty()) {
                // 既没有 title 也没有 key：没有任何可显示的文字，不占一行。
                continue;
            }
            items_.push_back(
                SidebarItem {
                    .page_key = entry.page_key,
                    .label = std::move(label),
                    .icon = entry.options.icon,
                    .activate = entry.activate,
                }
            );
        }
    }

    auto SidebarShell::make_item(const SidebarItem& item) -> std::shared_ptr<widget::Button> {
        auto ui = context_->ui();
        auto builder = ui.make<widget::Button>(item.label);
        builder.configure([this](widget::Button& node) {
            // 初始 treatment 只是"第一帧别闪"；之后由下面按 Router 状态绑定。
            node.set_treatment(idle_treatment_);
        });
        auto handle = builder.build();

        if (on_activate_) {
            // 组件只报告"哪一项被激活"，跳不跳、怎么跳由应用决定。
            const auto navigation = context_->navigation();
            const auto activated = item;
            handle->set_on_click([navigation, activated, handler = on_activate_] {
                handler(navigation, activated);
            });
        }
        return handle;
    }

    auto SidebarShell::build_shell() -> widget::View {
        auto ui = context_->ui();
        auto& current = context_->current_page();

        // ── 条目列表：每一项的高亮都派生自 Router 的当前路由 ──────────────────
        auto list = widget::Column::create();
        list->set_gap(kItemGap);
        // stretch 让条目横向铺满侧边栏（默认 start 会按内容收缩）。
        list->set_cross_alignment(widget::LayoutAlignment::stretch);

        for (const auto& item: items_) {
            auto handle = make_item(item);
            if (handle == nullptr) {
                continue;
            }

            // 高亮不靠应用同步：绑定到 Router 发布的当前路由，首屏 start() 与任何
            // 程序化导航都会自动跟随。
            auto& is_current = ui.computed([&current, key = item.page_key] {
                return current.get() == key;
            });
            ui.bind(
                handle,
                [this](widget::Button& node, const bool active) {
                    node.set_treatment(active ? active_treatment_ : idle_treatment_);
                },
                is_current
            );
            list->add(handle);
        }

        // ── 侧边栏面板 ──────────────────────────────────────────────────────
        auto panel = widget::Column::create();
        panel->set_gap(kSectionGap);
        panel->set_cross_alignment(widget::LayoutAlignment::stretch);
        if (!title_.empty()) {
            panel->add(
                ui.make<widget::Label>(title_)
                    .font_size(14.0F)
                    .color_token(theme::ColorToken::foreground)
                    .build()
            );
            panel->add(ui.make<widget::Divider>().build());
        }
        panel->add(list);

        auto padded = widget::Padding::create(foundation::NanInsets::all(kSidebarPadding));
        padded->set_child(panel);
        padded->set_width(width_);
        padded->set_height(widget::authoring::fill);

        // ── 外壳：侧边栏 + 内容区 ────────────────────────────────────────────
        auto content = widget::Expanded::create();
        content->set_child(context_->outlet());

        auto shell = widget::Row::create();
        shell->set_gap(0.0F);
        shell->set_cross_alignment(widget::LayoutAlignment::stretch);
        shell->set_width(widget::authoring::fill);
        shell->set_height(widget::authoring::fill);
        shell->add(padded);
        shell->add(content);
        return shell;
    }
} // namespace nandina::showcase
