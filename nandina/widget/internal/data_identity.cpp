#include "data_identity.hpp"

namespace nandina::widget::internal
{
    namespace
    {
        auto mix(std::size_t seed, const std::size_t value) noexcept -> std::size_t {
            seed ^=
                value + static_cast<std::size_t>(0x9e3779b97f4a7c15ULL) + (seed << 6) + (seed >> 2);
            return seed;
        }
    } // namespace

    auto RowIdHash::operator()(const RowId& id) const noexcept -> std::size_t {
        return std::hash<std::uint64_t> {}(id.value);
    }

    auto ColumnIdHash::operator()(const ColumnId& id) const noexcept -> std::size_t {
        return std::hash<std::string> {}(id.value);
    }

    auto CellIdHash::operator()(const CellId& id) const noexcept -> std::size_t {
        return mix(
            std::hash<std::uint64_t> {}(id.row.value),
            std::hash<std::string> {}(id.column.value)
        );
    }
} // namespace nandina::widget::internal
