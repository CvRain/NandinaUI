module;

#include <algorithm>
#include <memory>

export module nandina.widgets.split_row;

import nandina.foundation.nan_size;
import nandina.runtime.nan_widget;

export namespace nandina::widgets {

class SplitRow : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<SplitRow>;

    ~SplitRow() override = default;

    static auto create() -> Ptr {
        return Ptr{new SplitRow()};
    }

    auto set_gap(const float gap) noexcept -> SplitRow& {
        m_gap = std::max(0.0f, gap);
        return *this;
    }

    auto set_split_ratio(const float ratio) noexcept -> SplitRow& {
        m_split_ratio = std::clamp(ratio, 0.0f, 1.0f);
        return *this;
    }

    auto set_preferred_height(const float height) noexcept -> SplitRow& {
        m_preferred_height = std::max(0.0f, height);
        return *this;
    }

    auto set_leading(std::unique_ptr<runtime::NanWidget> child) -> runtime::NanWidget* {
        m_leading = child.get();
        add_child(std::move(child));
        return m_leading;
    }

    auto set_trailing(std::unique_ptr<runtime::NanWidget> child) -> runtime::NanWidget* {
        m_trailing = child.get();
        add_child(std::move(child));
        return m_trailing;
    }

    [[nodiscard]] auto leading_widget() const noexcept -> runtime::NanWidget* {
        return m_leading;
    }

    [[nodiscard]] auto trailing_widget() const noexcept -> runtime::NanWidget* {
        return m_trailing;
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        const float content_w = std::max(0.0f, w - m_gap);
        const float leading_w = content_w * m_split_ratio;
        const float trailing_w = std::max(0.0f, w - m_gap - leading_w);

        if (m_leading) {
            m_leading->set_bounds(x, y, leading_w, h);
        }
        if (m_trailing) {
            m_trailing->set_bounds(x + leading_w + m_gap, y, trailing_w, h);
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        float pref_h = m_preferred_height;
        if (!m_leading && !m_trailing) {
            return {0.0f, pref_h};
        }

        for_each_child([&](const runtime::NanWidget& child) {
            pref_h = std::max(pref_h, child.preferred_size().height());
        });

        return {0.0f, pref_h};
    }

protected:
    SplitRow() = default;

private:
    runtime::NanWidget* m_leading{nullptr};
    runtime::NanWidget* m_trailing{nullptr};
    float m_gap{16.0f};
    float m_split_ratio{0.5f};
    float m_preferred_height{0.0f};
};

} // namespace nandina::widgets