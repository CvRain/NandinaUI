#include "user_service_demo_page.hpp"

#include "app/notebook_store.hpp"
#include "data/data_error.hpp"

#include <nandina/foundation/geometry.hpp>
#include <nandina/widget/controls.hpp>

#include <format>
#include <string>

namespace nan_example::notebook::ui
{
    namespace
    {
        namespace notebook_app = app;

        [[nodiscard]] auto describe_users(const core::UserService& service) -> std::string {
            auto total = service.user_count();
            if (!total) {
                return "读取用户数量失败: " + data::describe(total.error());
            }
            auto exists = service.user_exist();
            if (!exists) {
                return "读取用户状态失败: " + data::describe(exists.error());
            }
            return std::format("用户总数: {}；是否已有用户: {}", *total, *exists ? "是" : "否");
        }
    } // namespace

    auto UserServiceDemoPage::build(nandina::app::PageContext& context) -> nandina::widget::View {
        const auto ui = context.ui();
        auto& store = context.store<notebook_app::NotebookStore>();
        auto& service = store.user_service();

        auto& status = ui.signal_value(describe_users(service));

        const auto title = ui.make<nandina::widget::Label>("UserService 演示")
                               .color_token(nandina::theme::ColorToken::foreground)
                               .font_size(28.0F)
                               .width(nandina::widget::authoring::fill)
                               .build();

        const auto description =
            ui.make<nandina::widget::Label>(
                  "这个页面没有构造 Database、Repository 或 Service。它通过 "
                  "PageContext::store<NotebookStore>() 取得应用级组合根，再从 store "
                  "取 UserService。"
            )
                .color_token(nandina::theme::ColorToken::muted_foreground)
                .font_size(14.0F)
                .width(nandina::widget::authoring::fill)
                .configure([](nandina::widget::Label& label) {
                    label.set_overflow(nandina::text::TextOverflow::wrap);
                    label.set_max_lines(4);
                })
                .build();

        const auto status_label = ui.make<nandina::widget::Label>(status)
                                      .color_token(nandina::theme::ColorToken::info)
                                      .font_size(16.0F)
                                      .width(nandina::widget::authoring::fill)
                                      .build();

        const auto refresh =
            ui.make<nandina::widget::Button>("刷新")
                .treatment(nandina::theme::ButtonTreatment::tonal)
                .on_click([&service, &status] { status.set(describe_users(service)); })
                .build();

        const auto create =
            ui.make<nandina::widget::Button>("创建 demo 用户")
                .treatment(nandina::theme::ButtonTreatment::filled)
                .on_click([&service, &status] {
                    auto created = service.create_user("demo", "demo-password-hash");
                    if (!created) {
                        if (created.error().code == data::DataErrorCode::constraint_violation) {
                            status.set("demo 用户已存在；点击「刷新」查看数量。");
                        }
                        else {
                            status.set("创建失败: " + data::describe(created.error()));
                        }
                        return;
                    }
                    status.set(
                        std::format(
                            "已创建用户 {}（id={}）。真实注册流程应由 AuthService 先哈希密码。",
                            created->username,
                            created->id
                        )
                    );
                })
                .build();

        const auto actions = ui.row().gap(8.0F).children(refresh, create);

        const auto content = ui.column()
                                 .width(nandina::widget::authoring::fill)
                                 .max_width(720.0F)
                                 .gap(16.0F)
                                 .cross_alignment(nandina::widget::LayoutAlignment::stretch)
                                 .children(title)
                                 .children(description)
                                 .children(status_label)
                                 .children(actions);

        return ui.padding(nandina::foundation::NanInsets::all(32.0F))
            .width(nandina::widget::authoring::fill)
            .height(nandina::widget::authoring::fill)
            .child(content)
            .build();
    }
} // namespace nan_example::notebook::ui
