module;

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

export module nandina.runtime.nan_event;

export import nandina.foundation.nan_types;       // nandina::types::PointerButton 等

using nandina::types::PointerButton;
// ============================================================
// 统一 Event 类型体系（Issue 011）
//
// 职责：
//   定义框架内所有 UI 事件的数据结构与类型标签，供
//   Widget 事件分发与 Window 事件翻译共用。
//
// EventType 枚举 + 独立 EventData 结构体（而非变体）：
//   - 与现有 NanWindow 回调签名完全兼容
//   - Widget 的 dispatchEvent() 通过 EventType 做静态分发
//   - 避免 std::variant 的 visit 开销与模板膨胀
//
// 依赖规则：
//   - 本模块仅依赖 foundation::nan_types（PointerButton）
//   - runtime/nan_widget、runtime/nan_window 均可导入本模块
// ============================================================
export namespace nandina::runtime {
    // ── 事件类型标签 ──────────────────────────────────────────
    enum class EventType : std::uint8_t {
        PointerMove,
        PointerDown,
        PointerUp,
        PointerClick,
        PointerWheel,
        KeyDown,
        KeyUp,
        TextInput,
        FocusIn,
        FocusOut,
        WindowResize,
        WindowClose,
    };

    // ── 事件名称辅助（调试/日志）────────────────────────────
    [[nodiscard]] inline auto to_string(EventType type) noexcept -> std::string_view {
        switch (type) {
        case EventType::PointerMove:   return "PointerMove";
        case EventType::PointerDown:   return "PointerDown";
        case EventType::PointerUp:     return "PointerUp";
        case EventType::PointerClick:  return "PointerClick";
        case EventType::PointerWheel:  return "PointerWheel";
        case EventType::KeyDown:       return "KeyDown";
        case EventType::KeyUp:         return "KeyUp";
        case EventType::TextInput:     return "TextInput";
        case EventType::FocusIn:       return "FocusIn";
        case EventType::FocusOut:      return "FocusOut";
        case EventType::WindowResize:  return "WindowResize";
        case EventType::WindowClose:   return "WindowClose";
        }
        return "Unknown";
    }

    // ── 事件数据结构体 ────────────────────────────────────────

    /// 鼠标/指针移动事件
    struct PointerMoveEvent {
        double x{0.0};
        double y{0.0};
        double delta_x{0.0};
        double delta_y{0.0};
    };

    /// 鼠标/指针按钮事件（按下/抬起/点击通用）
    struct PointerButtonEvent {
        PointerButton button{PointerButton::Unknown};
        double x{0.0};
        double y{0.0};
        bool is_repeat{false};
    };

    /// 鼠标滚轮事件
    struct PointerWheelEvent {
        double x{0.0};
        double y{0.0};
    };

    /// 键盘按键事件
    struct KeyEvent {
        std::int32_t key_code{0};
        bool is_repeat{false};
    };

    /// 文本输入事件（IME 最终结果）
    struct TextInputEvent {
        std::string text;
    };

    /// 焦点变化事件
    struct FocusEvent {
        bool got_focus{false};
    };

    /// 窗口尺寸变化事件
    struct WindowResizeEvent {
        int width{0};
        int height{0};
    };

    /// 窗口关闭事件（仅信号，无数据）
    struct WindowCloseEvent {};

    // ── 统一事件变体 ──────────────────────────────────────────
    // 用于需要将事件作为同一类型传递的场景（如事件队列）。
    using Event = std::variant<
        PointerMoveEvent,
        PointerButtonEvent,
        PointerWheelEvent,
        KeyEvent,
        TextInputEvent,
        FocusEvent,
        WindowResizeEvent,
        WindowCloseEvent
    >;

    /// 从 Event 变体中提取类型标签
    ///
    /// 注意：不能用 static_cast<EventType>(ev.index())，
    /// 因为 EventType 枚举值存在跳号（如 PointerClick=2, FocusOut=9），
    /// 与 variant 的线性索引不对齐。
    [[nodiscard]] inline auto event_type(const Event &ev) noexcept -> EventType {
        switch (ev.index()) {
        case 0:  return EventType::PointerMove;
        case 1:  return EventType::PointerDown;    // PointerButtonEvent
        case 2:  return EventType::PointerWheel;
        case 3:  return EventType::KeyDown;
        case 4:  return EventType::TextInput;
        case 5:  return EventType::FocusIn;
        case 6:  return EventType::WindowResize;
        case 7:  return EventType::WindowClose;
        default: return EventType::PointerMove;    // should never reach
        }
    }

} // namespace nandina::runtime