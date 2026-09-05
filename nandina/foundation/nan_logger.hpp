//
// foundation/nan_logger — Nandina 统一日志入口。
//
// 公共接口只依赖标准 C++ 类型。具体日志后端留在实现单元中，框架与应用代码
// 不需要直接包含或配置 spdlog。
//

#ifndef NANDINA_EXPERIMENT_FOUNDATION_NAN_LOGGER_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_NAN_LOGGER_HPP

#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace nandina::log
{
    enum class LogLevel {
        trace,
        debug,
        info,
        warning,
        error,
        critical,
        off,
    };

    struct LoggerConfig {
        std::string name = "nandina";
        LogLevel level = LogLevel::info;
        std::optional<std::filesystem::path> file;
        std::size_t max_file_size = 10U * 1024U * 1024U;
        std::size_t max_files = 5U;
    };

    namespace detail
    {
        struct LoggerBackend;

        [[nodiscard]] auto
        enabled(const std::shared_ptr<LoggerBackend>& backend, LogLevel level) noexcept -> bool;
        void write(
            const std::shared_ptr<LoggerBackend>& backend,
            LogLevel level,
            std::string_view message
        ) noexcept;
        void set_level(const std::shared_ptr<LoggerBackend>& backend, LogLevel level) noexcept;
        void flush(const std::shared_ptr<LoggerBackend>& backend) noexcept;

        [[nodiscard]] auto default_backend() -> std::shared_ptr<LoggerBackend>;

        template<typename... Args>
        void write_formatted(
            const std::shared_ptr<LoggerBackend>& backend,
            const LogLevel level,
            std::format_string<Args...> format,
            Args&&... args
        ) {
            if (enabled(backend, level)) {
                write(backend, level, std::format(format, std::forward<Args>(args)...));
            }
        }
    } // namespace detail

    /// 可复制的具名日志句柄。同名句柄共享后端和局部级别设置。
    class Logger {
    public:
        Logger() = default;

        void set_level(LogLevel level) const noexcept {
            detail::set_level(backend_, level);
        }

        [[nodiscard]] auto enabled(LogLevel level) const noexcept -> bool {
            return detail::enabled(backend_, level);
        }

        void flush() const noexcept {
            detail::flush(backend_);
        }

        [[nodiscard]] auto name() const noexcept -> std::string_view {
            return name_;
        }

        template<typename... Args>
        void trace(std::format_string<Args...> format, Args&&... args) const {
            log(LogLevel::trace, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(std::format_string<Args...> format, Args&&... args) const {
            log(LogLevel::debug, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void info(std::format_string<Args...> format, Args&&... args) const {
            log(LogLevel::info, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warn(std::format_string<Args...> format, Args&&... args) const {
            log(LogLevel::warning, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(std::format_string<Args...> format, Args&&... args) const {
            log(LogLevel::error, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void critical(std::format_string<Args...> format, Args&&... args) const {
            log(LogLevel::critical, format, std::forward<Args>(args)...);
        }

    private:
        Logger(std::shared_ptr<detail::LoggerBackend> backend, std::string name) noexcept:
            backend_(std::move(backend)),
            name_(std::move(name)) {}

        template<typename... Args>
        void log(const LogLevel level, std::format_string<Args...> format, Args&&... args) const {
            detail::write_formatted(backend_, level, format, std::forward<Args>(args)...);
        }

        std::shared_ptr<detail::LoggerBackend> backend_;
        std::string name_;

        friend auto get(std::string_view name) -> Logger;
    };

    /// 初始化进程级日志系统。首次显式配置生效，后续调用保持幂等。
    /// 未显式初始化时，第一次日志操作会使用默认配置。
    void initialize(const LoggerConfig& config = {});

    /// 刷写并释放所有日志后端。既有 Logger 句柄会安全变为空操作。
    void shutdown() noexcept;

    /// 调整根 logger 和所有现有具名 logger 的级别。
    void set_level(LogLevel level) noexcept;
    void flush() noexcept;

    /// 获取具名 logger；相同名称共享同一个后端实例。
    [[nodiscard]] auto get(std::string_view name) -> Logger;

    template<typename... Args>
    void trace(std::format_string<Args...> format, Args&&... args) {
        detail::write_formatted(
            detail::default_backend(),
            LogLevel::trace,
            format,
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void debug(std::format_string<Args...> format, Args&&... args) {
        detail::write_formatted(
            detail::default_backend(),
            LogLevel::debug,
            format,
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void info(std::format_string<Args...> format, Args&&... args) {
        detail::write_formatted(
            detail::default_backend(),
            LogLevel::info,
            format,
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void warn(std::format_string<Args...> format, Args&&... args) {
        detail::write_formatted(
            detail::default_backend(),
            LogLevel::warning,
            format,
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void error(std::format_string<Args...> format, Args&&... args) {
        detail::write_formatted(
            detail::default_backend(),
            LogLevel::error,
            format,
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    void critical(std::format_string<Args...> format, Args&&... args) {
        detail::write_formatted(
            detail::default_backend(),
            LogLevel::critical,
            format,
            std::forward<Args>(args)...
        );
    }

} // namespace nandina::log

#endif // NANDINA_EXPERIMENT_FOUNDATION_NAN_LOGGER_HPP
