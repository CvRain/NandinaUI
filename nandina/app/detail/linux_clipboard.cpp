#include "linux_clipboard.hpp"

#include <array>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace nandina::app::detail
{
    namespace
    {
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

        class BlockedSigpipe {
        public:
            BlockedSigpipe() {
                sigemptyset(&mask_);
                sigaddset(&mask_, SIGPIPE);
                blocked_ = pthread_sigmask(SIG_BLOCK, &mask_, &previous_mask_) == 0;
                if (blocked_) {
                    sigset_t pending {};
                    if (sigpending(&pending) == 0) {
                        was_pending_ = sigismember(&pending, SIGPIPE) == 1;
                    }
                }
            }

            ~BlockedSigpipe() {
                if (!blocked_) {
                    return;
                }
                if (!was_pending_) {
                    timespec timeout {};
                    (void)sigtimedwait(&mask_, nullptr, &timeout);
                }
                (void)pthread_sigmask(SIG_SETMASK, &previous_mask_, nullptr);
            }

            BlockedSigpipe(const BlockedSigpipe&) = delete;
            auto operator=(const BlockedSigpipe&) -> BlockedSigpipe& = delete;

        private:
            sigset_t mask_ {};
            sigset_t previous_mask_ {};
            bool blocked_ = false;
            bool was_pending_ = false;
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

        [[nodiscard]] auto spawn_command(
            const std::initializer_list<std::string_view> command,
            posix_spawn_file_actions_t& actions
        ) -> std::optional<pid_t> {
            if (command.size() == 0 || command.begin()->empty()) {
                return std::nullopt;
            }

            std::vector<std::string> owned;
            auto arguments = make_arguments(command, owned);
            pid_t process = -1;
            const auto result = posix_spawnp(
                &process,
                arguments.front(),
                &actions,
                nullptr,
                arguments.data(),
                environ
            );
            return result == 0 ? std::optional<pid_t>(process) : std::nullopt;
        }

        [[nodiscard]] auto wait_for_success(const pid_t process) noexcept -> bool {
            int status = 0;
            while (waitpid(process, &status, 0) == -1) {
                if (errno != EINTR) {
                    return false;
                }
            }
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
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

        [[nodiscard]] auto
        configure_write_actions(posix_spawn_file_actions_t& actions, const Pipe& pipe) noexcept
            -> bool {
            return posix_spawn_file_actions_adddup2(&actions, pipe.read_descriptor(), STDIN_FILENO)
                == 0
                && posix_spawn_file_actions_addopen(
                       &actions,
                       STDOUT_FILENO,
                       "/dev/null",
                       O_WRONLY,
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
    } // namespace

    auto is_wayland_session(const char* wayland_display) noexcept -> bool {
        return wayland_display != nullptr && wayland_display[0] != '\0';
    }

    auto read_command_output(const std::initializer_list<std::string_view> command)
        -> std::optional<std::string> {
        Pipe pipe;
        if (!pipe.valid()) {
            return std::nullopt;
        }

        posix_spawn_file_actions_t actions {};
        if (posix_spawn_file_actions_init(&actions) != 0) {
            return std::nullopt;
        }
        const auto process =
            configure_read_actions(actions, pipe) ? spawn_command(command, actions) : std::nullopt;
        (void)posix_spawn_file_actions_destroy(&actions);
        if (!process) {
            return std::nullopt;
        }

        pipe.close_write();
        std::string output;
        std::array<char, 4096> buffer {};
        bool read_succeeded = true;
        while (true) {
            const auto count = read(pipe.read_descriptor(), buffer.data(), buffer.size());
            if (count > 0) {
                output.append(buffer.data(), static_cast<std::size_t>(count));
                continue;
            }
            if (count == 0) {
                break;
            }
            if (errno != EINTR) {
                read_succeeded = false;
                break;
            }
        }
        pipe.close_read();
        return read_succeeded && wait_for_success(*process) ? std::optional<std::string>(output)
                                                            : std::nullopt;
    }

    auto write_command_input(
        const std::initializer_list<std::string_view> command,
        const std::string_view text
    ) -> bool {
        Pipe pipe;
        if (!pipe.valid()) {
            return false;
        }

        posix_spawn_file_actions_t actions {};
        if (posix_spawn_file_actions_init(&actions) != 0) {
            return false;
        }
        const auto process =
            configure_write_actions(actions, pipe) ? spawn_command(command, actions) : std::nullopt;
        (void)posix_spawn_file_actions_destroy(&actions);
        if (!process) {
            return false;
        }

        pipe.close_read();
        BlockedSigpipe blocked_sigpipe;
        std::size_t offset = 0;
        while (offset < text.size()) {
            const auto count =
                write(pipe.write_descriptor(), text.data() + offset, text.size() - offset);
            if (count > 0) {
                offset += static_cast<std::size_t>(count);
                continue;
            }
            if (count < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
        pipe.close_write();
        return offset == text.size() && wait_for_success(*process);
    }

    auto read_wayland_clipboard() -> std::optional<std::string> {
        return read_command_output({"wl-paste", "--no-newline"});
    }

    auto write_wayland_clipboard(const std::string_view text) -> bool {
        return write_command_input({"wl-copy", "--type", "text/plain;charset=utf-8"}, text);
    }
} // namespace nandina::app::detail
