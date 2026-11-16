//
// widget/internal/row_model — 非模板行模型契约（批 2A 内部协议）。
//
// IRowModel 只描述「行」：行数、稳定行身份、类型令牌与两个同步变更事件；列/单元格视觉
// 由 DataTableColumn + delegate 负责（批 2B）。RowModelBase 提供事件存储与通知工具，
// 派生模型通过 TypedRowModelBase<Derived> 获得无 RTTI 的类型令牌。
//
// 2A 边界：只验证同步触发、空范围不发事件、非法范围抛 invalid_argument、data_changed
// 不改变行数/顺序/RowId。遍历期 pending 合并依赖场景树相位与视图状态，移到 2B/3。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_ROW_MODEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_ROW_MODEL_HPP

#include "../../reactive/event.hpp"
#include "data_identity.hpp"

#include <cstddef>
#include <stdexcept>

namespace nandina::widget
{
    /// 半开行区间 [first, last_exclusive)。空范围 first == last_exclusive。
    struct RowRange {
        std::size_t first = 0;
        std::size_t last_exclusive = 0;

        friend constexpr auto operator==(const RowRange&, const RowRange&) noexcept
            -> bool = default;
    };

    /// 类型令牌：无 RTTI 构建下也可用的静态地址令牌（与 framework 既有 type-token 模式一致）。
    /// 必须先于使用它的类声明。
    template<typename T>
    struct ModelTypeToken {
        static inline const int value = 0;
    };

    template<typename T>
    [[nodiscard]] auto model_type_token_of() noexcept -> const void* {
        return &ModelTypeToken<T>::value;
    }

    /// 行模型只读接口；视图/typed delegate 通过类型令牌做配对校验。
    class IRowModel {
    public:
        virtual ~IRowModel() = default;

        [[nodiscard]] virtual auto row_count() const noexcept -> std::size_t = 0;
        [[nodiscard]] virtual auto row_id(std::size_t row) const -> RowId = 0;

        /// 视图/typed delegate 配对校验用；直接实现 IRowModel 时作者自己返回令牌。
        [[nodiscard]] virtual auto model_type_token() const noexcept -> const void* = 0;

        /// 变更事件：视图订阅（Event::subscribe 返回 move-only Subscription）。
        [[nodiscard]] virtual auto reset_event() const noexcept -> const reactive::Event<>& = 0;
        [[nodiscard]] virtual auto data_changed_event() const noexcept
            -> const reactive::Event<RowRange>& = 0;
    };

    /// 提供事件存储与 notify_* 工具的基类。
    ///
    /// 禁止拷贝与移动：reactive::Event 的拷贝会共享订阅状态，两个模型副本会互相触发
    /// 事件；模型统一通过 shared_ptr<IRowModel> 持有，不需要拷贝/移动身份。
    class RowModelBase: public IRowModel {
    public:
        RowModelBase() = default;
        ~RowModelBase() override = default;

        RowModelBase(const RowModelBase&) = delete;
        auto operator=(const RowModelBase&) -> RowModelBase& = delete;
        RowModelBase(RowModelBase&&) = delete;
        auto operator=(RowModelBase&&) -> RowModelBase& = delete;

        [[nodiscard]] auto reset_event() const noexcept -> const reactive::Event<>& override {
            return reset_event_;
        }
        [[nodiscard]] auto data_changed_event() const noexcept
            -> const reactive::Event<RowRange>& override {
            return data_changed_event_;
        }

    protected:
        /// 行集合/行数/顺序变化：同步发出 reset。视图必须按 reset 语义完整对账。
        void notify_reset() {
            reset_event_.emit();
        }

        /// 行内容变化（不改变行数/顺序/RowId）：同步发出 data_changed。
        /// - `first <= last_exclusive <= row_count()`，否则抛 invalid_argument 且不发事件；
        /// - 空范围（first == last_exclusive）是合法的 no-op，不发事件。
        void notify_data_changed(const RowRange range) {
            if (range.first > range.last_exclusive || range.last_exclusive > row_count()) {
                throw std::invalid_argument("RowModelBase data_changed range is invalid");
            }
            if (range.first == range.last_exclusive) {
                return;
            }
            data_changed_event_.emit(range);
        }

    private:
        reactive::Event<> reset_event_;
        reactive::Event<RowRange> data_changed_event_;
    };

    /// 非模板 RowModelBase 无法自动知道最终派生类型；作者通常继承这个：
    ///
    ///     class MyModel final: public TypedRowModelBase<MyModel> { ... };
    template<typename Derived>
    class TypedRowModelBase: public RowModelBase {
    public:
        [[nodiscard]] auto model_type_token() const noexcept -> const void* override {
            return model_type_token_of<Derived>();
        }
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_ROW_MODEL_HPP
