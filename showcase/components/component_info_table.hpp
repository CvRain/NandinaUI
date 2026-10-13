//
// Created by cvrain on 2026/10/11.
//

#pragma once
#include "page_chrome.hpp"

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

#include <string>
#include <vector>

namespace nandina::showcase
{

    /// 组件文档页头部的元数据表：文件位置、继承关系与注意事项。
    /// 输出一张 Card（配方自带内边距），内部是"键弱值强"的定义列表。
    class ComponentInfoTable {
    public:
        explicit ComponentInfoTable(app::PageContext& context);
        auto set_file_location(const std::string& file_location) -> ComponentInfoTable&;
        auto set_inherits(const std::string& inherits) -> ComponentInfoTable&;
        auto set_tips(const std::vector<std::string>& tips) -> ComponentInfoTable&;

        auto get_file_location() const -> std::string;
        auto get_inherits() const -> std::string;
        auto get_tips() const -> std::vector<std::string>;

        auto build() -> widget::authoring::NodeBuilder<widget::Card>;

    private:
        /// 一行「键 | 值」。键固定宽度以对齐，值占满剩余宽度并允许换行。
        [[nodiscard]] auto field_row(std::string_view key, std::string_view value) const
            -> widget::authoring::NodeBuilder<widget::Row>;

        app::PageContext& context_;

        std::string file_location_ {};
        std::string inherits_ {};
        std::vector<std::string> tips_ {};
    };

} // namespace nandina::showcase
