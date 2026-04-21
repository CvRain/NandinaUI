module;

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <ranges>
#include <utility>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module nandina.log;

namespace nandina::log {

    struct State {
        std::vector<spdlog::sink_ptr> sinks;
        std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> named_loggers;
        std::mutex mtx;
        bool ready{false};
    };

    [[nodiscard]] auto global_state() noexcept -> State & {
        static State s;
        return s;
    }

    [[nodiscard]] auto to_spd(const Level level) noexcept -> spdlog::level::level_enum {
        const int clamped = std::clamp(static_cast<int>(level), 0, static_cast<int>(spdlog::level::off));
        return static_cast<spdlog::level::level_enum>(clamped);
    }

    [[nodiscard]] auto ensure_named_logger(std::string_view name) -> std::shared_ptr<spdlog::logger> {
        auto &s = global_state();
        auto key = std::string(name);

        std::scoped_lock lk(s.mtx);

        if (auto it = s.named_loggers.find(key); it != s.named_loggers.end())
            return it->second;

        auto lg = std::make_shared<spdlog::logger>(key, s.sinks.begin(), s.sinks.end());
        lg->set_level(spdlog::get_level());
        spdlog::register_logger(lg);
        s.named_loggers.emplace(key, lg);
        return lg;
    }
    
    void write(void *logger_ptr, const Level level, std::string msg) {
        if (logger_ptr == nullptr)
            return;
        static_cast<spdlog::logger *>(logger_ptr)->log(spdlog::source_loc{}, to_spd(level), msg.c_str());
    }

    void write_default(const Level level, std::string msg) {
        if (auto *logger = spdlog::default_logger_raw(); logger != nullptr)
            logger->log(spdlog::source_loc{}, to_spd(level), msg.c_str());
    }

    void Logger::set_level(Level level) const noexcept {
        if (!m_impl)
            return;
        auto *lg = static_cast<spdlog::logger *>(m_impl.get());
        lg->set_level(to_spd(level));
    }

    auto Logger::flush() const -> void {
        if (!m_impl)
            return;
        static_cast<spdlog::logger *>(m_impl.get())->flush();
    }

} // namespace nandina::log

// ============================================================
// 全局 API 实现
// ============================================================
namespace nandina::log {

    void init(std::string_view app_name, Level console_level, bool enable_file, std::string_view log_file) {
        auto &s = global_state();
        std::scoped_lock lk(s.mtx);
        if (s.ready)
            return;

        // 控制台 sink — 带 ANSI 色彩，输出到 stdout
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_level(to_spd(console_level));
        console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%n%$] [%^%l%$] %v");
        s.sinks.push_back(console);

        // 可选：滚动文件 sink（10 MB × 5 份，记录全级别）
        if (enable_file) {
            constexpr std::size_t max_bytes = 10u * 1024u * 1024u;
            constexpr std::size_t max_files = 5u;
            auto file =
                    std::make_shared<spdlog::sinks::rotating_file_sink_mt>(std::string(log_file), max_bytes, max_files);
            file->set_level(spdlog::level::trace);
            file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
            s.sinks.push_back(file);
        }

        // 根 logger（全局默认 logger）
        // 自身级别设为 trace，由各 sink 独立过滤，保留最大灵活性
        auto root = std::make_shared<spdlog::logger>(std::string(app_name), s.sinks.begin(), s.sinks.end());
        root->set_level(spdlog::level::trace);
        spdlog::set_default_logger(root);
        spdlog::set_level(to_spd(console_level));

        s.ready = true;
    }

    void shutdown() {
        // 先释放我们持有的引用，再让 spdlog 做最终清理，避免 use-after-free
        {
            auto &s = global_state();
            std::scoped_lock lk(s.mtx);
            s.named_loggers.clear();
            s.sinks.clear();
            s.ready = false;
        }
        spdlog::shutdown();
    }

    void set_level(Level level) noexcept {
        const auto spd = to_spd(level);
        spdlog::set_level(spd);

        auto &s = global_state();
        std::scoped_lock lk(s.mtx);
        for (const auto &lg: s.named_loggers | std::views::values)
            lg->set_level(spd);
    }

    void flush() {
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger> &lg) { lg->flush(); });
    }

    Logger get(const std::string_view name) {
        const auto spdlog_ptr = ensure_named_logger(name);
        // aliasing 构造：shared_ptr<void> 与 shared_ptr<logger> 共享引用计数，
        // 但对外暴露 void*，消费方无需感知 spdlog 类型
        std::shared_ptr<void> erased{spdlog_ptr, spdlog_ptr.get()};
        return Logger{std::move(erased), std::string(name)};
    }

} // namespace nandina::log
