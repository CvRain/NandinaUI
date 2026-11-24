// app/detail/linux_system_preferences — Linux 桌面偏好读取与解析。

#include "linux_system_preferences.hpp"

#if defined(__linux__)

    #include "../../foundation/json.hpp"

    #include <algorithm>
    #include <array>
    #include <cctype>
    #include <cerrno>
    #include <charconv>
    #include <cmath>
    #include <csignal>
    #include <cstdlib>
    #include <fcntl.h>
    #include <fstream>
    #include <limits>
    #include <poll.h>
    #include <spawn.h>
    #include <sstream>
    #include <string>
    #include <utility>
    #include <vector>

    #include <sys/stat.h>
    #include <sys/wait.h>
    #include <unistd.h>

extern char** environ;

namespace nandina::app::detail
{
    namespace
    {
        constexpr auto kCommandTimeout = std::chrono::milliseconds(150);
        constexpr std::size_t kMaxCommandOutputBytes = 256U * 1024U;

        class Pipe {
        public:
            Pipe() {
                std::array<int, 2> descriptors {};
                if (pipe2(descriptors.data(), O_CLOEXEC) != 0) {
                    return;
                }
                read_ = move_above_standard(descriptors[0]);
                write_ = move_above_standard(descriptors[1]);
                if (read_ < 0 || write_ < 0) {
                    close_all();
                }
            }

            ~Pipe() {
                close_all();
            }

            Pipe(const Pipe&) = delete;
            auto operator=(const Pipe&) -> Pipe& = delete;

            [[nodiscard]] auto valid() const noexcept -> bool {
                return read_ >= 0 && write_ >= 0;
            }

            [[nodiscard]] auto read_descriptor() const noexcept -> int {
                return read_;
            }

            [[nodiscard]] auto write_descriptor() const noexcept -> int {
                return write_;
            }

            void close_read() noexcept {
                close_descriptor(read_);
            }

            void close_write() noexcept {
                close_descriptor(write_);
            }

        private:
            [[nodiscard]] static auto move_above_standard(const int descriptor) noexcept -> int {
                if (descriptor > STDERR_FILENO) {
                    return descriptor;
                }
                const auto moved = fcntl(descriptor, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
                (void)close(descriptor);
                return moved;
            }

            static void close_descriptor(int& descriptor) noexcept {
                if (descriptor >= 0) {
                    (void)close(descriptor);
                    descriptor = -1;
                }
            }

            void close_all() noexcept {
                close_read();
                close_write();
            }

            int read_ = -1;
            int write_ = -1;
        };

        [[nodiscard]] auto make_arguments(
            const std::initializer_list<std::string_view> command,
            std::vector<std::string>& owned
        ) -> std::vector<char*> {
            owned.reserve(command.size());
            for (const auto argument: command) {
                owned.emplace_back(argument);
            }
            std::vector<char*> arguments;
            arguments.reserve(owned.size() + 1);
            for (auto& argument: owned) {
                arguments.push_back(argument.data());
            }
            arguments.push_back(nullptr);
            return arguments;
        }

        [[nodiscard]] auto
        configure_read_actions(posix_spawn_file_actions_t& actions, const Pipe& pipe) noexcept
            -> bool {
            return posix_spawn_file_actions_adddup2(
                       &actions,
                       pipe.write_descriptor(),
                       STDOUT_FILENO
                   )
                == 0
                && posix_spawn_file_actions_addopen(
                       &actions,
                       STDIN_FILENO,
                       "/dev/null",
                       O_RDONLY,
                       0
                   )
                == 0
                && posix_spawn_file_actions_addopen(
                       &actions,
                       STDERR_FILENO,
                       "/dev/null",
                       O_WRONLY,
                       0
                   )
                == 0
                && posix_spawn_file_actions_addclose(&actions, pipe.read_descriptor()) == 0
                && posix_spawn_file_actions_addclose(&actions, pipe.write_descriptor()) == 0;
        }

        void kill_and_reap(const pid_t process) noexcept {
            (void)kill(process, SIGKILL);
            int status = 0;
            while (waitpid(process, &status, 0) == -1) {
                if (errno != EINTR) {
                    break;
                }
            }
        }

