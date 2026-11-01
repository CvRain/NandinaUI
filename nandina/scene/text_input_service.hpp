//
// scene/text_input_service — platform-neutral text input session protocol (D1 phase 2).
//
// 方向约定（见 docs/references/focus_and_input.md §5.1）：
//   scene → service：开始/结束会话、更新输入区域、请求键盘可见性；
//   service → scene：组合更新/提交/取消、键盘可见性回报，经受保护的 sink 进入场景。
// 会话身份由 scene 分配；迟到的旧会话事件不得写入新目标。
//

#ifndef NANDINA_EXPERIMENT_SCENE_TEXT_INPUT_SERVICE_HPP
#define NANDINA_EXPERIMENT_SCENE_TEXT_INPUT_SERVICE_HPP

#include "../foundation/geometry.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace nandina::scene
{
    /// 输入用途；只用于文本会话请求，不改变提交文本路径。
    enum class TextInputPurpose : std::uint8_t {
        text,
        password,
        email,
        number,
    };

    /// 平台能力位；能力不等于每次请求一定成功。
    enum class TextInputCapabilities : std::uint32_t {
        none = 0,
        composition = 1U << 0U,
        soft_keyboard = 1U << 1U,
        input_area = 1U << 2U,
        visibility_feedback = 1U << 3U,
        occlusion = 1U << 4U,
    };

    [[nodiscard]] constexpr auto
    operator|(const TextInputCapabilities lhs, const TextInputCapabilities rhs)
        -> TextInputCapabilities {
        return static_cast<TextInputCapabilities>(
            static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs)
        );
    }

    [[nodiscard]] constexpr auto
    has_capability(const TextInputCapabilities capabilities, const TextInputCapabilities wanted)
        -> bool {
        return (static_cast<std::uint32_t>(capabilities) & static_cast<std::uint32_t>(wanted)) != 0;
    }

    /// 请求结果：未支持、服务/会话不可用、平台拒绝都可诊断；accepted 不代表键盘已显示。
    enum class TextInputRequestResult : std::uint8_t {
        accepted,
        unsupported,
        unavailable,
        rejected,
    };

    enum class TextInputEndReason : std::uint8_t {
        focus_lost,
        window_inactive,
        service_detached,
        target_invalid,
    };

    enum class TextInputKeyboardVisibility : std::uint8_t {
        unknown,
        hidden,
        visible,
    };

    /// 会话身份；由 scene 分配，服务原样带回事件。
    struct TextInputSession {
        std::uint64_t id = 0;

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return id != 0;
        }
        friend constexpr auto operator==(TextInputSession, TextInputSession) noexcept
            -> bool = default;
    };

    struct TextInputBeginRequest {
        TextInputSession session {};
        TextInputPurpose purpose = TextInputPurpose::text;
        bool multiline = false;
    };

    /// 输入区域（窗口逻辑坐标）。当前阶段服务可以不支持，能力位如实报告。
    struct TextInputArea {
        foundation::NanRect caret {};
        foundation::NanRect window {};
    };

    /// 已提交文本与组合文本都由 scene 的编辑目标协议消费；平台值不出现 SDK 类型。
    class ITextInputTarget {
    public:
        virtual ~ITextInputTarget() = default;

        [[nodiscard]] virtual auto text_input_purpose() const -> TextInputPurpose {
            return TextInputPurpose::text;
        }
        [[nodiscard]] virtual auto text_input_multiline() const -> bool {
            return false;
        }
        [[nodiscard]] virtual auto text_input_read_only() const -> bool {
            return true;
        }
        [[nodiscard]] virtual auto text_input_caret_area() const -> foundation::NanRect {
            return foundation::NanRect::empty();
        }

        virtual void text_input_set_composition(
            std::string_view text,
            std::size_t selection_start,
            std::size_t selection_end
        ) = 0;
        virtual void text_input_commit(std::string_view text) = 0;
        virtual void text_input_cancel_composition() = 0;
    };

    /// 平台事件（service → scene 的输入侧）。host 把它交给 service，service 补会话身份后转发 sink。
    enum class TextInputEventType : std::uint8_t {
        composition_updated,
        composition_committed,
        composition_cancelled,
        keyboard_visibility_changed,
    };

    struct TextInputPlatformEvent {
        TextInputEventType type = TextInputEventType::composition_updated;
        std::string text;
        std::size_t selection_start = 0;
        std::size_t selection_end = 0;
        TextInputKeyboardVisibility visibility = TextInputKeyboardVisibility::unknown;
    };

    /// scene 会话管理器实现的回报入口；实现方必须按会话身份过滤并保持 UI 线程串行。
    class ITextInputSessionSink {
    public:
        virtual ~ITextInputSessionSink() = default;

        virtual void on_composition_updated(
            TextInputSession session,
            std::string_view text,
            std::size_t selection_start,
            std::size_t selection_end
        ) = 0;
        virtual void on_text_committed(TextInputSession session, std::string_view text) = 0;
        virtual void on_composition_cancelled(TextInputSession session) = 0;
        virtual void on_keyboard_visibility_changed(
            TextInputSession session,
            TextInputKeyboardVisibility visibility
        ) = 0;
    };

    /// 窗口持有/注入的文本输入服务。所有方法在 UI 线程调用。
    class ITextInputService {
    public:
        virtual ~ITextInputService() = default;

        [[nodiscard]] virtual auto capabilities() const -> TextInputCapabilities = 0;

        virtual auto begin_session(const TextInputBeginRequest& request)
            -> TextInputRequestResult = 0;
        virtual auto update_caret_area(TextInputSession session, const TextInputArea& area)
            -> TextInputRequestResult = 0;
        virtual auto request_keyboard_visibility(TextInputSession session, bool visible)
            -> TextInputRequestResult = 0;
        virtual void end_session(TextInputSession session, TextInputEndReason reason) = 0;

        virtual void set_sink(ITextInputSessionSink* sink) noexcept = 0;

        /// host 把平台事件交给服务；服务用当前会话身份标注后转发 sink。没有活动会话时丢弃。
        virtual void dispatch(const TextInputPlatformEvent& event) = 0;
    };
} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_SCENE_TEXT_INPUT_SERVICE_HPP
