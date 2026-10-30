#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_DATA_ERROR_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_DATA_ERROR_HPP

#include <expected>
#include <string>
#include <string_view>

namespace nan_example::notebook::data
{
    enum class DataErrorCode {
        invalid_argument,
        io_error,
        open_failed,
        sql_prepare_failed,
        sql_exec_failed,
        sql_step_failed,
        bind_failed,
        constraint_violation,
        migration_missing_resource,
        migration_invalid_resource,
        migration_duplicate_version,
        migration_out_of_order,
        migration_name_mismatch,
        migration_newer_schema,
    };

    struct DataError {
        DataErrorCode code = DataErrorCode::sql_exec_failed;
        std::string operation;
        std::string message;
        int sqlite_code = 0;
    };

    template<typename T>
    using DataResult = std::expected<T, DataError>;

    [[nodiscard]] auto describe(const DataError& error) -> std::string;
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_DATA_ERROR_HPP
