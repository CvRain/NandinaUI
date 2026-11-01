#include "auth_service.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <charconv>
#include <limits>
#include <span>

namespace nan_example::notebook::core
{
    namespace
    {
        constexpr std::string_view kAlgorithm = "pbkdf2-sha256";
        constexpr std::size_t kSaltBytes = 16;
        constexpr std::size_t kHashBytes = 32;
        /// Corrupted or hostile hashes must not make login an unbounded CPU sink.
        constexpr int kMaxParsedIterations = 10'000'000;

        [[nodiscard]] auto encode_hex(const std::span<const unsigned char> bytes) -> std::string {
            constexpr char digits[] = "0123456789abcdef";
            std::string result;
            result.reserve(bytes.size() * 2);
            for (const auto byte: bytes) {
                result.push_back(digits[byte >> 4U]);
                result.push_back(digits[byte & 0x0FU]);
            }
            return result;
        }

        [[nodiscard]] auto decode_nibble(const char character) -> int {
            if (character >= '0' && character <= '9') {
                return character - '0';
            }
            if (character >= 'a' && character <= 'f') {
                return character - 'a' + 10;
            }
            if (character >= 'A' && character <= 'F') {
                return character - 'A' + 10;
            }
            return -1;
        }

        [[nodiscard]] auto
        decode_hex(const std::string_view text, const std::span<unsigned char> output) -> bool {
            if (text.size() != output.size() * 2) {
                return false;
            }
            for (std::size_t index = 0; index < output.size(); ++index) {
                const int high = decode_nibble(text[index * 2]);
                const int low = decode_nibble(text[index * 2 + 1]);
                if (high < 0 || low < 0) {
                    return false;
                }
                output[index] = static_cast<unsigned char>((high << 4) | low);
            }
            return true;
        }

        [[nodiscard]] auto derive(
            const std::string_view password,
            const std::span<const unsigned char> salt,
            const int iterations,
            const std::span<unsigned char> output
        ) -> bool {
            if (password.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                return false;
            }
            return PKCS5_PBKDF2_HMAC(
                       password.data(),
                       static_cast<int>(password.size()),
                       salt.data(),
                       static_cast<int>(salt.size()),
                       iterations,
                       EVP_sha256(),
                       static_cast<int>(output.size()),
                       output.data()
                   )
                == 1;
        }

        struct ParsedHash {
            int iterations = 0;
            std::array<unsigned char, kSaltBytes> salt {};
            std::array<unsigned char, kHashBytes> digest {};
        };

        [[nodiscard]] auto parse_hash(const std::string_view encoded)
            -> std::optional<ParsedHash> {
            std::array<std::string_view, 4> parts {};
            std::size_t start = 0;
            for (std::size_t index = 0; index + 1 < parts.size(); ++index) {
                const auto separator = encoded.find('$', start);
                if (separator == std::string_view::npos) {
                    return std::nullopt;
                }
                parts[index] = encoded.substr(start, separator - start);
                start = separator + 1;
            }
            parts.back() = encoded.substr(start);

            if (parts[0] != kAlgorithm) {
                return std::nullopt;
            }

            ParsedHash parsed;
            const auto* begin = parts[1].data();
            const auto* end = begin + parts[1].size();
            const auto [cursor, error] = std::from_chars(begin, end, parsed.iterations);
            if (error != std::errc {} || cursor != end || parsed.iterations <= 0
                || parsed.iterations > kMaxParsedIterations)
            {
                return std::nullopt;
            }
            if (!decode_hex(parts[2], parsed.salt) || !decode_hex(parts[3], parsed.digest)) {
                return std::nullopt;
            }
            return parsed;
        }
    } // namespace

    auto AuthService::hash_password(const std::string_view password) -> std::optional<std::string> {
        std::array<unsigned char, kSaltBytes> salt {};
        if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
            return std::nullopt;
        }

        std::array<unsigned char, kHashBytes> digest {};
        if (!derive(password, salt, hash_iterations, digest)) {
            return std::nullopt;
        }

        return std::string(kAlgorithm) + '$' + std::to_string(hash_iterations) + '$'
             + encode_hex(salt) + '$' + encode_hex(digest);
    }

    auto AuthService::verify_password(
        const std::string_view password,
        const std::string_view encoded
    ) -> std::optional<bool> {
        const auto parsed = parse_hash(encoded);
        if (!parsed) {
            return std::nullopt;
        }

        std::array<unsigned char, kHashBytes> actual {};
        if (!derive(password, parsed->salt, parsed->iterations, actual)) {
            return std::nullopt;
        }
        return CRYPTO_memcmp(actual.data(), parsed->digest.data(), actual.size()) == 0;
    }
} // namespace nan_example::notebook::core