        [[nodiscard]] auto wait_until_deadline(
            const pid_t process,
            const std::chrono::steady_clock::time_point deadline
        ) noexcept -> bool {
            int status = 0;
            while (true) {
                const pid_t result = waitpid(process, &status, WNOHANG);
                if (result == process) {
                    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
                }
                if (result == -1) {
                    if (errno == EINTR) {
                        continue;
                    }
                    return false;
                }
                const auto now = std::chrono::steady_clock::now();
                if (now >= deadline) {
                    kill_and_reap(process);
                    return false;
                }
                const auto remaining =
                    std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
                const int wait_ms =
                    static_cast<int>(std::clamp<std::int64_t>(remaining.count(), 1, 5));
                (void)poll(nullptr, 0, wait_ms);
            }
        }

        [[nodiscard]] auto trim(std::string_view value) noexcept -> std::string_view {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
                value.remove_prefix(1);
            }
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
                value.remove_suffix(1);
            }
            return value;
        }

        [[nodiscard]] auto ascii_lower(std::string_view input) -> std::string {
            std::string result(input);
            for (char& ch: result) {
                if (ch >= 'A' && ch <= 'Z') {
                    ch = static_cast<char>(ch - 'A' + 'a');
                }
            }
            return result;
        }

        [[nodiscard]] auto parse_strict_double(std::string_view text) -> std::optional<double> {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }
            double value = 0.0;
            const auto* begin = text.data();
            const auto* end = text.data() + text.size();
            const auto [ptr, ec] = std::from_chars(begin, end, value);
            if (ec != std::errc {} || ptr != end || !std::isfinite(value)) {
                return std::nullopt;
            }
            return value;
        }

        [[nodiscard]] auto parse_bool_token(std::string_view text) -> std::optional<bool> {
            const auto lowered = ascii_lower(trim(text));
            if (lowered == "true" || lowered == "1" || lowered == "yes") {
                return true;
            }
            if (lowered == "false" || lowered == "0" || lowered == "no") {
                return false;
            }
            return std::nullopt;
        }

        /// 从 `busctl --json=short` 的 variant 节点解包出最内层 `data`。
        [[nodiscard]] auto unwrap_busctl_variant(const nlohmann::json& node)
            -> const nlohmann::json* {
            const nlohmann::json* current = &node;
            for (int depth = 0; depth < 4; ++depth) {
                if (!current->is_object()) {
                    return current;
                }
                const auto data_it = current->find("data");
                const auto type_it = current->find("type");
                if (data_it == current->end() || type_it == current->end() || !type_it->is_string())
                {
                    return current;
                }
                current = &(*data_it);
            }
            return current;
        }

        [[nodiscard]] auto parse_portal_uint(const nlohmann::json& node)
            -> std::optional<std::uint64_t> {
            const auto* value = unwrap_busctl_variant(node);
            if (value == nullptr) {
                return std::nullopt;
            }
            if (value->is_number_unsigned()) {
                return value->get<std::uint64_t>();
            }
            if (value->is_number_integer()) {
                const auto signed_value = value->get<std::int64_t>();
                return signed_value >= 0 ? std::optional<std::uint64_t>(signed_value)
                                         : std::nullopt;
            }
            return std::nullopt;
        }

        [[nodiscard]] auto parse_portal_double(const nlohmann::json& node)
            -> std::optional<double> {
            const auto* value = unwrap_busctl_variant(node);
            if (value == nullptr || !value->is_number()) {
                return std::nullopt;
            }
            return value->get<double>();
        }

        [[nodiscard]] auto parse_portal_bool(const nlohmann::json& node) -> std::optional<bool> {
            const auto* value = unwrap_busctl_variant(node);
            if (value == nullptr || !value->is_boolean()) {
                return std::nullopt;
            }
            return value->get<bool>();
        }

        [[nodiscard]] auto parse_portal_string(const nlohmann::json& node)
            -> std::optional<std::string_view> {
            const auto* value = unwrap_busctl_variant(node);
            if (value == nullptr || !value->is_string()) {
                return std::nullopt;
            }
            return value->get_ref<const std::string&>();
        }

        [[nodiscard]] auto find_ini_value(
            std::string_view ini_text,
            std::string_view wanted_section,
            std::string_view wanted_key
        ) -> std::optional<std::string_view> {
            if (ini_text.starts_with("\xEF\xBB\xBF")) {
                ini_text.remove_prefix(3);
            }
            std::string_view current_section;
            std::optional<std::string_view> found;
            std::size_t pos = 0;
            while (pos < ini_text.size()) {
                const std::size_t newline = ini_text.find('\n', pos);
                const std::size_t line_end =
                    newline == std::string_view::npos ? ini_text.size() : newline;
                std::string_view line = trim(ini_text.substr(pos, line_end - pos));
                pos = newline == std::string_view::npos ? ini_text.size() : newline + 1;

                if (line.empty() || line.front() == '#' || line.front() == ';') {
                    continue;
                }
                if (line.front() == '[' && line.back() == ']' && line.size() >= 2) {
                    current_section = trim(line.substr(1, line.size() - 2));
                    continue;
                }
                if (current_section != wanted_section) {
                    continue;
                }
                const std::size_t eq = line.find('=');
                if (eq == std::string_view::npos) {
                    continue;
                }
                if (trim(line.substr(0, eq)) == wanted_key) {
                    // INI / KConfig / GKeyFile 约定：同节重复键与同名重复节以最后一次赋值为准。
                    found = trim(line.substr(eq + 1));
                }
            }
            return found;
        }

        [[nodiscard]] auto read_small_file(const std::filesystem::path& path)
            -> std::optional<std::string> {
            std::error_code error;
            if (!std::filesystem::is_regular_file(path, error) || error) {
                return std::nullopt;
            }
            const auto file_size = std::filesystem::file_size(path, error);
            if (error || file_size > kMaxCommandOutputBytes) {
                return std::nullopt;
            }
            std::ifstream stream(path, std::ios::binary);
            if (!stream) {
                return std::nullopt;
            }
            std::ostringstream buffer;
            buffer << stream.rdbuf();
            return buffer.str();
        }

        [[nodiscard]] auto resolve_config_home() -> std::filesystem::path {
            if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0')
            {
                return std::filesystem::path(xdg);
            }
            if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0') {
                return std::filesystem::path(home) / ".config";
            }
            return {};
        }

        [[nodiscard]] auto has_session_dbus_env() -> bool {
            const char* bus = std::getenv("DBUS_SESSION_BUS_ADDRESS");
            if (bus != nullptr && bus[0] != '\0') {
                return true;
            }
            const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
            if (runtime_dir == nullptr || runtime_dir[0] == '\0') {
                return false;
            }
            std::error_code error;
            return std::filesystem::exists(std::filesystem::path(runtime_dir) / "bus", error)
                && !error;
        }

        [[nodiscard]] auto has_dconf_user_db(const std::filesystem::path& config_home) -> bool {
            if (config_home.empty()) {
                return false;
            }
            std::error_code error;
            return std::filesystem::exists(config_home / "dconf" / "user", error) && !error;
        }

        [[nodiscard]] auto is_kde_desktop() -> bool {
            const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
            if (desktop == nullptr || desktop[0] == '\0') {
                return false;
            }
            return ascii_lower(desktop).find("kde") != std::string::npos;
        }

        void merge_missing(
            SystemPreferencesSnapshot& target,
            const SystemPreferencesSnapshot& fallback
        ) noexcept {
            if (!target.text_scale.has_value()) {
                target.text_scale = fallback.text_scale;
            }
            if (!target.appearance.has_value()) {
                target.appearance = fallback.appearance;
            }
            if (!target.reduced_motion.has_value()) {
                target.reduced_motion = fallback.reduced_motion;
            }
        }

        void apply_kde_files(
            SystemPreferencesSnapshot& target,
            const std::filesystem::path& config_home
        ) {
            if (config_home.empty()) {
                return;
            }
            if (!target.text_scale.has_value()) {
                if (const auto content = read_small_file(config_home / "kcmfonts")) {
                    target.text_scale = parse_kde_kcmfonts(*content);
                }
            }
            if (!target.reduced_motion.has_value()) {
                if (const auto content = read_small_file(config_home / "kdeglobals")) {
                    target.reduced_motion = parse_kde_globals_reduced_motion(*content);
                }
            }
        }

        void apply_gtk_files(
            SystemPreferencesSnapshot& target,
            const std::filesystem::path& config_home
        ) {
            if (config_home.empty()) {
                return;
            }
            for (const auto& relative:
                 {std::filesystem::path("gtk-4.0") / "settings.ini",
                  std::filesystem::path("gtk-3.0") / "settings.ini"})
            {
                if (target.text_scale.has_value() && target.reduced_motion.has_value()
                    && target.appearance.has_value())
                {
                    break;
                }
                if (const auto content = read_small_file(config_home / relative)) {
                    merge_missing(target, parse_gtk_settings_ini(*content));
                }
            }
        }

        void apply_gsettings(SystemPreferencesSnapshot& target) {
            if (!target.text_scale.has_value()) {
                if (const auto out = read_command_output_with_timeout(
                        {"gsettings", "get", "org.gnome.desktop.interface", "text-scaling-factor"},
                        kCommandTimeout
                    ))
                {
                    target.text_scale = parse_gsettings_text_scale(*out);
                }
            }
            if (!target.reduced_motion.has_value()) {
                if (const auto out = read_command_output_with_timeout(
                        {"gsettings", "get", "org.gnome.desktop.interface", "enable-animations"},
                        kCommandTimeout
                    ))
                {
                    target.reduced_motion = parse_gsettings_enable_animations(*out);
                }
            }
            if (!target.appearance.has_value()) {
                if (const auto out = read_command_output_with_timeout(
                        {"gsettings", "get", "org.gnome.desktop.interface", "color-scheme"},
                        kCommandTimeout
                    ))
                {
                    target.appearance = parse_gsettings_color_scheme(*out);
                }
            }
        }

        [[nodiscard]] auto query_live_linux_preferences() -> SystemPreferencesSnapshot {
            SystemPreferencesSnapshot result {};
            const auto config_home = resolve_config_home();
            const bool kde = is_kde_desktop();

            const bool has_dbus = has_session_dbus_env();
            const bool can_query_gsettings = has_dbus || has_dconf_user_db(config_home);

            // 1. 首选 XDG Desktop Portal（单次调用读取 appearance / text_scale / reduced_motion）。
            if (has_dbus) {
                if (const auto portal_json = read_command_output_with_timeout(
                        {"busctl",
                         "--user",
                         "--json=short",
                         "call",
                         "org.freedesktop.portal.Desktop",
                         "/org/freedesktop/portal/desktop",
                         "org.freedesktop.portal.Settings",
                         "ReadAll",
                         "as",
                         "2",
                         "org.freedesktop.appearance",
                         "org.gnome.desktop.interface"},
                        kCommandTimeout
                    ))
                {
                    merge_missing(result, parse_xdg_portal_settings_json(*portal_json));
                }
            }

            // 2. 按桌面环境顺序补齐缺失字段：KDE 优先读 kcmfonts/kdeglobals，避免被机器上
            //    附带安装的 GNOME 默认 gsettings 覆盖；非 KDE 优先 gsettings。
            if (kde) {
                apply_kde_files(result, config_home);
                apply_gtk_files(result, config_home);
                if (can_query_gsettings
                    && (!result.text_scale.has_value() || !result.reduced_motion.has_value()))
                {
                    apply_gsettings(result);
                }
            }
            else {
                if (can_query_gsettings
                    && (!result.text_scale.has_value() || !result.reduced_motion.has_value()
                        || !result.appearance.has_value()))
                {
                    apply_gsettings(result);
                }
                apply_gtk_files(result, config_home);
                apply_kde_files(result, config_home);
            }

            return result;
        }
    } // namespace

    auto read_command_output_with_timeout(
        const std::initializer_list<std::string_view> command,
        const std::chrono::milliseconds timeout
    ) -> std::optional<std::string> {
        if (command.size() == 0 || command.begin()->empty()) {
            return std::nullopt;
        }

        Pipe pipe;
        if (!pipe.valid()) {
            return std::nullopt;
        }
        const int read_flags = fcntl(pipe.read_descriptor(), F_GETFL, 0);
        if (read_flags < 0 || fcntl(pipe.read_descriptor(), F_SETFL, read_flags | O_NONBLOCK) != 0)
        {
            return std::nullopt;
        }

        posix_spawn_file_actions_t actions {};
        if (posix_spawn_file_actions_init(&actions) != 0) {
            return std::nullopt;
        }
        pid_t process = -1;
        bool spawned = false;
        if (configure_read_actions(actions, pipe)) {
            std::vector<std::string> owned;
            auto arguments = make_arguments(command, owned);
            spawned = posix_spawnp(
                          &process,
                          arguments.front(),
                          &actions,
                          nullptr,
                          arguments.data(),
                          environ
                      )
                == 0;
        }
        (void)posix_spawn_file_actions_destroy(&actions);
        if (!spawned || process <= 0) {
            return std::nullopt;
        }

        pipe.close_write();

        const auto clamped_timeout = timeout.count() > 0 ? timeout : std::chrono::milliseconds(1);
        const auto deadline = std::chrono::steady_clock::now() + clamped_timeout;

        std::string output;
        std::array<char, 4096> buffer {};
        bool read_ok = true;
        bool reached_eof = false;

        while (!reached_eof) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                read_ok = false;
                break;
            }
            const auto remaining_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
            pollfd descriptor {
                .fd = pipe.read_descriptor(),
                .events = POLLIN,
                .revents = 0,
            };
            const auto clamped_remaining = std::clamp<std::int64_t>(
                remaining_ms,
                1,
                static_cast<std::int64_t>(std::numeric_limits<int>::max())
            );
            const int ready = poll(&descriptor, 1, static_cast<int>(clamped_remaining));
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                read_ok = false;
                break;
            }
            if (ready == 0) {
                read_ok = false;
                break;
            }

            while (true) {
                const auto count = read(pipe.read_descriptor(), buffer.data(), buffer.size());
                if (count > 0) {
                    if (output.size() + static_cast<std::size_t>(count) > kMaxCommandOutputBytes) {
                        read_ok = false;
                        reached_eof = true;
                        break;
                    }
                    output.append(buffer.data(), static_cast<std::size_t>(count));
                    continue;
                }
                if (count == 0) {
                    reached_eof = true;
                    break;
                }
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                read_ok = false;
                reached_eof = true;
                break;
            }
        }

        pipe.close_read();
        if (!read_ok) {
            kill_and_reap(process);
            return std::nullopt;
        }
        if (!wait_until_deadline(process, deadline)) {
            return std::nullopt;
        }
        return output;
    }

    auto parse_xdg_portal_settings_json(const std::string_view json_text)
        -> SystemPreferencesSnapshot {
        SystemPreferencesSnapshot snapshot {};
        const auto parsed = foundation::parse_json(json_text);
        if (!parsed || !parsed->is_object()) {
            return snapshot;
        }
        const auto data_it = parsed->find("data");
        if (data_it == parsed->end() || !data_it->is_array() || data_it->empty()
            || !(*data_it)[0].is_object())
        {
            return snapshot;
        }
        const auto& namespaces = (*data_it)[0];

        if (const auto app_it = namespaces.find("org.freedesktop.appearance");
            app_it != namespaces.end() && app_it->is_object())
        {
            if (const auto color_it = app_it->find("color-scheme"); color_it != app_it->end()) {
                if (const auto scheme = parse_portal_uint(*color_it)) {
                    if (*scheme == 1U) {
                        snapshot.appearance = theme::ColorAppearance::dark;
                    }
                    else if (*scheme == 2U) {
                        snapshot.appearance = theme::ColorAppearance::light;
                    }
                }
            }
            if (const auto motion_it = app_it->find("reduced-motion"); motion_it != app_it->end()) {
                if (const auto motion = parse_portal_uint(*motion_it)) {
                    if (*motion == 1U) {
                        snapshot.reduced_motion = true;
                    }
                    else if (*motion == 0U) {
                        snapshot.reduced_motion = false;
                    }
                }
            }
        }

        if (const auto gnome_it = namespaces.find("org.gnome.desktop.interface");
            gnome_it != namespaces.end() && gnome_it->is_object())
        {
            if (const auto scale_it = gnome_it->find("text-scaling-factor");
                scale_it != gnome_it->end())
            {
                if (const auto factor = parse_portal_double(*scale_it)) {
                    snapshot.text_scale = text_scale_from_factor(*factor);
                }
            }
            if (const auto anim_it = gnome_it->find("enable-animations");
                anim_it != gnome_it->end())
            {
                if (const auto enabled = parse_portal_bool(*anim_it)) {
                    // 若 portal 的 reduced-motion 未显式给出或旧版 portal 报 0 但 GNOME
                    // 明确关闭了动画，则按关闭动画（reduced_motion = true）生效。
                    if (!*enabled) {
                        snapshot.reduced_motion = true;
                    }
                    else if (!snapshot.reduced_motion.has_value()) {
                        snapshot.reduced_motion = false;
                    }
                }
            }
            if (!snapshot.appearance.has_value()) {
                if (const auto scheme_it = gnome_it->find("color-scheme");
                    scheme_it != gnome_it->end())
                {
                    if (const auto scheme_str = parse_portal_string(*scheme_it)) {
                        snapshot.appearance = parse_gsettings_color_scheme(*scheme_str);
                    }
                }
            }
        }

        return snapshot;
    }

    auto parse_gsettings_text_scale(const std::string_view text) -> std::optional<float> {
        const auto value = parse_strict_double(text);
        return value.has_value() ? text_scale_from_factor(*value) : std::nullopt;
    }

    auto parse_gsettings_color_scheme(std::string_view text)
        -> std::optional<theme::ColorAppearance> {
        text = trim(text);
        if (text.size() >= 2
            && ((text.front() == '\'' && text.back() == '\'')
                || (text.front() == '"' && text.back() == '"')))
        {
            text = text.substr(1, text.size() - 2);
        }
        const auto lowered = ascii_lower(trim(text));
        if (lowered == "prefer-dark") {
            return theme::ColorAppearance::dark;
        }
        if (lowered == "prefer-light") {
            return theme::ColorAppearance::light;
        }
        return std::nullopt;
    }

    auto parse_gsettings_enable_animations(const std::string_view text) -> std::optional<bool> {
        const auto enabled = parse_bool_token(text);
        return enabled.has_value() ? std::optional<bool>(!*enabled) : std::nullopt;
    }

    auto parse_kde_kcmfonts(const std::string_view ini_text) -> std::optional<float> {
        const auto raw = find_ini_value(ini_text, "General", "forceFontDPI");
        if (!raw.has_value()) {
            return std::nullopt;
        }
        const auto dpi = parse_strict_double(*raw);
        if (!dpi.has_value() || *dpi <= 0.0) {
            return std::nullopt;
        }
        return text_scale_from_dpi(*dpi);
    }

    auto parse_kde_globals_reduced_motion(const std::string_view ini_text) -> std::optional<bool> {
        const auto raw = find_ini_value(ini_text, "KDE", "AnimationDurationFactor");
        if (!raw.has_value()) {
            return std::nullopt;
        }
        const auto factor = parse_strict_double(*raw);
        if (!factor.has_value() || *factor < 0.0) {
            return std::nullopt;
        }
        return *factor == 0.0;
    }

    auto parse_gtk_settings_ini(const std::string_view ini_text) -> SystemPreferencesSnapshot {
        SystemPreferencesSnapshot snapshot {};
        if (const auto raw_dpi = find_ini_value(ini_text, "Settings", "gtk-xft-dpi")) {
            if (const auto scaled_dpi = parse_strict_double(*raw_dpi);
                scaled_dpi.has_value() && *scaled_dpi > 0.0)
            {
                snapshot.text_scale = text_scale_from_dpi(*scaled_dpi / 1024.0);
            }
        }
        if (const auto raw_anim = find_ini_value(ini_text, "Settings", "gtk-enable-animations")) {
            if (const auto enabled = parse_bool_token(*raw_anim)) {
                snapshot.reduced_motion = !*enabled;
            }
        }
        if (const auto raw_dark =
                find_ini_value(ini_text, "Settings", "gtk-application-prefer-dark-theme"))
        {
            // GTK 中 0/false 仅表示“不强制暗色”，不代表显式亮色，因此只在 true 时设 dark。
            if (const auto prefer_dark = parse_bool_token(*raw_dark);
                prefer_dark.has_value() && *prefer_dark)
            {
                snapshot.appearance = theme::ColorAppearance::dark;
            }
        }
        return snapshot;
    }

    auto linux_desktop_watch_paths(const std::filesystem::path& config_home)
        -> std::vector<std::filesystem::path> {
        if (config_home.empty()) {
            return {};
        }
        return {
            config_home / "dconf" / "user",
            config_home / "kcmfonts",
            config_home / "kdeglobals",
            config_home / "gtk-4.0" / "settings.ini",
            config_home / "gtk-3.0" / "settings.ini",
        };
    }

    auto capture_linux_watch_stamps(const std::vector<std::filesystem::path>& paths)
        -> std::vector<LinuxConfigFileStamp> {
        std::vector<LinuxConfigFileStamp> stamps;
        stamps.reserve(paths.size());
        for (const auto& path: paths) {
            LinuxConfigFileStamp stamp {};
            struct stat info {};
            if (stat(path.c_str(), &info) == 0) {
                stamp.exists = true;
                stamp.device = static_cast<std::uint64_t>(info.st_dev);
                stamp.inode = static_cast<std::uint64_t>(info.st_ino);
                stamp.size = static_cast<std::int64_t>(info.st_size);
                stamp.mtime_sec = static_cast<std::int64_t>(info.st_mtim.tv_sec);
                stamp.mtime_nsec = static_cast<std::int64_t>(info.st_mtim.tv_nsec);
            }
            stamps.push_back(stamp);
        }
        return stamps;
    }

    LinuxSystemPreferencesSource::LinuxSystemPreferencesSource():
        LinuxSystemPreferencesSource(
            query_live_linux_preferences,
            [paths = linux_desktop_watch_paths(resolve_config_home())] {
                return capture_linux_watch_stamps(paths);
            }
        ) {}

    LinuxSystemPreferencesSource::LinuxSystemPreferencesSource(
        QueryFn query,
        WatchFn watch,
        ClockFn clock,
        const std::chrono::milliseconds poll_interval
    ):
        query_(std::move(query)),
        watch_(std::move(watch)),
        clock_(clock ? std::move(clock) : [] { return std::chrono::steady_clock::now(); }),
        poll_interval_(poll_interval) {}

    auto LinuxSystemPreferencesSource::snapshot() const -> SystemPreferencesSnapshot {
        // 先采文件戳再查询，确保查询期间发生的并发修改能在下一次 poll 被捕获。
        const auto stamps = watch_ ? watch_() : std::vector<LinuxConfigFileStamp> {};
        const auto current = query_ ? query_() : SystemPreferencesSnapshot {};
        last_stamps_ = stamps;
        last_snapshot_ = current;
        has_baseline_ = true;
        next_poll_ = clock_() + poll_interval_;
        return current;
    }

    auto LinuxSystemPreferencesSource::poll_changes() -> std::optional<SystemPreferencesSnapshot> {
        const auto now = clock_();
        if (has_baseline_ && now < next_poll_) {
            return std::nullopt;
        }
        next_poll_ = now + poll_interval_;

        const auto stamps = watch_ ? watch_() : std::vector<LinuxConfigFileStamp> {};
        if (has_baseline_ && watch_ && stamps == last_stamps_) {
            // 所有被监视配置文件元数据未变化（包括无 HOME/XDG_CONFIG_HOME 时的空列表）：
            // 稳态直接返回，不拉起任何外部查询。
            return std::nullopt;
        }

        const auto current = query_ ? query_() : SystemPreferencesSnapshot {};
        last_stamps_ = stamps;
        if (!has_baseline_) {
            has_baseline_ = true;
            last_snapshot_ = current;
            return current == SystemPreferencesSnapshot {} ? std::nullopt : std::optional(current);
        }
        if (current == last_snapshot_) {
            return std::nullopt;
        }
        last_snapshot_ = current;
        return current;
    }

} // namespace nandina::app::detail

#endif // __linux__
