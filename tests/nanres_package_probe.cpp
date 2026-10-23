#include <nandina/resource/backends/sqlite_backend.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <string>

using namespace nandina;

auto main(int argc, char** argv) -> int {
    if (argc != 4) {
        return 2;
    }
    const std::filesystem::path database(argv[1]);
    auto backend = resource::SQLiteBackend::open({
        .database = database,
        .external_root = database.parent_path(),
    });
    if (!backend) {
        std::cerr << backend.error().message;
        return 1;
    }
    const std::string_view mode(argv[2]);
    if (mode == "embedded-font") {
        auto font = (*backend)->find(resource::ResourceKey(argv[3]));
        if (!font || !font->resource
            || (*font->resource)->storage() != resource::ResourceStorage::embedded_blob
            || (*font->resource)->size() < 4)
        {
            return 1;
        }
        const auto bytes = (*font->resource)->bytes();
        return bytes[0] == 0x00 && bytes[1] == 0x01 && bytes[2] == 0x00 && bytes[3] == 0x00 ? 0 : 1;
    }
    if (mode == "external") {
        auto embedded = (*backend)->find(resource::ResourceKey("app/text/default"));
        if (!embedded || !embedded->resource
            || (*embedded->resource)->storage() != resource::ResourceStorage::embedded_blob
            || std::string(
                   (*embedded->resource)->bytes().begin(),
                   (*embedded->resource)->bytes().end()
               ) != "hello")
        {
            return 1;
        }
        auto stream = (*backend)->open_stream(resource::ResourceKey(argv[3]));
        if (!stream || !stream->stream
            || (*stream->stream)->storage() != resource::ResourceStorage::external_file)
        {
            return 1;
        }
        std::array<std::uint8_t, 16> bytes {};
        const auto count = (*stream->stream)->read(bytes);
        return count && *count == 8 && std::string(bytes.begin(), bytes.begin() + 8) == "external"
            ? 0
            : 1;
    }
    if (mode == "external-font") {
        auto font = (*backend)->find(resource::ResourceKey(argv[3]));
        if (!font || !font->resource
            || (*font->resource)->storage() != resource::ResourceStorage::external_file
            || (*font->resource)->size() < 20'000'000)
        {
            return 1;
        }
        const auto bytes = (*font->resource)->bytes();
        return bytes.size() >= 4 && bytes[0] == 0x00 && bytes[1] == 0x01 && bytes[2] == 0x00
                && bytes[3] == 0x00
            ? 0
            : 1;
    }
    return 2;
}
