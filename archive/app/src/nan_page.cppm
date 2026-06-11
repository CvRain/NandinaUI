module;

#include <memory>
#include <string_view>

export module nandina.app.page;

import nandina.app.application;
import nandina.widgets.icon;

export namespace nandina::app {

    /**
     * NanPage — 页面抽象基类（Issue 063）
     *
     * 每一个 Page 代表一个可导航的内容单元。
     * 职责：
     *   - 声明路由键（route_key）与显示名（title）
     *   - 提供 build() 工厂方法，按需构造页面组件
     *
     * 使用方：
     *   struct MyPage : NanPage {
     *       auto route_key() const noexcept -> std::string_view override { return "my-page"; }
     *       auto title()     const noexcept -> std::string_view override { return "My Page"; }
     *       auto build()     -> NanComponent::Ptr             override { return mount(...); }
     *   };
     */
    class NanPage {
    public:
        virtual ~NanPage() = default;

        /// 路由标识符，用于 Router::navigate_to() 匹配
        [[nodiscard]] virtual auto route_key() const noexcept -> std::string_view = 0;

        /// 用于 UI 展示的页面标题
        [[nodiscard]] virtual auto title() const noexcept -> std::string_view = 0;

        /// 侧边栏图标类型，子类可覆盖
        [[nodiscard]] virtual auto icon_type() const noexcept -> widgets::IconType {
            return widgets::IconType::Square;
        }

        /// 构造并返回该页面的根组件，每次导航到本页时调用
        [[nodiscard]] virtual auto build() -> NanComponent::Ptr = 0;
    };

} // namespace nandina::app
