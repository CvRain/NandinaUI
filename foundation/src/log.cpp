module;

// ============================================================
// 全局模块片段：第三方与标准库头文件
// 所有 spdlog / fmtlib 的使用被隔离在此实现单元，
// 不会通过模块接口泄漏给消费方。
// ============================================================
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module nandina.log;

// ============================================================
// 模块内部状态（对外完全不可见）
// ============================================================
namespace nandina::log::detail {

    struct State {
        std::vector<spdlog::sink_ptr> sinks;
        std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> named_loggers;
        std::mutex mtx;
        bool ready{false};
    };

    [[nodiscard]] inline State &global() noexcept {
        static State s;
        return s;
    }

    [[nodiscard]] inline auto to_spd(int lvl) noexcept -> spdlog::level::level_enum {
        const int clamped = std::clamp(lvl, 0, static_cast<int>(spdlog::level::off));
        return static_cast<spdlog::level::level_enum>(clamped);
    }

    [[nodiscard]] inline std::shared_ptr<spdlog::logger> ensure_named(std::string_view name) {
        auto &s = global();
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

    // ── 实现 log.cppm 中声明的 detail 非模板函数 ──────────────

    // 向指定 logger（void* 类型擦除）写入一条已格式化消息
    void write(void *logger_ptr, int level, std::string msg) {
        if (!logger_ptr)
            return;
        auto *lg = static_cast<spdlog::logger *>(logger_ptr);
        lg->log(spdlog::source_loc{}, to_spd(level), msg.c_str());
    }

    // 向全局根 logger 写入一条已格式化消息
    void write_default(int level, std::string msg) {
        auto *lg = spdlog::default_logger_raw();
        if (lg)
            lg->log(spdlog::source_loc{}, to_spd(level), msg.c_str());
    }

} // namespace nandina::log::detail

// ============================================================
// Logger 成员函数实现
// ============================================================
namespace nandina::log {

    void Logger::set_level(Level level) noexcept {
        if (!m_impl)
            return;
        auto *lg = static_cast<spdlog::logger *>(m_impl.get());
        lg->set_level(detail::to_spd(static_cast<int>(level)));
    }

    void Logger::flush() {
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
        auto &s = detail::global();
        std::scoped_lock lk(s.mtx);
        if (s.ready)
            return;

        // 控制台 sink — 带 ANSI 色彩，输出到 stdout
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_level(detail::to_spd(static_cast<int>(console_level)));
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
        spdlog::set_level(detail::to_spd(static_cast<int>(console_level)));

        s.ready = true;
    }

    void shutdown() {
        // 先释放我们持有的引用，再让 spdlog 做最终清理，避免 use-after-free
        {
            auto &s = detail::global();
            std::scoped_lock lk(s.mtx);
            s.named_loggers.clear();
            s.sinks.clear();
            s.ready = false;
        }
        spdlog::shutdown();
    }

    void set_level(Level level) noexcept {
        const auto spd = detail::to_spd(static_cast<int>(level));
        spdlog::set_level(spd);

        auto &s = detail::global();
        std::scoped_lock lk(s.mtx);
        for (auto &[_, lg]: s.named_loggers)
            lg->set_level(spd);
    }

    void flush() {
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger> &lg) { lg->flush(); });
    }

    Logger get(std::string_view name) {
        auto spdlog_ptr = detail::ensure_named(name);
        // aliasing 构造：shared_ptr<void> 与 shared_ptr<logger> 共享引用计数，
        // 但对外暴露 void*，消费方无需感知 spdlog 类型
        std::shared_ptr<void> erased{spdlog_ptr, spdlog_ptr.get()};
        return Logger{std::move(erased), std::string(name)};
    }

} // namespace nandina::log
