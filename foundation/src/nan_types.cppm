module;

// ============================================================
// 全局模块片段
// ============================================================
#include <cstdint>

export module nandina.foundation.nan_types;

// ============================================================
// Issue 005 — Foundation 层基础枚举与通用类型
//
// 职责：提供跨模块共享的基础类型与枚举，
//       避免 layout / runtime / widgets 各层重复定义。
// 命名空间：nandina::types
//
// 迁移来源：
//   - PointerButton: nandina::runtime → nandina::types（Isssue 005）
//
// consum 示例（消费方）：
//   import nandina.foundation.nan_types;
//   using nandina::types::Axis;
//   using nandina::types::Align;
// ============================================================
export namespace nandina::types {
    // ── Layout 方向与对齐（供 layout / widget 使用）─────────

    // Axis：主轴方向
    enum class Axis : std::uint8_t {
        Horizontal = 0,
        Vertical = 1,
    };

    // Align：沿交叉轴的对齐方式
    enum class Align : std::uint8_t {
        Start = 0,
        Center = 1,
        End = 2,
        Stretch = 3,
    };

    // Justify：主轴方向的内容分布
    enum class Justify : std::uint8_t {
        Start = 0,
        Center = 1,
        End = 2,
        SpaceBetween = 3,
        SpaceAround = 4,
        SpaceEvenly = 5,
    };

    // Direction：文本 / 布局方向
    enum class Direction : std::uint8_t {
        LeftToRight = 0,
        RightToLeft = 1,
        TopToBottom = 2,
        BottomToTop = 3,
    };

    // ── 可见性与交互 ──────────────────────────────────────────

    // Visibility：控件可见性状态
    //   Visible   → 正常显示，参与布局与命中测试
    //   Hidden    → 隐藏，占位但不可见（仍参与布局）
    //   Collapsed → 折叠，不占空间、不可见
    enum class Visibility : std::uint8_t {
        Visible = 0,
        Hidden = 1,
        Collapsed = 2,
    };

    // ── 输入事件相关（从 runtime 迁移）────────────────────────

    // PointerButton：鼠标 / 指针按键
    // （迁移自 nandina::runtime::PointerButton，Issue 005）
    enum class PointerButton : std::uint8_t {
        Unknown = 0,
        Left = 1,
        Middle = 2,
        Right = 3,
        X1 = 4,
        X2 = 5,
    };

    // KeyCode：键盘按键码（最小常用子集，后续可扩展）
    enum class KeyCode : std::uint16_t {
        Unknown = 0,

        // 字母键
        A = 4, B, C, D, E, F, G, H, I, J,
        K, L, M, N, O, P, Q, R, S, T,
        U, V, W, X, Y, Z,

        // 数字键
        Key0 = 39, Key1, Key2, Key3, Key4,
        Key5, Key6, Key7, Key8, Key9,

        // 功能键
        F1 = 58, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,

        // 控制键
        Return = 40,
        Escape = 41,
        Backspace = 42,
        Tab = 43,
        Space = 44,
        Delete = 76,

        // 方向键
        LeftArrow = 80,
        RightArrow = 79,
        UpArrow = 82,
        DownArrow = 81,
    };

    // MouseCursor：鼠标光标样式
    enum class MouseCursor : std::uint8_t {
        Default = 0,
        Pointer = 1,
        Text = 2,
        Crosshair = 3,
        Wait = 4,
        Hand = 5,
        ResizeNS = 6,
        ResizeEW = 7,
        ResizeNESW = 8,
        ResizeNWSE = 9,
        NotAllowed = 10,
    };
} // namespace nandina::types
