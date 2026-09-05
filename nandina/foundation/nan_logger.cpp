//
// foundation/nan_logger — spdlog-backed implementation.
//

#include "nan_logger.hpp"

#include <exception>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <print>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nandina::log
{
    namespace detail
    {
        struct LoggerBackend {
            std::mutex mutex;
            std::shared_ptr<spdlog::logger> logger;
        };
    } // namespace detail

    namespace
    {
        struct LoggerState {
            std::mutex mutex;
            std::vector<spdlog::sink_ptr> sinks;
            std::shared_ptr<detail::LoggerBackend> root;
            std::unordered_map<std::string, std::shared_ptr<detail::LoggerBackend>> named;
            LogLevel level = LogLevel::info;
            bool initialized = false;
            bool explicitly_initialized = false;
        };

        [[nodiscard]] auto state() noexcept -> LoggerState& {
            static LoggerState instance;
            return instance;
        }

        [[nodiscard]] auto to_spd(const LogLevel level) noexcept -> spdlog::level::level_enum {
            switch (level) {
                case LogLevel::trace:
                    return spdlog::level::trace;
                case LogLevel::debug:
                    return spdlog::level::debug;
                case LogLevel::info:
                    return spdlog::level::info;
                case LogLevel::warning:
                    return spdlog::level::warn;
                case LogLevel::error:
                    return spdlog::level::err;
                case LogLevel::critical:
                    return spdlog::level::critical;
                case LogLevel::off:
                    return spdlog::level::off;
            }
            return spdlog::level::off;
        }

        void validate(const LoggerConfig& config) {
            if (config.name.empty()) {
                throw std::invalid_argument("LoggerConfig name cannot be empty");
            }
            if (config.file && (config.max_file_size == 0 || config.max_files == 0)) {
                throw std::invalid_argument("LoggerConfig rotating file limits must be positive");
            }
        }

        void replace_logger(
            const std::shared_ptr<detail::LoggerBackend>& backend,
            std::shared_ptr<spdlog::logger> logger
        ) {
            std::scoped_lock lock(backend->mutex);
            backend->logger = std::move(logger);
        }

        [[nodiscard]] auto make_logger(
            std::string name,
            const std::vector<spdlog::sink_ptr>& sinks,
            const LogLevel level
        ) -> std::shared_ptr<spdlog::logger> {
            auto logger =
                std::make_shared<spdlog::logger>(std::move(name), sinks.begin(), sinks.end());
            logger->set_level(to_spd(level));
            return logger;
        }

        void configure(LoggerState& current, const LoggerConfig& config) {
            std::vector<spdlog::sink_ptr> sinks;
            auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console->set_level(spdlog::level::trace);
            console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%n%$] [%^%l%$] %v");
            sinks.push_back(std::move(console));

            if (config.file) {
                auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    config.file->string(),
                    config.max_file_size,
                    config.max_files
                );
                file->set_level(spdlog::level::trace);
                file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
                sinks.push_back(std::move(file));
            }

            const auto root =
                current.root ? current.root : std::make_shared<detail::LoggerBackend>();
            std::vector<
                std::pair<std::shared_ptr<detail::LoggerBackend>, std::shared_ptr<spdlog::logger>>>
                replacements;
            replacements.reserve(current.named.size() + 1);
            replacements.emplace_back(root, make_logger(config.name, sinks, config.level));
            for (const auto& [name, backend]: current.named) {
                replacements.emplace_back(backend, make_logger(name, sinks, config.level));
            }
            for (auto& [backend, logger]: replacements) {
                replace_logger(backend, std::move(logger));
            }

            current.root = root;
            current.sinks = std::move(sinks);
            current.level = config.level;
            current.initialized = true;
        }

        void ensure_initialized(LoggerState& current) {
            if (!current.initialized) {
                configure(current, LoggerConfig {});
            }
        }

        [[nodiscard]] auto
        snapshot_logger(const std::shared_ptr<detail::LoggerBackend>& backend) noexcept
            -> std::shared_ptr<spdlog::logger> {
            if (!backend) {
                return nullptr;
            }
            std::scoped_lock lock(backend->mutex);
            return backend->logger;
        }

        /// 日志后端失效时直接写 stderr，避免回退路径再次依赖日志或格式化设施。
        void report_backend_failure(const char* operation, const char* message) noexcept {
        //todo 在windows上可能出现报错：
        // in function `std::vprint_unicode(_iobuf*, std::basic_string_view<char, std::char_traits<char> >, std::basic_format_args<std::basic_format_context<std::__format::_Sink_iter<char>, char> >)':
        // undefined reference to `std::__open_terminal(_iobuf*)'
	// undefined reference to `std::__write_to_terminal(void*, std::span<char, 18446744073709551615ull>)'
            std::println(
                stderr,
                "[LoggerBackend] Failed to {}{}{}",
                operation,
                message != nullptr ? ": " : "",
                message != nullptr ? message : ""
            );
        }
    } // namespace

    namespace detail
    {
        auto enabled(const std::shared_ptr<LoggerBackend>& backend, const LogLevel level) noexcept
            -> bool {
            const auto logger = snapshot_logger(backend);
            return logger && logger->should_log(to_spd(level));
        }

        void write(
            const std::shared_ptr<LoggerBackend>& backend,
            const LogLevel level,
            const std::string_view message
        ) noexcept {
            try {
                if (const auto logger = snapshot_logger(backend); logger) {
                    logger->log(
                        spdlog::source_loc {},
                        to_spd(level),
                        spdlog::string_view_t(message.data(), message.size())
                    );
                }
            }
            catch (const std::exception& e) {
                // 日志后端故障不能中断应用控制流。
                report_backend_failure("write log", e.what());
            }
            catch (...) {
                report_backend_failure("write log", nullptr);
            }
        }

        void
        set_level(const std::shared_ptr<LoggerBackend>& backend, const LogLevel level) noexcept {
            if (const auto logger = snapshot_logger(backend); logger) {
                logger->set_level(to_spd(level));
            }
        }

        void flush(const std::shared_ptr<LoggerBackend>& backend) noexcept {
            try {
                if (const auto logger = snapshot_logger(backend); logger) {
                    logger->flush();
                }
            }
            catch (const std::exception& e) {
                // 与普通写入一致，flush 失败不改变应用控制流。
                report_backend_failure("flush log", e.what());
            }
            catch (...) {
                report_backend_failure("flush log", nullptr);
            }
        }

        auto default_backend() -> std::shared_ptr<LoggerBackend> {
            try {
                auto& current = state();
                std::scoped_lock lock(current.mutex);
                ensure_initialized(current);
                return current.root;
            }
            catch (const std::exception& e) {
                report_backend_failure("initialize default backend", e.what());
                return nullptr;
            }
            catch (...) {
                report_backend_failure("initialize default backend", nullptr);
                return nullptr;
            }
        }
    } // namespace detail

    void initialize(const LoggerConfig& config) {
        auto& current = state();
        std::scoped_lock lock(current.mutex);
        if (current.explicitly_initialized) {
            return;
        }
        validate(config);
        configure(current, config);
        current.explicitly_initialized = true;
    }

    void shutdown() noexcept {
        try {
            auto& current = state();
            std::scoped_lock lock(current.mutex);
            if (current.root) {
                detail::flush(current.root);
                replace_logger(current.root, nullptr);
            }
            for (const auto& [name, backend]: current.named) {
                (void)name;
                detail::flush(backend);
                replace_logger(backend, nullptr);
            }
            current.named.clear();
            current.root.reset();
            current.sinks.clear();
            current.level = LogLevel::info;
            current.initialized = false;
            current.explicitly_initialized = false;
        }
        catch (const std::exception& e) {
            // shutdown 保持 noexcept，后端与分配错误留给进程回收。
            report_backend_failure("shutdown logger", e.what());
        }
        catch (...) {
            report_backend_failure("shutdown logger", nullptr);
        }
    }

    void set_level(const LogLevel level) noexcept {
        try {
            auto& current = state();
            std::scoped_lock lock(current.mutex);
            ensure_initialized(current);
            current.level = level;
            detail::set_level(current.root, level);
            for (const auto& [name, backend]: current.named) {
                (void)name;
                detail::set_level(backend, level);
            }
        }
        catch (const std::exception& e) {
            // 全局配置失败时保持现有日志状态。
            report_backend_failure("set log level", e.what());
        }
        catch (...) {
            report_backend_failure("set log level", nullptr);
        }
    }

    void flush() noexcept {
        try {
            auto& current = state();
            std::scoped_lock lock(current.mutex);
            if (!current.initialized) {
                return;
            }
            detail::flush(current.root);
            for (const auto& [name, backend]: current.named) {
                (void)name;
                detail::flush(backend);
            }
        }
        catch (const std::exception& e) {
            // flush 保持 noexcept。
            report_backend_failure("flush log", e.what());
        }
        catch (...) {
            report_backend_failure("flush log", nullptr);
        }
    }

    auto get(const std::string_view name) -> Logger {
        if (name.empty()) {
            throw std::invalid_argument("Logger name cannot be empty");
        }

        auto& current = state();
        std::scoped_lock lock(current.mutex);
        ensure_initialized(current);
        auto key = std::string(name);
        if (const auto found = current.named.find(key); found != current.named.end()) {
            return Logger {found->second, std::move(key)};
        }

        auto backend = std::make_shared<detail::LoggerBackend>();
        replace_logger(backend, make_logger(key, current.sinks, current.level));
        current.named.emplace(key, backend);
        return Logger {std::move(backend), std::move(key)};
    }

} // namespace nandina::log
