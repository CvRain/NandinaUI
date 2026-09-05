#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

auto main(int argc, char** argv) -> int {
    if (argc != 4) {
        std::cerr << "usage: embed_binary <input> <output> <symbol>\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "embed_binary: cannot open input: " << argv[1] << '\n';
        return 1;
    }
    const std::vector<unsigned char> bytes {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output) {
        std::cerr << "embed_binary: cannot open output: " << argv[2] << '\n';
        return 1;
    }

    output << "#include <array>\n#include <cstdint>\n\n"
              "namespace nandina::resource::detail\n{\n"
              "    extern const std::array<std::uint8_t, " << bytes.size() << "> " << argv[3]
           << " = {\n        ";
    output << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index != 0 && index % 12 == 0) {
            output << "\n        ";
        }
        output << "0x" << std::setw(2) << static_cast<unsigned int>(bytes[index]);
        if (index + 1 != bytes.size()) {
            output << ", ";
        }
    }
    output << "\n    };\n} // namespace nandina::resource::detail\n";
    return output ? 0 : 1;
}
