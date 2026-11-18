//
// widget/internal/item_delegate — DataTable/ListView 的单元格创建与更新协议（experimental）。
//
// 文件暂留在 internal/ 目录，但 3b③ 起按 experimental 公共头开放。create_item 才拿
// item-scoped BuildContext（with_scope），可用于建控件/effect/subscription；update_item
// 会被反复调用，只能更新控件，不得安装 effect/subscription。所有借用引用只在当次调用
// 期间有效。TypedItemDelegate 在每次转换前重新校验无 RTTI 令牌。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_ITEM_DELEGATE_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_ITEM_DELEGATE_HPP

#include "../../scene/control.hpp"
#include "../../widget/build_context.hpp"
#include "../../widget/label.hpp"
#include "data_identity.hpp"
#include "row_model.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::widget
{
    struct ItemState {
        bool selected = false;
        bool current = false; // 当前单元（键盘/程序化）
        bool hovered = false;
        bool enabled = true;
        std::uint64_t theme_revision = 0;
    };

    /// 同步借用；delegate 不得保存引用/指针到控件生命周期之后。
    struct ItemBinding {
        const IRowModel& model;
        std::size_t model_row = 0; // 模型行坐标
        ModelIndex display_index; // 显示序坐标（排序/过滤后）
        RowId row; // 稳定身份 = model.row_id(model_row)
        const ColumnId* column = nullptr; // DataTable 借用列 id；ListView 为 nullptr
        ItemState state;
    };

    class IItemDelegate {
    public:
        virtual ~IItemDelegate() = default;

        /// 绑定的模型类型令牌：nullptr = 通用 delegate（接受任意 IRowModel）。
        [[nodiscard]] virtual auto accepted_model_type_token() const noexcept -> const void* {
            return nullptr;
        }
        virtual auto create_item(const widget::BuildContext& ui, const ItemBinding& binding)
            -> std::shared_ptr<scene::NanControl> = 0;
        virtual void update_item(scene::NanControl& control, const ItemBinding& binding) = 0;
    };

    template<typename ModelT>
        requires std::derived_from<ModelT, IRowModel>
    class TypedItemDelegate: public IItemDelegate {
    public:
        [[nodiscard]] auto accepted_model_type_token() const noexcept -> const void* final {
            return model_type_token_of<ModelT>();
        }

        auto create_item(const widget::BuildContext& ui, const ItemBinding& binding)
            -> std::shared_ptr<scene::NanControl> final {
            return create_typed(ui, checked_model(binding.model), binding);
        }
        void update_item(scene::NanControl& control, const ItemBinding& binding) final {
            update_typed(control, checked_model(binding.model), binding);
        }

    protected:
        virtual auto create_typed(const widget::BuildContext&, const ModelT&, const ItemBinding&)
            -> std::shared_ptr<scene::NanControl> = 0;
        virtual void update_typed(scene::NanControl&, const ModelT&, const ItemBinding&) = 0;

    private:
        [[nodiscard]] static auto checked_model(const IRowModel& model) -> const ModelT& {
            if (model.model_type_token() != model_type_token_of<ModelT>()) {
                throw std::invalid_argument("Item delegate model type does not match");
            }
            return static_cast<const ModelT&>(model);
        }
    };

    /// 2B 默认单元格视觉：单行 Label，文本来自列取值器（按值返回，避免悬垂 view）。
    /// 内容为空时渲染空 Label；token 为 nullptr（通用 delegate）。
    class TextItemDelegate final: public IItemDelegate {
    public:
        using TextFunction = std::function<std::string(const IRowModel&, std::size_t)>;

        explicit TextItemDelegate(TextFunction text = {});

        [[nodiscard]] auto create_item(const widget::BuildContext& ui, const ItemBinding& binding)
            -> std::shared_ptr<scene::NanControl> override;
        void update_item(scene::NanControl& control, const ItemBinding& binding) override;

    private:
        [[nodiscard]] auto text_for(const ItemBinding& binding) const -> std::string;

        TextFunction text_;
    };

    inline TextItemDelegate::TextItemDelegate(TextFunction text): text_(std::move(text)) {}

    inline auto TextItemDelegate::text_for(const ItemBinding& binding) const -> std::string {
        return text_ ? text_(binding.model, binding.model_row) : std::string {};
    }

    inline auto
    TextItemDelegate::create_item(const widget::BuildContext& ui, const ItemBinding& binding)
        -> std::shared_ptr<scene::NanControl> {
        auto label = Label::create(ui.graph(), text_for(binding), ui.theme());
        label->set_typography_role(theme::TypographyRole::label);
        label->set_max_lines(1);
        label->set_overflow(text::TextOverflow::ellipsis);
        return label;
    }

    inline void
    TextItemDelegate::update_item(scene::NanControl& control, const ItemBinding& binding) {
        // 只由本 delegate 创建 Label；无 RTTI 构建下用静态转换（所有权在宿主侧）。
        auto& label = static_cast<Label&>(control);
        label.set_text(text_for(binding));
    }
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_ITEM_DELEGATE_HPP
