//
// tools/nandina_main — project-level CLI (D3 first slice).
//
// Commands:
//   nandina new <path> [--package <package-id>] [--nandina-source <path>]
//                                                  scaffold a minimal application
//   nandina build [path] [--build-dir <path>]     configure and compile a project
//   nandina run [path] [--build-dir <path>]       build and run a project
//   nandina doctor [path]                         check toolchain and project
//   nandina --version | --help
//
// `new` writes a self-contained project: meson.build (nandina subproject + the D1
// resource toolchain), src/main.cpp (a minimal `app::run<MainPage>` program), and the
// default `resources/` convention layout. `nanres` remains the resource solver; this
// command only owns project scaffolding and workflow actions.
//

#include "nandina_cli_config.hpp"

#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace
{
    constexpr std::string_view version = "0.1.0";

    struct ProcessResult {
        int exit_code = 0;
        std::string output;
    };

    struct ToolVersion {
        int major = 0;
        int minor = 0;
    };

    [[nodiscard]] auto process_exit_code(const int status) -> int {
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return 1;
    }

    [[nodiscard]] auto run_process(
        const std::vector<std::string>& args,
        const std::filesystem::path& working_directory,
        const bool capture
    ) -> ProcessResult {
        if (args.empty()) {
            throw std::invalid_argument("process command is empty");
        }

        std::array<int, 2> output_pipe {-1, -1};
        if (capture && ::pipe(output_pipe.data()) != 0) {
            throw std::runtime_error("cannot create process output pipe");
        }

        std::cout.flush();
        std::cerr.flush();
        const auto pid = ::fork();
        if (pid < 0) {
            if (capture) {
                (void)::close(output_pipe[0]);
                (void)::close(output_pipe[1]);
            }
            throw std::runtime_error("cannot create child process");
        }
        if (pid == 0) {
            if (!working_directory.empty()
                && ::chdir(working_directory.c_str()) != 0)
            {
                ::_exit(126);
            }
            if (capture) {
                (void)::close(output_pipe[0]);
                if (::dup2(output_pipe[1], STDOUT_FILENO) < 0
                    || ::dup2(output_pipe[1], STDERR_FILENO) < 0)
                {
                    ::_exit(126);
                }
                (void)::close(output_pipe[1]);
            }

            std::vector<char*> raw_args;
            raw_args.reserve(args.size() + 1);
            for (const auto& arg: args) {
                raw_args.push_back(const_cast<char*>(arg.c_str()));
            }
            raw_args.push_back(nullptr);
            ::execvp(raw_args.front(), raw_args.data());
            ::_exit(errno == ENOENT ? 127 : 126);
        }

        ProcessResult result;
        if (capture) {
            (void)::close(output_pipe[1]);
            std::array<char, 4096> buffer {};
            while (true) {
                const auto count = ::read(output_pipe[0], buffer.data(), buffer.size());
                if (count > 0) {
                    result.output.append(buffer.data(), static_cast<std::size_t>(count));
                    continue;
                }
                if (count < 0 && errno == EINTR) {
                    continue;
                }
                break;
            }
            (void)::close(output_pipe[0]);
        }

        int status = 0;
        while (::waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                throw std::runtime_error("cannot wait for child process");
            }
        }
        result.exit_code = process_exit_code(status);
        return result;
    }

    [[nodiscard]] auto first_line(std::string_view value) -> std::string_view {
        const auto end = value.find_first_of("\r\n");
        return value.substr(0, end);
    }

    [[nodiscard]] auto parse_version(std::string_view value) -> std::optional<ToolVersion> {
        std::size_t cursor = 0;
        while (cursor < value.size()
               && std::isdigit(static_cast<unsigned char>(value[cursor])) == 0)
        {
            ++cursor;
        }
        if (cursor == value.size()) {
            return std::nullopt;
        }

        ToolVersion result;
        while (cursor < value.size()
               && std::isdigit(static_cast<unsigned char>(value[cursor])) != 0)
        {
            result.major = result.major * 10 + (value[cursor] - '0');
            ++cursor;
        }
        if (cursor == value.size() || value[cursor] != '.') {
            return std::nullopt;
        }
        ++cursor;
        if (cursor == value.size()
            || std::isdigit(static_cast<unsigned char>(value[cursor])) == 0)
        {
            return std::nullopt;
        }
        while (cursor < value.size()
               && std::isdigit(static_cast<unsigned char>(value[cursor])) != 0)
        {
            result.minor = result.minor * 10 + (value[cursor] - '0');
            ++cursor;
        }
        return result;
    }

    [[nodiscard]] auto version_at_least(
        const ToolVersion actual,
        const ToolVersion minimum
    ) -> bool {
        return actual.major > minimum.major
            || (actual.major == minimum.major && actual.minor >= minimum.minor);
    }

    [[nodiscard]] auto sanitize_name(std::string_view value) -> std::string {
        std::string result;
        for (const char character: value) {
            const bool valid = (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9')
                || character == '_' || character == '-';
            result.push_back(valid ? character : '_');
        }
        if (result.empty() || (result.front() >= '0' && result.front() <= '9')) {
            result.insert(result.begin(), 'n');
        }
        return result;
    }

    [[nodiscard]] auto valid_package_id(std::string_view value) -> bool {
        if (value.empty() || value.find('/') != std::string_view::npos) { return false; }
        for (const char character: value) {
            const bool valid = (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9')
                || character == '_' || character == '-' || character == '.';
            if (!valid || character == ' ' || character == '\t') { return false; }
        }
        return true;
    }

    void write_file(const std::filesystem::path& path, std::string_view content) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("cannot write " + path.string());
        }
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!output) {
            throw std::runtime_error("cannot write " + path.string());
        }
    }

    [[nodiscard]] auto template_meson_build(std::string_view name) -> std::string {
        return "project('" + std::string(name)
            + "', 'cpp', default_options: ['cpp_std=c++26', 'default_library=static'])\n"
              "\n"
              "nandina = subproject('nandina', default_options: ['build_tests=false', 'physics2d=disabled'])\n"
              "nandina_dep = nandina.get_variable('nandina_dep')\n"
              "toolchain = nandina.get_variable('nandina_resource_toolchain')\n"
              "\n"
              "nanres_package = custom_target(\n"
              "    '" + std::string(name) + "-resources',\n"
              "    input: files('resources/resources.toml'),\n"
              "    output: 'resources',\n"
              "    command: [toolchain['build_helper'], 'build', toolchain['nanres'], '@INPUT@', '@OUTPUT@'],\n"
              "    build_by_default: true,\n"
              "    build_always_stale: true,\n"
              ")\n"
              "\n"
              "executable(\n"
              "    '" + std::string(name) + "',\n"
              "    files('src/main.cpp'),\n"
              "    dependencies: [nandina_dep],\n"
              ")\n";
    }

    [[nodiscard]] auto template_main_cpp(std::string_view package_id, std::string_view name)
        -> std::string {
        return "#include \"app/nan_application.hpp\"\n"
               "#include \"app/nan_page.hpp\"\n"
               "#include \"widget/controls.hpp\"\n"
               "\n"
               "using namespace nandina;\n"
               "\n"
               "class MainPage final: public app::Page<> {\n"
               "public:\n"
               "    auto build(widget::BuildContext& ui) -> widget::View override {\n"
               "        return ui.make<widget::Label>(\"Hello, Nandina!\").build();\n"
               "    }\n"
               "};\n"
               "\n"
               "auto main() -> int {\n"
               "    return app::run<MainPage>({\n"
               "        .id = \"" + std::string(package_id) + "\",\n"
               "        .window = {.title = \"" + std::string(name) + "\"},\n"
               "    });\n"
               "}\n";
    }

    void print_help() {
        std::cout
            << "nandina " << version << " — project-level workflow for Nandina\n"
            << "\n"
            << "Usage:\n"
            << "  nandina new <path> [--package <package-id>] [--nandina-source <path>]\n"
            << "                                                  scaffold a new application\n"
            << "  nandina build [path] [--build-dir <path>]     configure and compile\n"
            << "  nandina run [path] [--build-dir <path>] [--no-build] [-- <args...>]\n"
            << "                                                  build and run\n"
            << "  nandina doctor [path]                         check toolchain and project\n"
            << "  nandina --version                             print the version\n"
            << "  nandina --help                                print this help\n";
    }

    [[nodiscard]] auto resolve_project_root(std::string_view path) -> std::filesystem::path {
        const auto requested = path.empty() ? std::filesystem::current_path()
                                            : std::filesystem::path(path);
        std::error_code error;
        auto root = std::filesystem::weakly_canonical(
            std::filesystem::absolute(requested, error), error
        );
        if (error || !std::filesystem::is_directory(root)
            || !std::filesystem::is_regular_file(root / "meson.build"))
        {
            throw std::runtime_error(
                "invalid project root (expected meson.build): " + requested.string()
            );
        }
        return root;
    }

    [[nodiscard]] auto resolve_build_directory(
        const std::filesystem::path& root,
        std::string_view path
    ) -> std::filesystem::path {
        const auto requested = path.empty() ? std::filesystem::path("build")
                                            : std::filesystem::path(path);
        return requested.is_absolute() ? requested.lexically_normal()
                                       : (root / requested).lexically_normal();
    }

    [[nodiscard]] auto project_target(const std::filesystem::path& root) -> std::string {
        std::ifstream input(root / ".nandina" / "target", std::ios::binary);
        std::string target;
        std::getline(input, target);
        if (!input && target.empty()) {
            throw std::runtime_error(
                "missing .nandina/target metadata; regenerate or repair the project"
            );
        }
        if (target.empty() || sanitize_name(target) != target) {
            throw std::runtime_error("invalid executable target metadata: " + target);
        }
        return target;
    }

    auto build_project(std::string_view project_path, std::string_view build_path) -> int {
        const auto root = resolve_project_root(project_path);
        const auto build = resolve_build_directory(root, build_path);
        if (!std::filesystem::is_regular_file(build / "meson-private" / "coredata.dat")) {
            std::cout << "nandina build: configuring " << build << '\n';
            const auto setup = run_process(
                {"meson", "setup", build.string()}, root, false
            );
            if (setup.exit_code != 0) {
                std::cerr << "nandina build: meson setup failed with exit code "
                          << setup.exit_code << '\n';
                return setup.exit_code;
            }
        }
        std::cout << "nandina build: compiling " << build << '\n';
        const auto compile = run_process(
            {"meson", "compile", "-C", build.string()}, root, false
        );
        if (compile.exit_code != 0) {
            std::cerr << "nandina build: compile failed with exit code " << compile.exit_code
                      << '\n';
        }
        return compile.exit_code;
    }

    auto run_project(
        std::string_view project_path,
        std::string_view build_path,
        const bool no_build,
        const std::vector<std::string>& forwarded
    ) -> int {
        const auto root = resolve_project_root(project_path);
        const auto build = resolve_build_directory(root, build_path);
        if (!no_build) {
            const auto build_result = build_project(root.string(), build.string());
            if (build_result != 0) {
                return build_result;
            }
        }

        const auto executable = build / project_target(root);
        if (!std::filesystem::is_regular_file(executable) || ::access(executable.c_str(), X_OK) != 0) {
            std::cerr << "nandina run: executable is missing or not runnable: " << executable
                      << '\n';
            return 2;
        }
        std::vector<std::string> command {executable.string()};
        command.insert(command.end(), forwarded.begin(), forwarded.end());
        std::cout << "nandina run: " << executable << '\n';
        return run_process(command, root, false).exit_code;
    }

    auto check_versioned_tool(
        std::string_view name,
        const std::vector<std::string>& command,
        const ToolVersion minimum
    ) -> bool {
        const auto result = run_process(command, {}, true);
        if (result.exit_code != 0) {
            std::cerr << "doctor: " << name << " is missing or failed to run\n";
            return false;
        }
        const auto actual = parse_version(result.output);
        if (!actual || !version_at_least(*actual, minimum)) {
            std::cerr << "doctor: " << name << " must be >= " << minimum.major << '.'
                      << minimum.minor << " (found " << first_line(result.output) << ")\n";
            return false;
        }
        std::cout << "doctor: " << name << " ok (" << first_line(result.output) << ")\n";
        return true;
    }

    auto check_tool(std::string_view name, const std::vector<std::string>& command) -> bool {
        const auto result = run_process(command, {}, true);
        if (result.exit_code != 0) {
            std::cerr << "doctor: " << name << " is missing or failed to run\n";
            return false;
        }
        std::cout << "doctor: " << name << " ok (" << first_line(result.output) << ")\n";
        return true;
    }

    auto check_compiler() -> bool {
        const auto result = run_process({"c++", "--version"}, {}, true);
        if (result.exit_code != 0) {
            std::cerr << "doctor: c++ is missing or failed to run\n";
            return false;
        }

        std::string_view family;
        ToolVersion minimum;
        if (result.output.find("clang") != std::string::npos) {
            family = "Clang";
            minimum = {21, 0};
        }
        else if (result.output.find("GCC") != std::string::npos
                 || result.output.find("gcc") != std::string::npos
                 || result.output.find("g++") != std::string::npos
                 || result.output.find("Free Software Foundation") != std::string::npos)
        {
            family = "GCC";
            minimum = {16, 0};
        }
        else {
            std::cerr << "doctor: unsupported C++ compiler (expected GCC or Clang; found "
                      << first_line(result.output) << ")\n";
            return false;
        }

        const auto actual = parse_version(result.output);
        if (!actual || !version_at_least(*actual, minimum)) {
            std::cerr << "doctor: " << family << " must be >= " << minimum.major << '.'
                      << minimum.minor << " (found " << first_line(result.output) << ")\n";
            return false;
        }
        std::cout << "doctor: c++ ok (" << family << ' ' << actual->major << '.' << actual->minor
                  << "; " << first_line(result.output) << ")\n";
        return true;
    }

    auto check_cpp26() -> bool {
        std::error_code error;
        const auto root = std::filesystem::temp_directory_path(error)
            / ("nandina-doctor-" + std::to_string(
                   std::chrono::steady_clock::now().time_since_epoch().count()
               ));
        if (error || !std::filesystem::create_directories(root, error) || error) {
            std::cerr << "doctor: cannot create C++26 probe directory\n";
            return false;
        }
        const auto source = root / "probe.cpp";
        const auto binary = root / "probe";
        try {
            write_file(
                source,
                "#include <expected>\n"
                "#include <functional>\n"
                "#include <print>\n"
                "int main() {\n"
                "  std::expected<int, int> value = 1;\n"
                "  std::move_only_function<int()> fn = [] { return 2; };\n"
                "  std::println(\"{}\", *value + fn());\n"
                "}\n"
            );
            const auto result = run_process(
                {"c++", "-std=c++26", source.string(), "-o", binary.string()}, root, true
            );
            (void)std::filesystem::remove_all(root, error);
            if (result.exit_code != 0) {
                std::cerr << "doctor: C++26 compile/link probe failed\n" << result.output;
                return false;
            }
        }
        catch (...) {
            (void)std::filesystem::remove_all(root, error);
            throw;
        }
        std::cout << "doctor: C++26 standard library probe ok\n";
        return true;
    }

    auto check_nandina_source(const std::filesystem::path& source) -> bool {
        bool ok = true;
        if (!std::filesystem::is_regular_file(source / "meson.build")
            || !std::filesystem::is_regular_file(source / "nandina" / "meson.build"))
        {
            std::cerr << "doctor: invalid Nandina source: " << source << '\n';
            return false;
        }
        constexpr std::string_view required_subprojects[] = {
            "Catch2",
            "freetype",
            "fribidi",
            "harfbuzz",
            "nlohmann_json",
            "raylib",
            "spdlog",
            "tomlplusplus",
            "utf8proc",
        };
        for (const auto dependency: required_subprojects) {
            const auto dependency_root = source / "subprojects" / dependency;
            if (!std::filesystem::exists(dependency_root)
                || std::filesystem::is_empty(dependency_root))
            {
                std::cerr << "doctor: Nandina submodule missing or empty: "
                          << dependency_root << '\n';
                ok = false;
            }
        }
        return ok;
    }

    auto check_project(const std::filesystem::path& root) -> bool {
        bool ok = true;
        if (std::filesystem::is_regular_file(root / "nandina" / "meson.build")) {
            ok = check_nandina_source(root);
        }
        else {
            const auto require_file = [&](const std::filesystem::path& path,
                                          std::string_view label) {
                if (!std::filesystem::is_regular_file(path)) {
                    std::cerr << "doctor: project missing " << label << ": " << path << '\n';
                    ok = false;
                }
            };
            require_file(root / "resources" / "resources.toml", "resource manifest");
            require_file(root / ".nandina" / "target", "CLI target metadata");

            const auto source = root / "subprojects" / "nandina";
            const auto wrap = root / "subprojects" / "nandina.wrap";
            if (std::filesystem::exists(source)) {
                ok = check_nandina_source(source) && ok;
            }
            else if (!std::filesystem::is_regular_file(wrap)) {
                std::cerr << "doctor: project has neither subprojects/nandina nor nandina.wrap\n";
                ok = false;
            }
        }

        if (ok) {
            std::cout << "doctor: project ok (" << root << ")\n";
        }
        return ok;
    }

    auto doctor(std::string_view project_path, const bool project_required) -> int {
        bool ok = true;
        ok = check_versioned_tool("meson", {"meson", "--version"}, {1, 3}) && ok;
        ok = check_versioned_tool("ninja", {"ninja", "--version"}, {1, 10}) && ok;
        ok = check_tool("cmake", {"cmake", "--version"}) && ok;
        ok = check_compiler() && ok;
        ok = check_tool("python3", {"python3", "--version"}) && ok;
        ok = check_tool("pkg-config", {"pkg-config", "--version"}) && ok;
        ok = check_tool("git", {"git", "--version"}) && ok;
        if (run_process({"pkg-config", "--atleast-version=3.0", "openssl"}, {}, true).exit_code
            != 0)
        {
            std::cerr << "doctor: OpenSSL >= 3.0 development package is required\n";
            ok = false;
        }
        else {
            std::cout << "doctor: OpenSSL >= 3.0 ok\n";
        }
        ok = check_cpp26() && ok;

        std::error_code error;
        const auto candidate = project_path.empty() ? std::filesystem::current_path()
                                                     : std::filesystem::path(project_path);
        if (project_required || std::filesystem::is_regular_file(candidate / "meson.build", error)) {
            try {
                ok = check_project(resolve_project_root(candidate.string())) && ok;
            }
            catch (const std::exception& exception) {
                std::cerr << "doctor: " << exception.what() << '\n';
                ok = false;
            }
        }
        else {
            std::cout << "doctor: no project detected; toolchain-only check\n";
        }

        if (ok) {
            std::cout << "doctor: all checks passed\n";
            return 0;
        }
        return 1;
    }

    [[nodiscard]] auto resolve_nandina_source(std::string_view source) -> std::filesystem::path {
        const std::filesystem::path requested =
            source.empty() ? std::filesystem::path(NANDINA_SOURCE_ROOT)
                           : std::filesystem::path(source);
        std::error_code error;
        auto absolute = std::filesystem::absolute(requested, error);
        if (error) {
            throw std::runtime_error(
                "cannot resolve Nandina source " + requested.string() + ": " + error.message()
            );
        }
        auto resolved = std::filesystem::weakly_canonical(absolute, error);
        if (error || !std::filesystem::is_regular_file(resolved / "meson.build")
            || !std::filesystem::is_regular_file(resolved / "nandina" / "meson.build"))
        {
            throw std::runtime_error(
                "invalid Nandina source (expected meson.build and nandina/meson.build): "
                + resolved.string()
            );
        }
        return resolved;
    }

    [[nodiscard]] auto staging_path_for(const std::filesystem::path& root)
        -> std::filesystem::path {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        return root.parent_path()
            / ("." + root.filename().string() + ".nandina-tmp-" + std::to_string(stamp));
    }

    auto new_project(
        std::string_view path,
        std::string_view package_id,
        std::string_view nandina_source
    ) -> int {
        if (path.empty()) {
            std::cerr << "nandina new: path is required\n";
            return 2;
        }
        std::error_code error;
        auto root = std::filesystem::absolute(std::filesystem::path(path), error).lexically_normal();
        if (!error && root.filename().empty() && root.has_parent_path()) {
            root = root.parent_path();
        }
        if (error || root.filename().empty()) {
            std::cerr << "nandina new: invalid destination: " << path << '\n';
            return 2;
        }
        if (std::filesystem::exists(root, error) && !std::filesystem::is_empty(root, error)) {
            std::cerr << "nandina new: destination exists and is not empty: " << root << '\n';
            return 2;
        }
        const auto name = sanitize_name(root.filename().string());
        const auto resolved_package = package_id.empty() ? "org.example." + name
                                                          : std::string(package_id);
        if (!valid_package_id(resolved_package)) {
            std::cerr << "nandina new: invalid package id: " << resolved_package << '\n';
            return 2;
        }

        std::filesystem::path source;
        try {
            source = resolve_nandina_source(nandina_source);
        }
        catch (const std::exception& exception) {
            std::cerr << "nandina new: " << exception.what() << '\n';
            return 2;
        }

        const bool destination_existed = std::filesystem::exists(root);
        const auto staging = staging_path_for(root);
        try {
            std::filesystem::create_directories(staging / "src", error);
            std::filesystem::create_directories(staging / "resources" / "assets", error);
            std::filesystem::create_directories(staging / "subprojects", error);
            if (error) {
                throw std::runtime_error("cannot create staging directories: " + error.message());
            }
            write_file(staging / "meson.build", template_meson_build(name));
            write_file(staging / "src" / "main.cpp", template_main_cpp(resolved_package, name));
            write_file(staging / "resources" / "resources.toml",
                       "package = \"" + resolved_package + "\"\n");
            write_file(staging / "resources" / "assets" / ".gitkeep", "");
            std::filesystem::create_directories(staging / ".nandina", error);
            if (error) {
                throw std::runtime_error("cannot create CLI metadata directory: " + error.message());
            }
            write_file(staging / ".nandina" / "target", name + "\n");
            write_file(staging / ".gitignore", "build/\nsubprojects/nandina\n");
            std::filesystem::create_directory_symlink(
                source, staging / "subprojects" / "nandina", error
            );
            if (error) {
                throw std::runtime_error("cannot link Nandina source: " + error.message());
            }

            if (destination_existed) {
                if (!std::filesystem::remove(root, error) || error) {
                    throw std::runtime_error(
                        "cannot replace empty destination: " + error.message()
                    );
                }
            }
            std::filesystem::rename(staging, root, error);
            if (error) {
                if (destination_existed) {
                    std::error_code restore_error;
                    (void)std::filesystem::create_directory(root, restore_error);
                }
                throw std::runtime_error("cannot publish staged project: " + error.message());
            }
        }
        catch (const std::exception& exception) {
            std::error_code cleanup_error;
            (void)std::filesystem::remove_all(staging, cleanup_error);
            std::cerr << "nandina new: " << exception.what() << '\n';
            return 2;
        }

        std::cout << "created " << name << " at " << root << '\n';
        std::cout << "  package: " << resolved_package << '\n';
        std::cout << "  nandina: " << source << '\n';
        std::cout << "next: nandina build " << root << " && nandina run " << root << '\n';
        return 0;
    }

    struct ProjectCommandOptions {
        std::string_view project = ".";
        std::string_view build_directory = "build";
        bool no_build = false;
        std::vector<std::string> forwarded;
    };

    [[nodiscard]] auto parse_project_options(
        const std::vector<std::string>& args,
        const bool allow_run_options
    ) -> ProjectCommandOptions {
        ProjectCommandOptions result;
        bool project_seen = false;
        for (std::size_t index = 1; index < args.size(); ++index) {
            if (allow_run_options && args[index] == "--") {
                result.forwarded.insert(
                    result.forwarded.end(), args.begin() + static_cast<std::ptrdiff_t>(index + 1),
                    args.end()
                );
                break;
            }
            if (args[index] == "--build-dir" && index + 1 < args.size()) {
                result.build_directory = args[++index];
            }
            else if (allow_run_options && args[index] == "--no-build") {
                result.no_build = true;
            }
            else if (!args[index].starts_with('-') && !project_seen) {
                result.project = args[index];
                project_seen = true;
            }
            else {
                throw std::invalid_argument("unknown argument: " + args[index]);
            }
        }
        return result;
    }
} // namespace

