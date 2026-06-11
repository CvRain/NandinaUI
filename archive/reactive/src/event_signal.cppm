//
// Created by cvrain on 2026/4/26.
//

module;

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

export module nandina.reactive.event_signal;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// § Connection — 信号连接句柄
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 表示一个已注册的槽（slot）连接。
 *
 * 调用 disconnect() 来取消订阅。支持 move 语义，
 * 可安全地存储和传递。默认构造的是空连接（connected() == false）。
 *
 * @note 从 connect() 获取 Connection，而非手动构造。
 */
class Connection {
public:
    Connection() noexcept {}

    Connection(std::function<void()> disconnect_fn, std::function<bool()> connected_fn)
        : disconnect_fn_(std::move(disconnect_fn)), connected_fn_(std::move(connected_fn)) {}

    Connection(const Connection&) = delete;
    auto operator=(const Connection&) -> Connection& = delete;

    Connection(Connection&& other) noexcept
        : disconnect_fn_(std::move(other.disconnect_fn_))
        , connected_fn_(std::move(other.connected_fn_)) {}

    auto operator=(Connection&& other) noexcept -> Connection& {
        if (this != &other) {
            disconnect_fn_ = std::move(other.disconnect_fn_);
            connected_fn_  = std::move(other.connected_fn_);
        }
        return *this;
    }

    ~Connection() {}

    /// 主动断开连接。调用后 connected() 返回 false。
    auto disconnect() -> void {
        if (disconnect_fn_) {
            disconnect_fn_();
            disconnect_fn_ = {};
            connected_fn_  = {};
        }
    }

    /// 连接是否仍然有效
    [[nodiscard]] auto connected() const -> bool {
        return connected_fn_ && connected_fn_();
    }

private:
    std::function<void()> disconnect_fn_;
    std::function<bool()> connected_fn_;
};

// ═════════════════════════════════════════════════════════════════════════════
// § ScopedConnection — RAII 自动断开
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Conection 的 RAII 封装。
 *
 * 析构时自动断开连接。适合槽的生命周期与宿主对象绑定的场景。
 *
 * @code
 * ScopedConnection sc{ signal.connect([&]{ … }) };
 * // sc 析构时自动断开
 * @endcode
 */
class ScopedConnection {
public:
    ScopedConnection() noexcept {}
    explicit ScopedConnection(Connection conn) : conn_(std::move(conn)) {}

    ScopedConnection(const ScopedConnection&) = delete;
    auto operator=(const ScopedConnection&) -> ScopedConnection& = delete;

    ScopedConnection(ScopedConnection&& other) noexcept : conn_(std::move(other.conn_)) {}

    auto operator=(ScopedConnection&& other) noexcept -> ScopedConnection& {
        if (this != &other) { conn_ = std::move(other.conn_); }
        return *this;
    }

    ~ScopedConnection() { conn_.disconnect(); }

    auto disconnect() -> void { conn_.disconnect(); }
    [[nodiscard]] auto connected() const -> bool { return conn_.connected(); }

    /// 释放所有权，调用者负责管理连接生命周期
    auto release() -> Connection { return std::move(conn_); }

private:
    Connection conn_;
};

// ═════════════════════════════════════════════════════════════════════════════
// § EventSignal<Args…> — 多播事件信号
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 线程安全的多播事件信号。
 *
 * 线程安全保证：
 *   - connect / connect_once / disconnect / clear — 互斥锁保护
 *   - emit — 在锁内拍快照，然后无锁遍历调用槽
 *   - 在槽回调内调用 disconnect() 是安全的（无重入锁）
 *
 * 异常安全：
 *   - emit 收集第一个抛出的异常，完成所有槽调用后重新抛出
 *
 * @code
 * EventSignal<int> sig;
 * auto conn = sig.connect([](int v) { … });
 * sig.emit(42);
 * conn.disconnect();
 *
 * // 一次性槽：触发一次后自动断开
 * auto once = sig.connect_once([](int v) { … });
 * @endcode
 */
template<typename... Args>
class EventSignal {
public:
    using Slot = std::function<void(Args...)>;

    /// 注册一个槽，返回 Connection 句柄
    auto connect(Slot slot) -> Connection {
        std::lock_guard lock(mutex_);
        const std::size_t id = next_id_++;
        slots_.push_back(SlotEntry{id, true, false, std::move(slot)});
        return make_connection(id);
    }

    /// 注册一个一次性槽，触发一次后自动断开
    auto connect_once(Slot slot) -> Connection {
        std::lock_guard lock(mutex_);
        const std::size_t id = next_id_++;
        slots_.push_back(SlotEntry{id, true, true, std::move(slot)});
        return make_connection(id);
    }

    /// 触发信号，按注册顺序调用所有活跃槽
    auto emit(Args... args) -> void {
        std::vector<SlotEntry> snapshot;
        {
            std::lock_guard lock(mutex_);
            snapshot = slots_;
        }

        std::vector<std::size_t> once_ids;
        std::exception_ptr first_exception;
        for (auto& entry : snapshot) {
            if (!entry.active) {
                continue;
            }

            if (entry.once) {
                once_ids.push_back(entry.id);
            }

            try {
                entry.slot(args...);
            } catch (...) {
                if (!first_exception) {
                    first_exception = std::current_exception();
                }
            }
        }

        {
            std::lock_guard lock(mutex_);
            for (auto id : once_ids) { disconnect_locked(id); }
            cleanup_locked();
        }

        if (first_exception) {
            std::rethrow_exception(first_exception);
        }
    }

    /// 清除所有槽
    auto clear() -> void {
        std::lock_guard lock(mutex_);
        slots_.clear();
    }

private:
    struct SlotEntry {
        std::size_t id     = 0;
        bool        active = false;
        bool        once   = false;
        Slot        slot;
    };

    auto make_connection(std::size_t id) -> Connection {
        return Connection(
            [this, id]{ disconnect(id); },
            [this, id]{ return is_connected(id); }
        );
    }

    auto disconnect(std::size_t id) -> void {
        std::lock_guard lock(mutex_);
        disconnect_locked(id);
    }

    auto disconnect_locked(std::size_t id) -> void {
        for (auto& e : slots_) {
            if (e.id == id) { e.active = false; break; }
        }
    }

    [[nodiscard]] auto is_connected(std::size_t id) const -> bool {
        std::lock_guard lock(mutex_);
        for (const auto& e : slots_) {
            if (e.id == id) { return e.active; }
        }
        return false;
    }

    auto cleanup_locked() -> void {
        std::erase_if(slots_, [](const SlotEntry& e) { return !e.active; });
    }

    mutable std::mutex     mutex_;
    std::size_t            next_id_ = 1;
    std::vector<SlotEntry> slots_;
};

} // namespace nandina::reactive