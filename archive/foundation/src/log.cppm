module;

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

export module nandina.log;

export namespace nandina::log {
    /**
     * nandina.log
     *
     * 面向框架与应用的统一日志入口。
     * 对外只暴露标准 C++ 类型，spdlog 细节全部隔离在实现单元中。
     */
    // ──────────────────────────────────────────────────────────
    // 日志级别枚举
    // 数值与 spdlog::level::level_enum 一一对应，但对外不暴露 spdlog 类型。
    // ──────────────────────────────────────────────────────────
    enum class Level : int {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5,
        Off = 6,
    };

    // 低层写入接口：模板成员与自由函数最终都汇聚到这里。
    // 使用 void* 做类型擦除，避免在模块接口里泄漏 spdlog 类型。
    void write(void *logger_ptr, Level level, const std::string &msg);
    void write_default(Level level, const std::string &msg);

    /// Logger — 具名子模块日志句柄
    ///
    /// 设计要点：
    ///   - m_impl 持有 spdlog::logger 的类型擦除 shared_ptr<void>，
    ///     消费方无需感知 spdlog 类型即可持有 / 拷贝 / 析构 Logger。
    ///   - 模板成员仅调用 detail::write（标准类型签名），
    ///     不在模板体内实例化任何 spdlog / fmtlib 模板。
    ///   - 名称缓存为 std::string，name() 无需解引用 spdlog 类型。
    ///
    /// 通过 nandina::log::get("module.sub") 获取实例。
    /// 同名多次调用共享同一底层 logger（级别修改互相可见）。
    class Logger {
    public:
        Logger(std::shared_ptr<void> impl, std::string name) noexcept : m_impl(std::move(impl)),
                                                                        m_name(std::move(name)) {
        }

        // ── 运行时配置（非模板，定义在 log.cpp 实现单元中）──

        void set_level(Level level) const noexcept;

        void flush() const;

        [[nodiscard]] std::string_view name() const noexcept {
            return m_name;
        }

        template<typename... Args>
        void trace(std::format_string<Args...> fmt, Args &&... args) {
            log(Level::Trace, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(std::format_string<Args...> fmt, Args &&... args) {
            log(Level::Debug, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void info(std::format_string<Args...> fmt, Args &&... args) {
            log(Level::Info, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warn(std::format_string<Args...> fmt, Args &&... args) {
            log(Level::Warn, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(std::format_string<Args...> fmt, Args &&... args) {
            log(Level::Error, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void critical(std::format_string<Args...> fmt, Args &&... args) {
            log(Level::Critical, fmt, std::forward<Args>(args)...);
        }

    private:
        template<typename... Args>
        void log(const Level level, std::format_string<Args...> fmt, Args &&... args) {
            write(m_impl.get(), level, std::format(fmt, std::forward<Args>(args)...));
        }

        std::shared_ptr<void> m_impl; // 实际持有 spdlog::logger（类型擦除）
        std::string m_name; // 缓存名称，避免跨模块边界解引用 spdlog 类型
    };

    // ──────────────────────────────────────────────────────────
    // 全局生命周期管理（实现在 log.cpp）
    // ──────────────────────────────────────────────────────────

    /**
     * @brief 初始化全局日志系统
     * @param app_name 根 logger 名称，通常为应用 / 库名
     * @param console_level 控制台输出的最低级别
     * @param enable_file 是否同时开启滚动文件日志
     * @param log_file 日志文件路径（enable_file = true 时生效）
     */
    void init(std::string_view app_name = "nandina",
              Level console_level = Level::Debug,
              bool enable_file = false,
              std::string_view log_file = "nandina.log");

    /**
     * 刷写所有日志并释放资源。调用后可重新 init()。
     */
    void shutdown();

    /**
     * 动态调整所有 logger（含具名子 logger）的输出级别
     */
    void set_level(Level level) noexcept;

    // flush — 立即刷写所有 logger 的缓冲区
    void flush();

    /// 具名子模块 Logger 获取
    ///
    /// 同一 name 多次调用共享同一底层 spdlog logger（线程安全）。
    /// 推荐命名规范：层级以点分隔，例如 "runtime.event_loop"。
    [[nodiscard]] Logger get(std::string_view name);

    // ──────────────────────────────────────────────────────────
    // 默认 logger 快捷函数
    //
    // 转发到全局根 logger（由 init() 创建）。
    // 格式串在编译期验证（std::format_string）。
    // ──────────────────────────────────────────────────────────

    template<typename... Args>
    void trace(std::format_string<Args...> fmt, Args &&... args) {
        write_default(Level::Trace, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args &&... args) {
        write_default(Level::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args &&... args) {
        write_default(Level::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warn(std::format_string<Args...> fmt, Args &&... args) {
        write_default(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args &&... args) {
        write_default(Level::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void critical(std::format_string<Args...> fmt, Args &&... args) {
        write_default(Level::Critical, std::format(fmt, std::forward<Args>(args)...));
    }
} // namespace nandina::log
