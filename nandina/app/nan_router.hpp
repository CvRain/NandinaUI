//
// app/nan_router — keep-alive page stack router.
//
// The router owns a stack of page frames. Each frame keeps both the page object
// and its built root node alive. Only the top frame is visible; lower frames stay
// mounted and keep their reactive bindings active, which makes shared Store
// updates immediately visible when popping back.
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
#define NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP

#include "../reactive/graph.hpp"
#include "../scene/control.hpp"
#include "../theme/theme_manager.hpp"
#include "nan_page.hpp"
#include "nan_store.hpp"
#include "async_scope.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::app
{

    class PageFrame;

    class NanRouter {
    public:
        explicit NanRouter(
            reactive::Graph& graph,
            const theme::NanTheme& theme,
            NanStore* store = nullptr,
            NanTypeKey store_key = nullptr,
            resource::ResourceManager* resources = nullptr,
            text::FontLoader* font_loader = nullptr,
            text::FontFamilyRegistry* font_families = nullptr,
            UiDispatcher* dispatcher = nullptr,
            BackgroundExecutor* background_executor = nullptr
        );
        explicit NanRouter(
            reactive::Graph& graph,
            theme::ThemeManager& theme_manager,
            NanStore* store = nullptr,
            NanTypeKey store_key = nullptr,
            resource::ResourceManager* resources = nullptr,
            text::FontLoader* font_loader = nullptr,
            text::FontFamilyRegistry* font_families = nullptr,
            UiDispatcher* dispatcher = nullptr,
            BackgroundExecutor* background_executor = nullptr
        );
        ~NanRouter() = default;

        NanRouter(const NanRouter&) = delete;
        auto operator=(const NanRouter&) -> NanRouter& = delete;
        NanRouter(NanRouter&&) = delete;
        auto operator=(NanRouter&&) -> NanRouter& = delete;

        [[nodiscard]] auto host() -> std::shared_ptr<scene::NanControl>;
        [[nodiscard]] auto graph() -> reactive::Graph&;
        [[nodiscard]] auto theme() const -> const theme::NanTheme&;
        [[nodiscard]] auto store_base() -> NanStore*;
        [[nodiscard]] auto depth() const -> std::size_t;
        [[nodiscard]] auto empty() const -> bool;
        [[nodiscard]] auto current_key() const -> std::string_view;
        [[nodiscard]] auto can_pop() const -> bool;

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        void set_store(StoreT& store) {
            store_ = &store;
            store_key_ = nan_type_key<StoreT>();
        }

        void clear_store();

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        auto push(ParamsT params) -> PageT& {
            auto page = std::make_unique<PageT>(std::move(params));
            auto* raw = page.get();
            push_page(std::move(page));
            return *raw;
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        [[nodiscard]] auto request_push(ParamsT params) -> bool {
            return post_command([this, params = std::move(params)]() mutable {
                (void)push<PageT>(std::move(params));
            });
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto push() -> PageT& {
            auto page = std::make_unique<PageT>();
            auto* raw = page.get();
            push_page(std::move(page));
            return *raw;
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto request_push() -> bool {
            return post_command([this] { (void)push<PageT>(); });
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        auto replace(ParamsT params) -> PageT& {
            remove_top();
            return push<PageT>(std::move(params));
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        [[nodiscard]] auto request_replace(ParamsT params) -> bool {
            return post_command([this, params = std::move(params)]() mutable {
                (void)replace<PageT>(std::move(params));
            });
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto replace() -> PageT& {
            remove_top();
            return push<PageT>();
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        [[nodiscard]] auto request_replace() -> bool {
            return post_command([this] { (void)replace<PageT>(); });
        }

        auto pop() -> bool;
        auto pop_to(std::string_view route_key) -> bool;
        void clear();

        [[nodiscard]] auto request_pop() -> bool;
        [[nodiscard]] auto request_pop_to(std::string route_key) -> bool;
        [[nodiscard]] auto request_clear() -> bool;

        /// 页面转场（默认关闭=即时切换）。开启后 push 淡入、pop/replace 淡出，且淡出
        /// 完成前保留被替换页面的生命周期（scope/async），随后才销毁。
        void set_transition_enabled(bool enabled);
        [[nodiscard]] auto transition_enabled() const -> bool;
        void set_transition_duration(float seconds);

    private:
        using NodePtr = std::shared_ptr<scene::NanNode>;

        struct Frame {
            std::unique_ptr<NanPage> page;
            std::shared_ptr<scene::NanNode2D> root;
            /// 转场包装节点（开启转场时非空，承载每页的淡入淡出 opacity）。
            std::shared_ptr<PageFrame> frame;
            std::unique_ptr<reactive::ReactiveScope> scope;
            std::unique_ptr<AsyncScope> async_scope;
            std::string key;
            bool active = false;
        };

        void push_page(std::unique_ptr<NanPage> page);
        void sync_visibility();
        void attach_root(const std::shared_ptr<scene::NanNode2D>& root);
        void detach_root(const std::shared_ptr<scene::NanNode2D>& root);
        void drop_frame(Frame& frame);
        /// 移除栈顶：开启转场时淡出并延迟 drop，否则即时 drop。
        void remove_top();
        [[nodiscard]] auto frame_node(const Frame& frame) const -> std::shared_ptr<scene::NanNode2D>;
        void fade_frame(Frame& frame, float target);
        void drop_completed_exits();
        [[nodiscard]] auto post_command(std::move_only_function<void()> command) -> bool;
        [[nodiscard]] auto make_context_for(Frame& frame) -> PageContext;

        reactive::Graph* graph_;
        const theme::NanTheme* theme_;
        theme::ThemeManager* theme_manager_ = nullptr;
        NanStore* store_ = nullptr;
        NanTypeKey store_key_ = nullptr;
        resource::ResourceManager* resources_ = nullptr;
        text::FontLoader* font_loader_ = nullptr;
        text::FontFamilyRegistry* font_families_ = nullptr;
        UiDispatcher* dispatcher_ = nullptr;
        BackgroundExecutor* background_executor_ = nullptr;
        std::shared_ptr<scene::NanControl> host_;
        std::vector<Frame> frames_;
        /// 淡出中的页面：生命周期（scope/async）保留，淡出完成后销毁。
        std::vector<Frame> exiting_;
        bool transition_enabled_ = false;
        float transition_duration_ = 0.2F;
        std::shared_ptr<void> command_lifetime_ = std::make_shared<int>(0);
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
