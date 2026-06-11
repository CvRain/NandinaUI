module;

#include <memory>
#include <functional>
#include <vector>
#include <cmath>
#include <algorithm>

export module nandina.reactive.animation;

export namespace nandina::reactive {

    enum class Easing {
        Linear,
        EaseIn,
        EaseOut,
        EaseInOut,
        SineIn,
        SineOut,
        SineInOut,
    };

    [[nodiscard]] inline auto apply_easing(float t, const Easing easing) -> float {
        t = std::clamp(t, 0.0f, 1.0f);
        switch (easing) {
        case Easing::Linear:
            return t;
        case Easing::EaseIn:
            return t * t * t;
        case Easing::EaseOut:
            return 1.0f - std::pow(1.0f - t, 3.0f);
        case Easing::EaseInOut:
            return t < 0.5f
                       ? 4.0f * t * t * t
                       : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
        case Easing::SineIn:
            return 1.0f - std::cos(t * 3.14159265f * 0.5f);
        case Easing::SineOut:
            return std::sin(t * 3.14159265f * 0.5f);
        case Easing::SineInOut:
            return -(std::cos(3.14159265f * t) - 1.0f) * 0.5f;
        }
        return t;
    }

    class NanAnimation {
    public:
        using Ptr = std::unique_ptr<NanAnimation>;
        using UpdateCallback = std::function<void(float progress)>;
        using CompletionCallback = std::function<void()>;

        static auto create(float duration) -> Ptr {
            auto anim = Ptr{new NanAnimation()};
            anim->m_duration = std::max(duration, 0.001f);
            return anim;
        }

        auto on_update(UpdateCallback cb) -> NanAnimation& {
            m_on_update = std::move(cb);
            return *this;
        }

        auto on_complete(CompletionCallback cb) -> NanAnimation& {
            m_on_complete = std::move(cb);
            return *this;
        }

        auto set_easing(const Easing easing) -> NanAnimation& {
            m_easing = easing;
            return *this;
        }

        auto set_loop(const bool loop) -> NanAnimation& {
            m_loop = loop;
            return *this;
        }

        auto start() -> void {
            m_elapsed = 0.0f;
            m_running = true;
            if (m_on_update) {
                m_on_update(0.0f);
            }
        }

        auto stop() -> void {
            m_running = false;
            m_elapsed = 0.0f;
        }

        auto pause() -> void {
            m_running = false;
        }

        auto resume() -> void {
            m_running = true;
        }

        [[nodiscard]] auto is_running() const noexcept -> bool {
            return m_running;
        }

        [[nodiscard]] auto progress() const noexcept -> float {
            return std::clamp(m_elapsed / m_duration, 0.0f, 1.0f);
        }

        auto tick(const float delta_seconds) -> void {
            if (!m_running) return;

            m_elapsed += delta_seconds;
            const float raw_t = std::clamp(m_elapsed / m_duration, 0.0f, 1.0f);
            const float eased_t = apply_easing(raw_t, m_easing);

            if (m_on_update) {
                m_on_update(eased_t);
            }

            if (raw_t >= 1.0f) {
                if (m_loop) {
                    m_elapsed = 0.0f;
                    if (m_on_update) {
                        m_on_update(0.0f);
                    }
                } else {
                    m_running = false;
                    if (m_on_complete) {
                        m_on_complete();
                    }
                }
            }
        }

    private:
        NanAnimation() = default;

        UpdateCallback m_on_update;
        CompletionCallback m_on_complete;
        Easing m_easing{Easing::Linear};
        float m_duration{1.0f};
        float m_elapsed{0.0f};
        bool m_running{false};
        bool m_loop{false};
    };

    class AnimationManager {
    public:
        auto add(NanAnimation::Ptr anim) -> NanAnimation* {
            if (!anim) return nullptr;
            auto* raw = anim.get();
            m_animations.push_back(std::move(anim));
            return raw;
        }

        auto remove(const NanAnimation* anim) -> void {
            auto it = std::remove_if(m_animations.begin(), m_animations.end(),
                [anim](const auto& ptr) { return ptr.get() == anim; });
            if (it != m_animations.end()) {
                m_animations.erase(it, m_animations.end());
            }
        }

        auto update(const float delta_seconds) -> void {
            for (auto& anim : m_animations) {
                anim->tick(delta_seconds);
            }
            m_animations.erase(
                std::remove_if(m_animations.begin(), m_animations.end(),
                    [](const auto& ptr) { return !ptr->is_running(); }),
                m_animations.end()
            );
        }

        auto clear() -> void {
            m_animations.clear();
        }

        [[nodiscard]] auto count() const noexcept -> std::size_t {
            return m_animations.size();
        }

    private:
        std::vector<NanAnimation::Ptr> m_animations;
    };

} // namespace nandina::reactive
