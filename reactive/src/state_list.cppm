//
// Created by cvrain on 2026/4/26.
//

module;

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

export module nandina.reactive.state_list;

export import nandina.reactive.event_signal;
export import nandina.reactive.tracking;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// § ListChangeKind — 列表变更类型
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 标识 StateList 发生了哪种变化。
 */
enum class ListChangeKind {
    Insert,
    Remove,
    Replace,
    Move,
    Reset,
};

// ═════════════════════════════════════════════════════════════════════════════
// § ListChange<T> — 一次列表变更的描述
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 描述 StateList 上一次具体的变化。
 *
 * 不同 kind 对应的字段含义：
 *   - Insert:  index=目标位置, item=插入元素
 *   - Remove:  index=被删位置,  item=被删元素
 *   - Replace: index=替换位置,  item=新值,  old_item=旧值（optional）
 *   - Move:    index=目标位置,  item=被移元素（old_index 记录原位置）
 *   - Reset:   无具体字段（表示整体替换）
 */
template<typename T>
struct ListChange {
    ListChangeKind kind;
    std::size_t    index      = 0;
    T              item       = {};
    std::size_t    old_index  = 0;   // 仅 Move
    T              old_item   = {};  // 仅 Replace
};

// ═════════════════════════════════════════════════════════════════════════════
// § StateList<T> — 响应式列表
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 响应式列表容器。
 *
 * 包装 std::vector<T>，支持批量变更并通知 observer。
 * 常用于管理动态子组件的列表。
 *
 * @note StateList 自身不可复制不可移动。
 *       其内部 observer 机制与 Effect 依赖追踪系统兼容。
 *
 * @code
 * StateList<int> list;
 * list.push_back(1);
 * list.push_back(2);
 *
 * Effect e([&]{
 *     for (auto x : list.get()) { fmt::print("{} ", x); }
 * });
 *
 * list.push_back(3);  // Effect 自动重跑
 * @endcode
 */
template<typename T>
class StateList {
public:
    StateList() = default;
    explicit StateList(std::vector<T> initial) : items_(std::move(initial)) {}

    StateList(const StateList&)          = delete;
    auto operator=(const StateList&)     = delete;
    StateList(StateList&&)               = delete;
    auto operator=(StateList&&)          = delete;

    // ── 读取 ──

    [[nodiscard]] auto operator()() const -> const std::vector<T>& {
        track_access();
        return items_;
    }

    [[nodiscard]] auto get() const -> const std::vector<T>& { return (*this)(); }
    [[nodiscard]] auto size() const -> std::size_t { return items_.size(); }
    [[nodiscard]] auto at(std::size_t idx) const -> const T& { return items_.at(idx); }
    [[nodiscard]] auto operator[](std::size_t idx) const -> const T& { return items_[idx]; }
    [[nodiscard]] auto empty() const -> bool { return items_.empty(); }

    // ── 修改 ──

    auto push_back(T value) -> void {
        items_.push_back(value);
        notify(ListChange<T>{ListChangeKind::Insert, items_.size() - 1, std::move(value)});
    }

    auto pop_back() -> void {
        if (items_.empty()) { return; }
        auto item = std::move(items_.back());
        items_.pop_back();
        notify(ListChange<T>{ListChangeKind::Remove, items_.size(), std::move(item)});
    }

    auto insert(std::size_t index, T value) -> void {
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), value);
        notify(ListChange<T>{ListChangeKind::Insert, index, std::move(value)});
    }

    auto erase(std::size_t index) -> void {
        auto item = std::move(items_[index]);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
        notify(ListChange<T>{ListChangeKind::Remove, index, std::move(item)});
    }

    auto replace(std::size_t index, T new_value) -> void {
        auto old_item = std::move(items_[index]);
        items_[index] = new_value;
        notify(ListChange<T>{ListChangeKind::Replace, index, new_value, 0, std::move(old_item)});
    }

    auto move_item(std::size_t from, std::size_t to) -> void {
        if (from == to || from >= items_.size() || to >= items_.size()) { return; }
        auto item = std::move(items_[from]);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(from));
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(to), std::move(item));
        notify(ListChange<T>{ListChangeKind::Move, to, items_[to], from});
    }

    auto reset(std::vector<T> new_items) -> void {
        items_ = std::move(new_items);
        notify(ListChange<T>{ListChangeKind::Reset});
    }

    auto clear() -> void {
        items_.clear();
        notify(ListChange<T>{ListChangeKind::Reset});
    }

    /// 注册变化回调，返回 Connection（可断开）
    auto on_change(std::function<void(const std::vector<T>&)> callback) -> Connection {
        change_callback_ = std::move(callback);
        return Connection{
            [this] { change_callback_ = {}; },
            [this] { return static_cast<bool>(change_callback_); }
        };
    }

private:
    auto track_access() const -> void {
        if (!detail::current_tracking_context || !detail::current_tracking_context->invalidate) {
            return;
        }
        for (const auto& obs : observers_) {
            if (obs.active && obs.id == detail::current_tracking_context->id) { return; }
        }
        observers_.push_back({
            detail::current_tracking_context->id,
            true,
            *detail::current_tracking_context->invalidate
        });
    }

    auto notify(const ListChange<T>& /*change*/) -> void {
        auto snapshot = std::move(observers_);
        observers_    = {};
        std::size_t next_observer = 0;
        detail::PendingObserverRestore restore_guard{observers_, snapshot, next_observer};

        for (; next_observer < snapshot.size(); ++next_observer) {
            const auto& entry = snapshot[next_observer];
            if (!entry.active || !entry.invalidate) { continue; }

            if (detail::is_batching()) {
                detail::queue_invalidation(entry.id, entry.invalidate);
            } else {
                entry.invalidate();
            }
        }

        restore_guard.commit();

        if (change_callback_) {
            change_callback_(items_);
        }
    }

    std::vector<T>                                items_;
    mutable std::vector<detail::ObserverEntry>    observers_;
    std::function<void(const std::vector<T>&)>    change_callback_;
};

} // namespace nandina::reactive