auto main(int argc, char** argv) -> int {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        print_help();
        return 2;
    }
    if (args[0] == "--help" || args[0] == "-h") {
        print_help();
        return 0;
    }
    if (args[0] == "--version" || args[0] == "-V") {
        std::cout << "nandina " << version << '\n';
        return 0;
    }
    if (args[0] == "new") {
        if (args.size() < 2) {
            std::cerr << "nandina new: path is required\n";
            return 2;
        }
        std::string_view package_id;
        std::string_view nandina_source;
        for (std::size_t index = 2; index < args.size(); ++index) {
            if (args[index] == "--package" && index + 1 < args.size()) {
                package_id = args[index + 1];
                ++index;
            }
            else if (args[index] == "--nandina-source" && index + 1 < args.size()) {
                nandina_source = args[index + 1];
                ++index;
            }
            else {
                std::cerr << "nandina new: unknown argument: " << args[index] << '\n';
                return 2;
            }
        }
        return new_project(args[1], package_id, nandina_source);
    }
    try {
        if (args[0] == "build") {
            const auto options = parse_project_options(args, false);
            return build_project(options.project, options.build_directory);
        }
        if (args[0] == "run") {
            const auto options = parse_project_options(args, true);
            return run_project(
                options.project,
                options.build_directory,
                options.no_build,
                options.forwarded
            );
        }
        if (args[0] == "doctor") {
            if (args.size() > 2) {
                throw std::invalid_argument("doctor accepts at most one project path");
            }
            return doctor(args.size() == 2 ? std::string_view(args[1]) : std::string_view {},
                          args.size() == 2);
        }
    }
    catch (const std::exception& exception) {
        std::cerr << "nandina " << args[0] << ": " << exception.what() << '\n';
        return 2;
    }
    std::cerr << "nandina: unknown command: " << args[0] << "\nrun 'nandina --help'\n";
    return 2;
}
