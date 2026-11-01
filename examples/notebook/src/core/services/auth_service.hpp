#ifndef NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_AUTH_SERVICE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_AUTH_SERVICE_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace nan_example::notebook::core
{
    /// Password hashing and verification for local notebook accounts.
    ///
    /// Encoded form: `pbkdf2-sha256$<iterations>$<salt-hex>$<hash-hex>`. The salt is
    /// 16 random bytes and the derived key is 32 bytes. Verification uses the
    /// iteration count stored with the hash, so the cost can be raised later without
    /// invalidating existing accounts.
    class AuthService final {
    public:
        /// Password policy shared by registration validation and the service.
        static constexpr std::size_t minimum_password_length = 8;
        /// PBKDF2-HMAC-SHA256 work factor for newly created hashes.
        static constexpr int hash_iterations = 600'000;

        /// Hash a password with a fresh random salt.
        /// `nullopt` means the platform random source or OpenSSL failed.
        [[nodiscard]] static auto hash_password(std::string_view password)
            -> std::optional<std::string>;

        /// Verify a password against an encoded hash.
        /// `nullopt` means the stored value is malformed or unsupported; `false`
        /// means the password does not match.
        [[nodiscard]] static auto
        verify_password(std::string_view password, std::string_view encoded) -> std::optional<bool>;
    };
} // namespace nan_example::notebook::core

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_AUTH_SERVICE_HPP
