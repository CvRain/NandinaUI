module;

#include <memory>
#include <thorvg-1/thorvg.h>
#include <vector>

export module nandina.runtime.nan_widget;

export namespace nandina {
    /**
     * @brief NanWidget — 所有 UI 元素的基类 (M1)
     */
    class NanWidget {
    public:
        using Ptr = std::unique_ptr<NanWidget>;

        NanWidget() {
        } // Avoid = default for now to dodge GCC module bugs
        virtual ~NanWidget() {
        }

        // ── 空间属性 ──────────────────────────────────────────
        [[nodiscard]] auto x() const noexcept -> float {
            return m_x;
        }

        [[nodiscard]] auto y() const noexcept -> float {
            return m_y;
        }

        [[nodiscard]] auto width() const noexcept -> float {
            return m_width;
        }

        [[nodiscard]] auto height() const noexcept -> float {
            return m_height;
        }

        auto set_position(float x, float y) noexcept -> void {
            m_x = x;
            m_y = y;
        }

        auto set_size(float w, float h) noexcept -> void {
            m_width = w;
            m_height = h;
        }

        // ── 组合能力 ──────────────────────────────────────────
        auto add_child(Ptr child) -> NanWidget* {
            if (!child)
                return nullptr;
            auto *ptr = child.get();
            m_children.push_back(std::move(child));
            return ptr;
        }

        auto clear_children() -> void {
            m_children.clear();
        }

        virtual void draw(tvg::SwCanvas &canvas) {
            on_draw(canvas);
            for (const auto &child: m_children) {
                child->draw(canvas);
            }
        }

    protected:
        virtual void on_draw(tvg::SwCanvas & /*canvas*/) {
        }

    private:
        float m_x{0.0f};
        float m_y{0.0f};
        float m_width{0.0f};
        float m_height{0.0f};

        std::vector<Ptr> m_children;
    };
} // namespace nandina::runtime
