#include "data_identity.hpp"

namespace std
{
    auto hash<nandina::widget::RowId>::operator()(const nandina::widget::RowId& id) const noexcept
        -> std::size_t {
        return hash<std::uint64_t> {}(id.value);
    }

    auto
    hash<nandina::widget::ColumnId>::operator()(const nandina::widget::ColumnId& id) const noexcept
        -> std::size_t {
        return hash<std::string> {}(id.value);
    }

    auto hash<nandina::widget::CellId>::operator()(const nandina::widget::CellId& id) const noexcept
        -> std::size_t {
        auto seed = hash<nandina::widget::RowId> {}(id.row);
        seed ^= hash<nandina::widget::ColumnId> {}(id.column)
            + static_cast<std::size_t>(0x9e3779b97f4a7c15ULL) + (seed << 6) + (seed >> 2);
        return seed;
    }
} // namespace std
