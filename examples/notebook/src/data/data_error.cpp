#include "data_error.hpp"

#include <format>

namespace nan_example::notebook::data
{
    namespace
    {
        [[nodiscard]] auto code_name(const DataErrorCode code) -> std::string_view {
            switch (code) {
                case DataErrorCode::invalid_argument:
                    return "invalid_argument";
                case DataErrorCode::io_error:
                    return "io_error";
                case DataErrorCode::open_failed:
                    return "open_failed";
                case DataErrorCode::sql_prepare_failed:
                    return "sql_prepare_failed";
                case DataErrorCode::sql_exec_failed:
                    return "sql_exec_failed";
                case DataErrorCode::sql_step_failed:
                    return "sql_step_failed";
                case DataErrorCode::bind_failed:
                    return "bind_failed";
                case DataErrorCode::constraint_violation:
                    return "constraint_violation";
                case DataErrorCode::migration_missing_resource:
                    return "migration_missing_resource";
                case DataErrorCode::migration_invalid_resource:
                    return "migration_invalid_resource";
                case DataErrorCode::migration_duplicate_version:
                    return "migration_duplicate_version";
                case DataErrorCode::migration_out_of_order:
                    return "migration_out_of_order";
                case DataErrorCode::migration_name_mismatch:
                    return "migration_name_mismatch";
                case DataErrorCode::migration_newer_schema:
                    return "migration_newer_schema";
            }
            return "unknown";
        }
    } // namespace

    auto describe(const DataError& error) -> std::string {
        std::string result(code_name(error.code));
        if (!error.operation.empty()) {
            result += " [" + error.operation + "]";
        }
        if (!error.message.empty()) {
            result += ": " + error.message;
        }
        if (error.sqlite_code != 0) {
            result += std::format(" (sqlite {})", error.sqlite_code);
        }
        return result;
    }
} // namespace nandina::examples::notebook::data
