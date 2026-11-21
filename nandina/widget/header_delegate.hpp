//
// widget/header_delegate — DataTable 表头视觉 delegate（experimental）。
//
// 表头语义/焦点/指针/键盘/排序事实仍由框架的 HeaderCellHost + HeaderHost 拥有；delegate
// 只创建/更新视觉根节点，不能替换 semantics wrapper，也不能自行接管排序或焦点。
// HeaderBinding 只在当次 create/update 回调期间有效，作者不得保存 column/theme 引用。
// 主题变化走 update_header（携带新的 NanTheme 快照与 revision），delegate 自行重解析 token。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_HEADER_DELEGATE_HPP
#define NANDINA_EXPERIMENT_WIDGET_HEADER_DELEGATE_HPP

#include "../scene/control.hpp"
#include "../theme/nan_style.hpp"
#include "build_context.hpp"
#include "data_table_column.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace nandina::widget
{
    /// 单个表头单元的状态：排序只描述"本列是否当前排序列 + 方向"，列身份由 column 提供。
    struct HeaderState {
        std::optional<SortDirection> sort_direction;
        bool focused = false;
        std::uint64_t theme_revision = 0;
    };

    /// 同步借用；只在 create/update 回调期间有效。
    struct HeaderBinding {
        const DataTableColumn& column;
        const theme::NanTheme& theme;
        HeaderState state;
    };

    class IHeaderDelegate {
    public:
        virtual ~IHeaderDelegate() = default;

        /// 创建视觉根节点（detached）；失败不得留下任何已挂载节点或 scope。
        virtual auto create_header(const widget::BuildContext& ui, const HeaderBinding& binding)
            -> std::shared_ptr<scene::NanControl> = 0;
        /// 更新视觉；主题 revision 变化时也必须重解析语义 token。
        virtual void update_header(scene::NanControl& control, const HeaderBinding& binding) = 0;
    };

    /// 默认表头视觉：Label + 排序三角图元；column_header semantics 由框架 wrapper 提供。
    class TextHeaderDelegate final: public IHeaderDelegate {
    public:
        [[nodiscard]] auto
        create_header(const widget::BuildContext& ui, const HeaderBinding& binding)
            -> std::shared_ptr<scene::NanControl> override;
        void update_header(scene::NanControl& control, const HeaderBinding& binding) override;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_HEADER_DELEGATE_HPP
